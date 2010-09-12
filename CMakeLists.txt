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

add_subdirectory ( src )
