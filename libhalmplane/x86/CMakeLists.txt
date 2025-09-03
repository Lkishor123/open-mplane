# x86 board sources. Support both YOCTO (target_sources) and legacy (add_sources)

set(X86_SRC
  src/MplaneEcpri.c
  src/MplaneUplaneConf.c
  src/MplaneSync.c
  src/MplanePerformanceMgmt.c
  src/mock_hal_control.c
  src/HalMplane.cpp
)

if("${CONTEXT}" STREQUAL "YOCTO")
  # When building as part of halmplane shared lib
  target_sources(halmplane PRIVATE ${X86_SRC})
  target_include_directories(halmplane PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
else()
  # Fallback for other build styles that use add_sources macro
  add_sources (SOURCES ${X86_SRC})
endif()
