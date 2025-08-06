cmake_minimum_required(VERSION 3.13)
project(HoistAnticipatedExpressions LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_llvm_library(HoistAnticipatedExpressions MODULE
  HoistAnticipatedExpressions.cpp

  PLUGIN_TOOL
  opt
)

set_target_properties(HoistAnticipatedExpressions PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
)

