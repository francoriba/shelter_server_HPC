cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  "cannyEdge"
  VERSION 1.1.0
  DESCRIPTION "Our lucky algorithm to sell to the government"
  LANGUAGES CXX
)

if(DEBUG)
  set(CMAKE_BUILD_TYPE Debug)
  set(CXX_FLAGS "-g -Wall -Wextra -Werror -Wpedantic -fsanitize=address")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clangd LSP
else()
  set(CMAKE_BUILD_TYPE Release)
endif()

if(FETCH_LIBS)
  message(STATUS "Fetching libraries from the GitHub")
  include(FetchContent)

  set(OPENCV_GIT_URL "https://github.com/opencv/opencv.git")
  set(GDAL_GIT_URL "https://github.com/OSGeo/gdal.git")

  FetchContent_Declare(
    opencv
    GIT_REPOSITORY ${OPENCV_GIT_URL}
    set(BUILD_opencv_uninstall_target OFF CACHE BOOL "Disable OpenCV uninstall target" FORCE)
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/opencv
    GIT_TAG        4.8.1  # Specify the version you need
  )

  FetchContent_Declare(
    gdal
    GIT_REPOSITORY ${GDAL_GIT_URL}
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/gdal
    GIT_TAG        v3.8.5  # Specify the version you need
  )

  FetchContent_GetProperties(opencv)
  if(NOT opencv_POPULATED)
    FetchContent_Populate(opencv)
    set(BUILD_opencv_uninstall_target OFF CACHE BOOL "Disable OpenCV uninstall target" FORCE)
    add_subdirectory(${opencv_SOURCE_DIR} ${opencv_BINARY_DIR})
  endif()

  FetchContent_GetProperties(gdal)
  if(NOT gdal_POPULATED)
    FetchContent_Populate(gdal)
    set(BUILD_GDAL_uninstall_target OFF CACHE BOOL "Disable GDAL uninstall target" FORCE)
    add_subdirectory(${gdal_SOURCE_DIR} ${gdal_BINARY_DIR})
  endif()

  set(WITH_BENCHMARK_TOOLS OFF)
  set(WITH_TOOLS OFF)
  set(WITH_TESTS OFF)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

  message(STATUS "Fetching libraries from the GitHub - done")
else()
  message(STATUS "Using libraries from the system")
  find_package(GDAL REQUIRED)
  include_directories(${GDAL_INCLUDE_DIRS})
  find_package(OpenCV REQUIRED)
  include_directories(${OpenCV_INCLUDE_DIRS})


endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

if(MAKE_EXECUTABLE)
  file(GLOB_RECURSE SOURCES "src/*.cpp")
  add_executable(${PROJECT_NAME} ${SOURCES})
else()
  file(GLOB_RECURSE SOURCES
    "src/cannyEdgeFilter.cpp"
    "src/imageFileOperations.cpp"
    "src/satelliteImageWrapper.cpp"
  )
  add_library(${PROJECT_NAME} SHARED ${SOURCES})
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARIES} ${OpenCV_LIBS})

# Add subdirectory of tests
if(RUN_TESTS)
  add_subdirectory(tests)
endif()
