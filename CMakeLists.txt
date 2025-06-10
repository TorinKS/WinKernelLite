cmake_minimum_required(VERSION 3.29)

# Git version extraction
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    ERROR_VARIABLE GIT_ERROR
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT GIT_RESULT EQUAL 0)
    message(STATUS "Git tag not found, using default version: 1.0.0")
    set(GIT_TAG "1.0.0")
  else()
    message(STATUS "Git tag found: ${GIT_TAG}")
    # Remove 'v' prefix if present
    string(REGEX REPLACE "^v" "" GIT_TAG ${GIT_TAG})
  endif()
else()
  message(STATUS "Git not found, using default version: 1.0.0")
  set(GIT_TAG "1.0.0")
endif()

# Split the version string into components
string(REPLACE "." ";" VERSION_LIST ${GIT_TAG})
list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(LENGTH VERSION_LIST VERSION_LIST_LENGTH)
if(VERSION_LIST_LENGTH GREATER 2)
  list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)
else()
  set(PROJECT_VERSION_PATCH 0)
endif()

project(WinKernelSimLite 
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  LANGUAGES C CXX)

message(STATUS "WinKernelSimLite version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4)    # Warning level 4
endif()

# Set Visual Studio folder organization
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# List header files explicitly
set(HEADER_FILES
    include/KernelHeapAlloc.h
    include/LinkedList.h
    include/UnicodeString.h
    include/UnicodeStringUtils.h
)

# Configure version header file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelSimLite/Version.h"
    @ONLY
)

# Create header-only library
add_library(WinKernelSimLite INTERFACE)
target_sources(WinKernelSimLite 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/KernelHeapAlloc.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/LinkedList.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/UnicodeString.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/UnicodeStringUtils.h>
)

# Set up include directories properly
# - For build tree, we need the original include directory and the generated include directory
# - For install tree, we need the installed include directory
target_include_directories(WinKernelSimLite INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Create an alias target for use in the build tree
add_library(WinKernelSimLite::WinKernelSimLite ALIAS WinKernelSimLite)

# Organize headers in Visual Studio solution
source_group(
    TREE "${CMAKE_SOURCE_DIR}/include"
    PREFIX "Include Files"
    FILES ${HEADER_FILES}
)

# Testing Configuration
include(FetchContent)
enable_testing()

# Set external dependencies directory
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/extern)

# Fetch and configure GoogleTest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)
# Prevent GoogleTest from being installed
set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of Google Test" FORCE)

# Save the original install prefix
set(ORIGINAL_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/googletest
)

# Make GTest available
FetchContent_MakeAvailable(googletest)

# Reset the install prefix to its original value
set(CMAKE_INSTALL_PREFIX ${ORIGINAL_CMAKE_INSTALL_PREFIX})

# Configure OpenCppCoverage
option(ENABLE_COVERAGE "Enable code coverage" ON)
if(ENABLE_COVERAGE)
    set(OpenCppCoverage_EXECUTABLE "C:/Program Files/OpenCppCoverage/OpenCppCoverage.exe")

    if(OpenCppCoverage_EXECUTABLE)
        message(STATUS "Found OpenCppCoverage: ${OpenCppCoverage_EXECUTABLE}")
        
        # Convert paths to Windows format and ensure proper escaping for OpenCppCoverage
        string(REPLACE "/" "\\" SOURCES_DIR "${CMAKE_SOURCE_DIR}/include")
        string(REPLACE "/" "\\" COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
        string(REPLACE "/" "\\" OPENCPPCOVERAGE_PATH "${OpenCppCoverage_EXECUTABLE}")
          add_custom_target(coverage
            COMMAND ${OPENCPPCOVERAGE_PATH}
                --export_type=html:${COVERAGE_DIR}
                --sources=${SOURCES_DIR}
                --verbose
                --cover_children
                --
                "$<TARGET_FILE:runTests>" || (exit 0)
            DEPENDS runTests
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            COMMENT "Running code coverage analysis..."
            VERBATIM
        )
    else()
        message(WARNING "OpenCppCoverage not found. Please ensure it is installed and in your PATH.")
    endif()
endif()

# Configure test executable
set(TEST_SOURCES
    tests/test_linked_list.cpp
    tests/test_unicode_string.cpp
    tests/test_unicode_string_utils.cpp
    tests/test_kernel_heap_alloc.cpp
)

add_executable(runTests ${TEST_SOURCES})
target_link_libraries(runTests PRIVATE gtest_main gtest WinKernelLite)
set_target_properties(runTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# Organize test files in Visual Studio solution
source_group(
    TREE "${CMAKE_SOURCE_DIR}/tests"
    PREFIX "Test Files"
    FILES ${TEST_SOURCES}
)

add_test(NAME runTests COMMAND runTests)

# Examples
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# Package and Installation Configuration
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# Install header files to include/WinKernelSimLite/
install(DIRECTORY include/ 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/WinKernelSimLite
    COMPONENT WinKernelSimLite
    FILES_MATCHING PATTERN "*.h"
)

# Install generated version header
install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelSimLite/Version.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/WinKernelSimLite
    COMPONENT WinKernelSimLite
)

# Install targets with proper include directory path
install(TARGETS WinKernelSimLite
    EXPORT WinKernelSimLiteTargets
    COMPONENT WinKernelSimLite
)

# Export the targets
install(EXPORT WinKernelSimLiteTargets
    FILE WinKernelSimLiteTargets.cmake
    NAMESPACE WinKernelSimLite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinKernelSimLite
    COMPONENT WinKernelSimLite
)

# Add a custom install target that only installs WinKernelSimLite components
add_custom_target(install_winkernelsimlite
    COMMAND ${CMAKE_COMMAND} 
        -DCMAKE_INSTALL_COMPONENT=WinKernelSimLite
        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMENT "Installing WinKernelSimLite components only"
)

# Generate the version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate the configuration file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WinKernelLiteConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinKernelLite
)

# Install the config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinKernelLite
    COMPONENT WinKernelLite
)

