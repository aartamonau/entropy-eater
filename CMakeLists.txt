cmake_minimum_required(VERSION 2.6.0)
project (EntropyEater)

set (
  ARCH um
  CACHE string
  "Choose archtecture"
)

set (
  KERNEL_DIR ${CMAKE_SOURCE_DIR}/kernel
  CACHE path
  "Choose kernel source tree directory"
)

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
  set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG" )
endif ( CMAKE_BUILD_TYPE STREQUAL "Debug" )

add_subdirectory ( src )
