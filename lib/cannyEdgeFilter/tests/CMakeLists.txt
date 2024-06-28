cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

file(GLOB TESTS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp)
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp)

set(GTEST_GIT_URL "https://github.com/google/googletest.git")
include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY ${GTEST_GIT_URL}
        GIT_TAG release-1.12.0  # Optionally pin to a stable release
)

FetchContent_MakeAvailable(googletest)

if(RUN_COVERAGE)
    message("Coverage enabled!")
    find_program(LCOV_PATH lcov)
    if(NOT LCOV_PATH)
        message(FATAL_ERROR "lcov not found! Aborting")
    endif ()

    find_program(GCOV_PATH gcov)
    if(NOT GCOV_PATH)
        message(FATAL_ERROR "gcov not found! Aborting")
    endif ()

    find_program(GENHTML_PATH genhtml)
    if(NOT GENHTML_PATH)
        message(FATAL_ERROR "genhtml not found! Aborting")
    endif ()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fprofile-arcs -ftest-coverage --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov --coverage")
endif ()

add_executable(test_${PROJECT_NAME} ${SRC_FILES} ${TESTS_FILES})

target_link_libraries(test_${PROJECT_NAME} ${GDAL_LIBRARIES} ${OpenCV_LIBS} gtest gtest_main)

add_test(NAME test_${PROJECT_NAME} COMMAND test_${PROJECT_NAME})

