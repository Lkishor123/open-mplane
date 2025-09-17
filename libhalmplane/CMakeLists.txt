cmake_minimum_required(VERSION 3.12)

# ------------------------------------------------------------------
# Fetch tinyxml2 for all contexts
# ------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        10.0.0
)
# Make available now (will download and add targets if possible)
FetchContent_MakeAvailable(tinyxml2)

# ------------------------------------------------------------------
# Helper: configure tinyxml2 for a given target
# - ensures include paths are present so `#include "libtinyxml2/tinyxml2.h"` works
# - links the FetchContent-provided target if available
# - falls back to system find_package if needed
# ------------------------------------------------------------------
function(configure_tinyxml2_for target)
  # ----------------------------------------------------------------
  # Ensure includes like #include "libtinyxml2/tinyxml2.h" work
  # by exposing a virtual include root with "libtinyxml2/" prefix
  # ----------------------------------------------------------------
  if(NOT DEFINED tinyxml2_SOURCE_DIR)
    message(FATAL_ERROR "tinyxml2 FetchContent not available (tinyxml2_SOURCE_DIR missing)")
  endif()

  set(_tinyxml2_headers "${tinyxml2_SOURCE_DIR}")

  # Create a libtinyxml2 folder in the build tree and copy the header there.
  # Using file(COPY ...) keeps compatibility with CMake 3.12.
  set(_virtual_include "${CMAKE_BINARY_DIR}/libtinyxml2")
  file(MAKE_DIRECTORY "${_virtual_include}")
  file(COPY "${_tinyxml2_headers}/tinyxml2.h" DESTINATION "${_virtual_include}")

  # Expose build dir as an include root (so "libtinyxml2/tinyxml2.h" resolves)
  target_include_directories(${target} PRIVATE
    ${CMAKE_BINARY_DIR}          # contains "libtinyxml2/tinyxml2.h"
    ${_tinyxml2_headers}         # still allow plain "tinyxml2.h"
  )

  # ----------------------------------------------------------------
  # Link to the correct library target
  # ----------------------------------------------------------------
  if(TARGET tinyxml2)
    target_link_libraries(${target} PRIVATE tinyxml2)
    message(STATUS "[halmplane] linked FetchContent-provided tinyxml2 target")
    return()
  elseif(TARGET tinyxml2::tinyxml2)
    target_link_libraries(${target} PRIVATE tinyxml2::tinyxml2)
    message(STATUS "[halmplane] linked FetchContent-provided tinyxml2::tinyxml2 target")
    return()
  endif()

  # If embedded copy in repo exists
  if(EXISTS "${PROJECT_SOURCE_DIR}/libtinyxml2/CMakeLists.txt")
    add_subdirectory(${PROJECT_SOURCE_DIR}/libtinyxml2 ${CMAKE_BINARY_DIR}/libtinyxml2_build)
    if(TARGET tinyxml2)
      target_link_libraries(${target} PRIVATE tinyxml2)
      message(STATUS "[halmplane] linked in-tree tinyxml2 target (libtinyxml2/CMakeLists.txt)")
      return()
    endif()
  elseif(EXISTS "${PROJECT_SOURCE_DIR}/libtinyxml2/tinyxml2.cpp")
    add_library(tinyxml2_embedded STATIC ${PROJECT_SOURCE_DIR}/libtinyxml2/tinyxml2.cpp)
    target_include_directories(tinyxml2_embedded PUBLIC ${PROJECT_SOURCE_DIR}/libtinyxml2)
    target_link_libraries(${target} PRIVATE tinyxml2_embedded)
    message(STATUS "[halmplane] built tinyxml2_embedded from libtinyxml2/tinyxml2.cpp")
    return()
  endif()

  # Fallback: try system package
  find_package(tinyxml2 QUIET)
  if(TARGET tinyxml2::tinyxml2)
    target_link_libraries(${target} PRIVATE tinyxml2::tinyxml2)
    message(STATUS "[halmplane] linked system tinyxml2::tinyxml2")
  elseif(TARGET tinyxml2)
    target_link_libraries(${target} PRIVATE tinyxml2)
    message(STATUS "[halmplane] linked system tinyxml2")
  else()
    message(WARNING "[halmplane] tinyxml2 not linked. Only headers exposed at ${CMAKE_BINARY_DIR}/libtinyxml2")
  endif()
endfunction()

if("${CONTEXT}" STREQUAL "STANDALONE")

  include(GNUInstallDirs)

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

  # the -I flag in gcc
  include_directories(
    ${PROJECT_SOURCE_DIR}/inc/
    )

  set(LIB_SOURCES
      ${BUILD}/src/MplaneEcpri.c
      ${BUILD}/src/MplaneAlarms.c
      ${BUILD}/src/MplanePerformanceMgmt.c
  )

  # Library
  set(LIBRARY_NAME halmplane)
  add_library(${LIBRARY_NAME} STATIC ${LIB_SOURCES})
  
  # Configure tinyxml2 for the target
  configure_tinyxml2_for(${LIBRARY_NAME})
  
  install(TARGETS halmplane
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      )

elseif( "${CONTEXT}" STREQUAL "YOCTO")
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

  if (ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -fsanitize=address")
  endif()

  set(LIBRARY_NAME halmplane)
  set(version "1.1")
  project (halmplane VERSION 1.1.0 LANGUAGES CXX)
  add_library(${LIBRARY_NAME} SHARED)
  set_target_properties(${LIBRARY_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
  set_target_properties(${LIBRARY_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

  include(GNUInstallDirs)

  # Build context
  if(${BUILD_BOARD} STREQUAL "ZCU111")
     set(BOARD "zcu111")
  else()
     set(BOARD "${BUILD_BOARD}")
  endif()

  if (NOT GITDIR)
    get_filename_component (GITDIR ${PROJECT_SOURCE_DIR} DIRECTORY)
    list(APPEND CMAKE_MODULE_PATH ${GITDIR}/buildTools/cmake/Modules)
  endif()

  include_directories(
    ${PROJECT_SOURCE_DIR}/inc/
    ${PROJECT_SOURCE_DIR}/hal-common/inc/
  )

  add_subdirectory(hal-common)

  # Include board-specific CMakeLists.txt (including x86)
  include(${BOARD}/CMakeLists.txt)
  target_include_directories(halmplane PUBLIC hal-common/inc)
  target_include_directories(halmplane PUBLIC inc)

  # Configure tinyxml2 for the target
  configure_tinyxml2_for(${LIBRARY_NAME})

  if(${BUILD_BOARD_STYLE} STREQUAL "MODULE")
    set_target_properties(${LIBRARY_NAME} PROPERTIES OUTPUT_NAME ${LIBRARY_NAME}-mod-${BOARD})
  else()
    install(
      FILES
      inc/MplaneAlarms.h
      inc/MplaneAntennaCalibration.h
      inc/MplaneBeamforming.h
      inc/MplaneCompression.h
      inc/MplaneDelayMgmt.h
      inc/MplaneDhcp.h
      inc/MplaneEcpri.h
      inc/MplaneFaultMgmt.h
      inc/MplaneHardware.h
      inc/MplaneInterfaces.h
      inc/MplaneModuleCapability.h
      inc/MplanePerformanceMgmt.h
      inc/MplaneProcessingElements.h
      inc/MplaneSupervision.h
      inc/MplaneTransceiver.h
      inc/MplaneUplaneConf.h
      inc/MplaneExternalio.h
      inc/MplaneTypes.h
      inc/MplaneFan.h
      inc/MplaneSync.h
      inc/HalMplane.h
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()

  install(TARGETS halmplane
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

else() # fb-oru app build
  set(version "0.1")
  project (halmplane VERSION 0.1 LANGUAGES CXX)

  # Build context
  if(${BUILD_BOARD} STREQUAL "ZCU111")
     set(BOARD "zcu111")
  elseif(${BUILD_BOARD} STREQUAL "ATLAS")
     set(BOARD "atlas")
  elseif(${BUILD_BOARD} STREQUAL "FBZCU27")
     set(BOARD "fbzcu27")
  else()
     set(BOARD "zcu111")
  endif()

  if (NOT GITDIR)

    get_filename_component (GITDIR ${PROJECT_SOURCE_DIR} DIRECTORY)
    list(APPEND CMAKE_MODULE_PATH ${GITDIR}/buildTools/cmake/Modules)

  endif()

  include(rse)
  addIncludes ()
  addInterfaceIncludes(hardware)
  useRepoLib (common)
  add_subdirectory(hal-common)
  add_subdirectory(${BOARD})
  include(libraryCommonTasks)
  target_include_directories(halmplane PUBLIC inc)

  # Configure tinyxml2 for the target (if halmplane target exists)
  if(TARGET halmplane)
    configure_tinyxml2_for(halmplane)
  endif()

endif()