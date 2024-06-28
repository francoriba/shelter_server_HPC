cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  "rocksDBWrapper"
  VERSION 1.1.0
  DESCRIPTION "Header wrapper facilitating seamless integration of the RocksDB database into C++ projects."
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include_directories(include)

find_package(RocksDB)

if(ROCKSDB_FOUND)
    message(STATUS "RocksDB found in the system")
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${ROCKSDB_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC rocksdb)
