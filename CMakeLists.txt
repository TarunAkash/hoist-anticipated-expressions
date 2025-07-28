cmake_minimum_required(VERSION 3.13)

project(HoistAnticipatedExpressionsPass LANGUAGES CXX)

if(NOT DEFINED LLVM_DIR)
  set(LLVM_DIR "$ENV{LLVM_DIR}")
endif()

if(NOT EXISTS "${LLVM_DIR}")
  message(FATAL_ERROR "LLVM_DIR is not set or invalid. Use -DLLVM_DIR=/path/to/llvm-build/lib/cmake/llvm")
endif()

set(LLVM_EXPORT_SYMBOLS_FOR_PLUGINS ON)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM_DIR: ${LLVM_DIR}")
message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(HoistAnticipatedExpressions MODULE
  lib/HoistAnticipatedExpressions.cpp
)

set_target_properties(HoistAnticipatedExpressions PROPERTIES
  PREFIX ""
  OUTPUT_NAME "LLVMHoistAnticipatedExpressions"
)

install(TARGETS HoistAnticipatedExpressions DESTINATION lib)

