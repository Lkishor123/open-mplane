cmake_minimum_required(VERSION 3.12)
project(server_test_shim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_path(HALMPLANE_INCLUDE_DIR HalMplane.h
  HINTS ${CMAKE_SOURCE_DIR}/../../libhalmplane/inc ${CMAKE_SOURCE_DIR}/../../../libhalmplane/inc)
find_library(HALMPLANE_LIB halmplane
  HINTS ${CMAKE_SOURCE_DIR}/../../build/lib ${CMAKE_SOURCE_DIR}/../../../build/lib)

if(NOT HALMPLANE_INCLUDE_DIR)
  message(WARNING "HalMplane.h not found; set HALMPLANE_INCLUDE_DIR to libhalmplane/inc")
endif()

include_directories(${HALMPLANE_INCLUDE_DIR})

add_executable(server-test-shim ServerTestShim.cpp)
target_link_libraries(server-test-shim ${HALMPLANE_LIB})

