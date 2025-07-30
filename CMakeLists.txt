cmake_minimum_required(VERSION 3.13)
project(HoistAnticipatedExpressions)

set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Specify required LLVM components explicitly
llvm_map_components_to_libnames(llvm_libs
  Core
  IRReader
  Support
)

add_subdirectory(lib)

add_executable(HoistAnticipatedExpressions main.cpp)
target_link_libraries(HoistAnticipatedExpressions PRIVATE HoistAnticipatedLib ${llvm_libs})

