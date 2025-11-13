# Alarm Testing Fix: Shared Memory Approach

## Problem

`inject_alarm.sh` returns `ERR:alarm\` because `mplane-server-app` and `server-test-shim` run in separate processes with isolated memory spaces. The alarm callback registered in the server is not visible to the test shim.

## Solution: POSIX Shared Memory

Store the alarm callback pointer in shared memory accessible to both processes.

## Implementation

### Step 1: Modify libhalmplane/x86/src/mock_hal_control.cpp

Replace the static callback variable with a shared memory pointer:

```cpp
// Add includes at top of file
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// Replace this line:
// static halmplane_oran_alarm_cb_t g_alarm_cb = NULL;

// With this helper function and usage:
static halmplane_oran_alarm_cb_t* get_shared_alarm_cb() {
  static halmplane_oran_alarm_cb_t* shared_cb = NULL;

  if (!shared_cb) {
    // Open or create shared memory object
    int fd = shm_open("/mplane_alarm_cb", O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
      perror("shm_open");
      return NULL;
    }

    // Set size to hold one function pointer
    if (ftruncate(fd, sizeof(halmplane_oran_alarm_cb_t)) < 0) {
      perror("ftruncate");
      close(fd);
      return NULL;
    }

    // Map shared memory into process address space
    shared_cb = (halmplane_oran_alarm_cb_t*)mmap(
      NULL,
      sizeof(halmplane_oran_alarm_cb_t),
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fd,
      0
    );

    close(fd);  // Can close after mmap

    if (shared_cb == MAP_FAILED) {
      perror("mmap");
      return NULL;
    }
  }

  return shared_cb;
}
```

### Step 2: Update mock_hal_reset()

Modify the reset function (around line 63):

```cpp
void mock_hal_reset(void) {
  g_sync_locked = 0;

  // Reset shared alarm callback
  halmplane_oran_alarm_cb_t* cb = get_shared_alarm_cb();
  if (cb) {
    *cb = NULL;
  }

  g_rx_carrier_cb = NULL;
  g_tx_carrier_cb = NULL;
  memset(g_rx_carriers, 0, sizeof(g_rx_carriers));
  memset(g_tx_carriers, 0, sizeof(g_tx_carriers));
  strncpy(g_sw_download_result, "OK", sizeof(g_sw_download_result) - 1);
  strncpy(g_sw_install_result, "OK", sizeof(g_sw_install_result) - 1);
  strncpy(g_sw_activate_result, "OK", sizeof(g_sw_activate_result) - 1);
  strncpy(g_sw_reset_result, "OK", sizeof(g_sw_reset_result) - 1);
  g_sw_slot[0] = '\0';
  g_sw_file[0] = '\0';
}
```

### Step 3: Update mock_hal_inject_alarm()

Modify the injection function (around line 81):

```cpp
int mock_hal_inject_alarm(
    uint16_t fault_id,
    const char* fault_source,
    int severity,
    bool is_cleared,
    const char* fault_text) {

  halmplane_oran_alarm_cb_t* cb_ptr = get_shared_alarm_cb();
  if (!cb_ptr || !(*cb_ptr)) {
    return 1;  // No callback registered
  }

  halmplane_oran_alarm_cb_t callback = *cb_ptr;

  halmplane_oran_alarm_t alarm;
  memset(&alarm, 0, sizeof(alarm));
  alarm.fault_id = fault_id;
  alarm.fault_source = (char*)fault_source;
  alarm.fault_severity = (halmplane_oran_fault_severity_t)severity;
  alarm.is_cleared = is_cleared;
  alarm.fault_text = (char*)fault_text;
  alarm.fault_time = (uint64_t)time(NULL);
  alarm.event_time = alarm.fault_time;

  callback(&alarm, NULL);
  return 0;
}
```

### Step 4: Update halmplane_registerOranAlarmCallback()

Modify the registration function (around line 190):

```cpp
int halmplane_registerOranAlarmCallback(halmplane_oran_alarm_cb_t cb) {
  halmplane_oran_alarm_cb_t* cb_ptr = get_shared_alarm_cb();
  if (!cb_ptr) {
    return 1;  // Failed to access shared memory
  }

  *cb_ptr = cb;
  return 0;
}
```

### Step 5: Cleanup on Exit

Add cleanup function in libhalmplane/x86/src/HalMplane.cpp:

```cpp
int halmplane_exit() {
  mock_hal_reset();

  // Clean up shared memory
  shm_unlink("/mplane_alarm_cb");

  return 0;
}
```

## Build and Test

### 1. Rebuild HAL library:

```bash
cd /home/fahim-bro/mplane_dev/open-mplane
cmake --build build --target halmplane-x86
```

### 2. Rebuild applications:

```bash
cmake --build build --target mplane-server-app
cmake --build build --target server-test-shim
```

### 3. Stop existing server:

```bash
sudo bash ./tools/stop_server.sh
```

### 4. Clean shared memory (important!):

```bash
sudo rm -f /dev/shm/mplane_alarm_cb
sudo rm -f /dev/shm/sr_*
```

### 5. Start server:

```bash
sudo bash ./tools/run_server_only.sh
```

### 6. Test alarm injection:

```bash
./tools/sim/inject_alarm.sh 1001 Major false "test alarm"
```

**Expected output:** `OK` (not `ERR:alarm\`)

### 7. Verify in logs:

```bash
tail -50 /var/log/console.log | grep -i alarm
```

You should see alarm notification being processed.

## Troubleshooting

### Issue: Permission denied on shm_open

```bash
# Check shared memory permissions
ls -la /dev/shm/mplane_alarm_cb

# If owned by wrong user:
sudo rm /dev/shm/mplane_alarm_cb
```

### Issue: Still getting ERR:alarm

```bash
# 1. Check if shared memory is created
ls -la /dev/shm/mplane_alarm_cb

# 2. Verify both processes are running
ps aux | grep -E "mplane-server|test-shim"

# 3. Test callback registration
sudo gdb -p $(pgrep mplane-server-app) -batch \
  -ex 'call shm_open("/mplane_alarm_cb", 2, 0666)'

# 4. Check for compilation errors
grep -i "shm_open\|mmap" build/CMakeFiles/halmplane-x86.dir/build.make
```

### Issue: Callback pointer is NULL in test-shim

The test-shim calls `mock_hal_reset()` at startup which sets the callback to NULL. This happens BEFORE the server registers the callback.

**Timing Fix**: Remove the `mock_hal_reset()` call from test-shim initialization:

```cpp
// In mplane_server/utils/test_shim/ServerTestShim.cpp:135
int main() {
  std::signal(SIGINT, on_sigint);
  std::signal(SIGTERM, on_sigint);

  // REMOVE THIS LINE:
  // halmplane_init(nullptr);

  // Or modify halmplane_init to NOT reset shared callback

  srv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  // ... rest of code
}
```

## Alternative: Don't Reset Shared Callback

Modify `libhalmplane/x86/src/HalMplane.cpp`:

```cpp
int halmplane_init(tinyxml2::XMLDocument* doc) {
  (void)doc;

  // Don't call mock_hal_reset() which would clear the shared callback
  // Only reset process-local state

  g_sync_locked = 0;
  g_rx_carrier_cb = NULL;
  g_tx_carrier_cb = NULL;
  // ... reset other non-shared state

  return 0;
}
```

## Verification Commands

```bash
# Check shared memory exists
ls -la /dev/shm/mplane_alarm_cb

# Read callback value (should be non-zero after server starts)
xxd /dev/shm/mplane_alarm_cb

# Monitor alarm notifications
tail -f /var/log/console.log | grep -i alarm

# Test multiple alarms
./tools/sim/inject_alarm.sh 1001 Critical false "Critical alarm"
./tools/sim/inject_alarm.sh 2002 Major false "Major alarm"
./tools/sim/inject_alarm.sh 3003 Minor false "Minor alarm"
./tools/sim/inject_alarm.sh 1001 Major true "Clear alarm 1001"
```

## Pros and Cons

### Advantages
- ✅ Minimal code changes (single file modification)
- ✅ Works with existing architecture
- ✅ No changes to test scripts
- ✅ Both processes can use same callback
- ✅ POSIX standard, portable

### Disadvantages
- ⚠️ Adds shared memory dependency
- ⚠️ Requires cleanup on exit
- ⚠️ Potential permission issues with sudo
- ⚠️ Function pointer sharing across processes (works on same architecture)

## Security Note

Storing function pointers in shared memory accessible with 0666 permissions is acceptable for development/testing but should not be used in production. For production, use proper IPC mechanisms or NETCONF RPCs.
