cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  "HPCPP"
  VERSION 1.0.0
  DESCRIPTION "High Performance C++"
  LANGUAGES CXX C
)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the `include` directory, where the headers are located
include_directories(include)

# To automatically download external dependencies, use `FetchContent`
include(FetchContent)

# Configure FetchContent to download dependencies
set(GTEST_GIT_URL "https://github.com/google/googletest.git")
set(NLOHMANN_JSON_GIT_URL "https://github.com/nlohmann/json.git")
set(CPP_HTTPLIB_GIT_URL "https://github.com/yhirose/cpp-httplib.git")
set(ROCKSDB_GIT_URL "https://github.com/facebook/rocksdb.git")
set(OPENCV_GIT_URL "https://github.com/opencv/opencv.git")
set(GDAL_GIT_URL "https://github.com/OSGeo/gdal.git")
#set(CANNYEDGE_GIT_URL "https://github.com/ICOMP-UNC/cannyEdgeFilter.git")
# set(LIBZIP_GIT_URL "https://github.com/nih-at/libzip.git")

FetchContent_Declare(
  googletest
  GIT_REPOSITORY ${GTEST_GIT_URL}
  GIT_TAG release-1.12.0  # Optionally pin to a stable release
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY ${NLOHMANN_JSON_GIT_URL}
  GIT_TAG v3.11.2
  UPDATE_COMMAND ""
)

FetchContent_Declare(
  cpp_httplib
  GIT_REPOSITORY ${CPP_HTTPLIB_GIT_URL}
  GIT_TAG v0.15.3
  UPDATE_COMMAND ""
)

# FetchContent_Declare(
#   libzip
#   GIT_REPOSITORY ${LIBZIP_GIT_URL}
#   GIT_TAG v1.10.1
#   UPDATE_COMMAND ""
# )

FetchContent_MakeAvailable(googletest)
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(cpp_httplib)
# FetchContent_MakeAvailable(libzip)

# Add the `src` directory, where the C++ source files are located
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Set the compiler flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -Werror -Wconversion -std=gnu11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Werror -Wconversion -std=c++20")

add_executable(${PROJECT_NAME} ${SOURCES})
add_executable(tcp_client "src/clients/tcp_client.c")
add_executable(udp_client "src/clients/udp_client.c")

target_include_directories(tcp_client PUBLIC lib/cJSON/include)
target_include_directories(udp_client PUBLIC lib/cJSON/include)
target_link_libraries(tcp_client socketSetup cJSON)
target_link_libraries(udp_client socketSetup cJSON)

add_subdirectory(lib/socketSetup)
add_subdirectory(lib/suppliesData)
add_subdirectory(lib/alertInfection)
add_subdirectory(lib/emergNotif)
add_subdirectory(lib/cJSON)
add_subdirectory(lib/rocksDbWrapper)
add_subdirectory(lib/myRocksDbWrapper)
add_subdirectory(lib/cannyEdgeFilter)
#add_subdirectory(lib/cppSocket)

target_include_directories(${PROJECT_NAME} PUBLIC lib/suppliesData/include)
target_include_directories(${PROJECT_NAME} PUBLIC lib/alertInfection/include)
target_include_directories(${PROJECT_NAME} PUBLIC lib/emergNotif/include)
target_include_directories(${PROJECT_NAME}  PUBLIC lib/cJSON/include)
target_include_directories(${PROJECT_NAME}  PUBLIC lib/rocksDbWrapper/include)
target_include_directories(${PROJECT_NAME}  PUBLIC lib/myRocksDbWrapper/include)
target_include_directories(${PROJECT_NAME}  PUBLIC lib/cannyEdgeFilter/include)
#target_include_directories(${PROJECT_NAME} PUBLIC lib/cppSocket/include)


#OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "OpenMP not found")
endif()

#ZLIB
find_package(ZLIB)
if(ZLIB_FOUND)
    set(ZLIB_LIB ZLIB::ZLIB)
else()
    message(FATAL_ERROR "ZLIB not found")
endif()

#OpenCV
find_package(OpenCV)
if(OpenCV_FOUND)
    set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs)
else()
  message(STATUS "OpenCV not found in the system, fetching from GitHub")
  FetchContent_Declare(
    opencv
    GIT_REPOSITORY ${OPENCV_GIT_URL}
    GIT_TAG 4.5.3
  )
  FetchContent_MakeAvailable(opencv)
  set(OpenCV_DIR ${opencv_SOURCE_DIR})
  find_package(OpenCV REQUIRED)
endif()
include_directories(${OpenCV_INCLUDE_DIRS})

#GDAL
find_package(GDAL)
if (GDAL_FOUND)
    message(STATUS "GDAL found in the system")
    set(GDAL_LIB GDAL::GDAL)
    include_directories(${GDAL_INCLUDE_DIRS})
else()
    message(STATUS "GDAL not found in the system, fetching from GitHub")
      FetchContent_Declare(
      gdal
      GIT_REPOSITORY ${GDAL_GIT_URL}
      GIT_TAG v3.3.0
    )
  FetchContent_MakeAvailable(gdal)
  set(GDAL_DIR ${gdal_SOURCE_DIR})
endif()


# if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#  target_compile_options(${PROJECT_NAME} PRIVATE -g -O0 -Wall -Wextra -pedantic -Werror -fhardened)
# else()
  #target_compile_options(${PROJECT_NAME} PRIVATE -O3)
# endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g --coverage -fsanitize=address,leak,undefined")

target_compile_options(cannyEdge PRIVATE -O3)

# Link the libraries
target_link_libraries(${PROJECT_NAME} PUBLIC 
  suppliesDataModule 
  alertInfectionModule 
  emergencyNotification 
  cJSON 
  nlohmann_json::nlohmann_json
  httplib
  rocksdb 
  rocksDBWrapper
  myRocksDBWrapper
  cannyEdge
) #SocketWrapper rocksDBWrapper

# Setup google test
if (RUN_TESTS EQUAL 1 OR RUN_COVERAGE EQUAL 1)
  add_subdirectory(tests)
endif()
