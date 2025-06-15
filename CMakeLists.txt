cmake_minimum_required(VERSION 3.29)

# Include system architecture detection
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SystemArchitectureConfig.cmake)

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

project(WinKernelLite 
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  LANGUAGES C CXX)

message(STATUS "WinKernelLite version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4)    # Warning level 4
endif()

# Enable Visual Studio folder organization
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
    "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelLite/Version.h"
    @ONLY
)

# Create header-only library
add_library(WinKernelLite INTERFACE)
target_sources(WinKernelLite 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/KernelHeapAlloc.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/LinkedList.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/UnicodeString.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/UnicodeStringUtils.h>
)

# Set up include directories
# - For build tree, we need the original include directory and the generated include directory
# - For install tree, we need the installed include directory
target_include_directories(WinKernelLite INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Create an alias target for use in the build tree
add_library(WinKernelLite::WinKernelLite ALIAS WinKernelLite)

# Organize headers in Visual Studio solution
source_group(
    TREE "${CMAKE_SOURCE_DIR}/include"
    PREFIX "Include Files"
    FILES ${HEADER_FILES}
)

# Testing Configuration
include(FetchContent)
enable_testing()

set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/extern)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of Google Test" FORCE)

# Save the original install prefix
set(ORIGINAL_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/googletest
)

FetchContent_MakeAvailable(googletest)

# Reset the install prefix to its original value
set(CMAKE_INSTALL_PREFIX ${ORIGINAL_CMAKE_INSTALL_PREFIX})

# Configure OpenCppCoverage
option(ENABLE_COVERAGE "Enable code coverage" ON)
if(ENABLE_COVERAGE)
    # Try to find OpenCppCoverage in various possible locations
    set(OPENCPPCOVERAGE_SEARCH_PATHS
        "$ENV{ProgramFiles}/OpenCppCoverage"
        "$ENV{ProgramFiles\(x86\)}/OpenCppCoverage"
        "$ENV{ProgramW6432}/OpenCppCoverage"
        "$ENV{LOCALAPPDATA}/Programs/OpenCppCoverage"
        "$ENV{ChocolateyInstall}/lib/opencppcoverage/tools"
    )

    # First check if it's specified in the environment
    if(DEFINED ENV{OPENCPPCOVERAGE_PATH})
        set(OpenCppCoverage_EXECUTABLE "$ENV{OPENCPPCOVERAGE_PATH}")
    else()
        # Search in the predefined paths
        foreach(SEARCH_PATH ${OPENCPPCOVERAGE_SEARCH_PATHS})
            if(EXISTS "${SEARCH_PATH}/OpenCppCoverage.exe")
                set(OpenCppCoverage_EXECUTABLE "${SEARCH_PATH}/OpenCppCoverage.exe")
                break()
            endif()
        endforeach()

        # If not found in predefined paths, try to find it in PATH
        if(NOT DEFINED OpenCppCoverage_EXECUTABLE)
            find_program(OpenCppCoverage_EXECUTABLE "OpenCppCoverage.exe")
        endif()
    endif()

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
option(INSTALL_EXAMPLES "Install example programs" OFF)

if(BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    # Pass the INSTALL_EXAMPLES option to the examples subdirectory
    add_subdirectory(examples)
endif()

# Package and Installation Configuration
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# Install header files to include/WinKernelLite/
install(DIRECTORY include/ 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/WinKernelLite
    COMPONENT WinKernelLite
    FILES_MATCHING PATTERN "*.h"
)

# Install generated version header
install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelLite/Version.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/WinKernelLite
    COMPONENT WinKernelLite
)

# Install targets with proper include directory path
install(TARGETS WinKernelLite
    EXPORT WinKernelLiteTargets
    COMPONENT WinKernelLite
)

# For examples, we want to make the headers available during the build
if(BUILD_EXAMPLES)
    # Export the include directories to the examples
    set(WinKernelLite_INCLUDE_DIRS 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
    
    # Also make sure examples can find WinKernelLite by creating an explicit directory structure
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include/WinKernelLite")
    
    # Copy headers to the build directory to create the expected structure
    file(GLOB WKL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
    foreach(HEADER ${WKL_HEADERS})
        get_filename_component(HEADER_NAME ${HEADER} NAME)
        configure_file(${HEADER} "${CMAKE_BINARY_DIR}/include/WinKernelLite/${HEADER_NAME}" COPYONLY)
    endforeach()
    
    # Only set to parent scope if this is not the top-level CMakeLists.txt
    if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(WinKernelLite_INCLUDE_DIRS ${WinKernelLite_INCLUDE_DIRS} PARENT_SCOPE)
    endif()
endif()

# Export the targets to both build and install directories
export(EXPORT WinKernelLiteTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteTargets.cmake"
    NAMESPACE WinKernelLite::
)

# Install the export targets
install(EXPORT WinKernelLiteTargets
    FILE WinKernelLiteTargets.cmake
    NAMESPACE WinKernelLite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinKernelLite
    COMPONENT WinKernelLite
)

# Add a custom install target that only installs WinKernelLite components
add_custom_target(install_WinKernelLite
    COMMAND ${CMAKE_COMMAND} 
        -DCMAKE_INSTALL_COMPONENT=WinKernelLite
        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMENT "Installing WinKernelLite components only"
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

