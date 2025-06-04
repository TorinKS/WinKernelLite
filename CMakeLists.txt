cmake_minimum_required(VERSION 3.29)
project(WinKernelLite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

option(BUILD_TESTS "Sets option to build the unit tests, default on" ON)

# Add include directory for headers
include_directories(${CMAKE_SOURCE_DIR}/include)

if(BUILD_TESTS)
    enable_testing()
    
    # Fetch and configure GoogleTest
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)

    # Configure test executable
    file(GLOB TEST_SOURCES tests/*.cpp)
    add_executable(runTests ${TEST_SOURCES})
    target_link_libraries(runTests PRIVATE gtest_main gtest)
    
    add_test(NAME runTests COMMAND runTests)
endif()

