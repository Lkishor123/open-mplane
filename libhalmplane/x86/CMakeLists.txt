# x86 board sources. Support both YOCTO (target_sources) and legacy (add_sources)

set(X86_SOURCES
  src/MplaneEcpri.cpp
  src/MplaneUplaneConf.cpp
  src/MplaneSync.cpp
  src/MplaneProcessingElements.cpp
  src/MplaneInterfaces.cpp
  # src/MplanePerformanceMgmt.cpp
  src/MplaneAlarms.cpp
  src/mock_hal_control.cpp
  src/HalMplane.cpp
)

# Transform relative paths to absolute paths from x86 directory
list(TRANSFORM X86_SOURCES PREPEND "${CMAKE_CURRENT_LIST_DIR}/")

# Add sources to the halmplane target
target_sources(halmplane PRIVATE ${X86_SOURCES})

# Add x86-specific include directory
target_include_directories(halmplane PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc")

# Ensure C++ compilation for all x86 sources
set_source_files_properties(${X86_SOURCES} PROPERTIES LANGUAGE CXX)

# Ensure the library uses the C++ linker
set_target_properties(halmplane PROPERTIES LINKER_LANGUAGE CXX)

message(STATUS "[halmplane] Added x86 C++ sources: ${X86_SOURCES}")