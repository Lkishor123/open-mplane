/*
 * Mock HAL control interface for x86 simulator
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #ifdef __cplusplus
// extern "C" {
// #endif

// Reset all mock state to defaults
void mock_hal_reset(void);

// Synchronization state: true=LOCKED, false=FREERUN/UNLOCKED
void mock_hal_set_sync_locked(bool locked);
bool mock_hal_get_sync_locked(void);

// Inject an O-RAN FM alarm via registered callback (if any)
// severity: 0=CRITICAL,1=MAJOR,2=MINOR,3=WARNING
int mock_hal_inject_alarm(
    uint16_t fault_id,
    const char* fault_source,
    int severity,
    bool is_cleared,
    const char* fault_text);

// Control software management outcomes for phases (download/install/activate/reset)
typedef struct mock_sw_event_s {
  const char* phase;   // download|install|activate|reset
  const char* result;  // COMPLETED|INTEGRITY_ERROR|FAILED|OK
  const char* slot;    // optional slot name
  const char* file;    // optional file name
} mock_sw_event_t;

// Register SW event callback (invoked when result is set)
typedef void (*mock_sw_cb_t)(const mock_sw_event_t* ev);
int mock_hal_register_sw_cb(mock_sw_cb_t cb);

// Set software mgmt outcome; optional slot/file provide context
void mock_hal_set_sw_result(const char* phase, const char* result);
void mock_hal_set_sw_result_ext(
    const char* phase, const char* result, const char* slot, const char* file);

// Trigger PM generation stub; writes CSVs under out_dir and optionally copies
// them to remote_dir to emulate an upload to a remote server
int mock_hal_trigger_pm(const char* object_name,
                        const char* out_dir,
                        const char* remote_dir);

// Provide a simple JSON status snapshot for observability
int mock_hal_status(char* buf, size_t buflen);

// #ifdef __cplusplus
// }
// #endif
