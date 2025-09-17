
# Sprint 2 – Changes and Build Instructions

## Purpose
This sprint focuses on successfully building the test simulator.  
To achieve this, the `libhalmplane` x86 sources are being migrated from **C (`.c`)** to **C++ (`.cpp`)**, along with updates to the build system.  

The main goals are to enable successful builds of:
- **halmplane (x86)**
- **server-test-shim**

## Build Instructions

### 1. Build `halmplane (x86)`
```bash
echo "[sim] Building halmplane (x86)"

cmake -S "$ROOT_DIR/libhalmplane" -B "$ROOT_DIR/build/hal-x86" \
  -DCONTEXT=YOCTO \
  -DBUILD_BOARD=x86 \
  -DBUILD_BOARD_STYLE=MONO \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build "$ROOT_DIR/build/hal-x86" -j
````

### 2. Build `server-test-shim`

```bash
echo "[sim] Building server-test-shim"

HALMPLANE_LIB="$ROOT_DIR/build/hal-x86/libhalmplane.so"
HALMPLANE_INCLUDE_DIR="$ROOT_DIR/libhalmplane/inc"
HALMPLANE_X86_INCLUDE_DIR="$ROOT_DIR/libhalmplane/x86/inc"
HALMPLANE_BUILD_INCLUDE_DIR="$ROOT_DIR/build/hal-x86"

cmake -S "$ROOT_DIR/mplane_server/utils/test_shim" \
      -B "$ROOT_DIR/mplane_server/utils/test_shim/build" \
      -DHALMPLANE_LIB="$HALMPLANE_LIB" \
      -DHALMPLANE_INCLUDE_DIR="$HALMPLANE_INCLUDE_DIR" \
      -DHALMPLANE_X86_INCLUDE_DIR="$HALMPLANE_X86_INCLUDE_DIR" \
      -DHALMPLANE_BUILD_INCLUDE_DIR="$HALMPLANE_BUILD_INCLUDE_DIR"

cmake --build "$ROOT_DIR/mplane_server/utils/test_shim/build" -j
```

---

## File Changes

### Modified Files

* `libhalmplane/CMakeLists.txt`
* `libhalmplane/x86/CMakeLists.txt`
* `libhalmplane/x86/inc/mock_hal_control.h`
* `mplane_server/CMakeLists.txt`
* `mplane_server/utils/test_shim/CMakeLists.txt`
* `mplane_server/utils/test_shim/ServerTestShim.cpp`
* `tools/sim/build_sim.sh`

### Deleted Files (`.c` sources removed)

* `libhalmplane/x86/src/MplaneAlarms.c`
* `libhalmplane/x86/src/MplaneEcpri.c`
* `libhalmplane/x86/src/MplanePerformanceMgmt.c`
* `libhalmplane/x86/src/MplaneSync.c`
* `libhalmplane/x86/src/MplaneUplaneConf.c`
* `libhalmplane/x86/src/mock_hal_control.c`

### Added Files (`.cpp` replacements)

* `libhalmplane/x86/src/MplaneAlarms.cpp`
* `libhalmplane/x86/src/MplaneEcpri.cpp`
* `libhalmplane/x86/src/MplanePerformanceMgmt.cpp`
* `libhalmplane/x86/src/MplaneSync.cpp`
* `libhalmplane/x86/src/MplaneUplaneConf.cpp`
* `libhalmplane/x86/src/mock_hal_control.cpp`


---

## Notes

* Migration from `.c` → `.cpp` ensures better compatibility with C++ components in the `mplane_server`.
* Build scripts have been updated in `tools/sim/build_sim.sh` for automated compilation.
* Ensure CMake picks up the correct paths for includes and libraries when running locally.
* The header file **`MplanePerformanceMgmt.h`** currently has syntax issues and may need refactoring or fixes before a clean build.


### 3. Build `mplane_server` with HAL_TEST
```bash
echo "[sim] Building mplane_server with HAL_TEST"

cmake -S "$ROOT_DIR/mplane_server" -B "$ROOT_DIR/build/server-sim" \
      -DHAL_TEST=ON \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build "$ROOT_DIR/build/server-sim" -j
````

---

## Blockers

* Building **`mplane_server` with HAL\_TEST** currently requires additional **external dependencies** that are not yet linked in the project context.
* These missing dependencies prevent a clean build of the test simulator.
* Next steps:

  * Identify required external libraries and their versions.
  * Update `CMakeLists.txt` with proper `find_package()` or `target_link_libraries()` entries.
  * Document installation steps for these dependencies to ensure reproducibility across environments.

---

