cmake_minimum_required(VERSION 3.29)

# Add a CMake option to make versioning optional
option(REQUIRE_GIT_TAG "Require Git tag for versioning" OFF)

find_package(Git QUIET)
if(GIT_FOUND AND REQUIRE_GIT_TAG)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    ERROR_VARIABLE GIT_ERROR
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT GIT_RESULT EQUAL 0)
    message(FATAL_ERROR "Git tag not found. Please create a Git tag for versioning.")
  else()
    message(STATUS "Git tag found: ${GIT_TAG}")
    string(REGEX REPLACE "^v" "" GIT_TAG ${GIT_TAG})
  endif()
elseif(GIT_FOUND)
  # Try to get tag, but use fallback if not found
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    ERROR_VARIABLE GIT_ERROR
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT GIT_RESULT EQUAL 0)
    message(WARNING "Git tag not found. Using default version 0.1.0")
    set(GIT_TAG "0.1.0")
  else()
    message(STATUS "Git tag found: ${GIT_TAG}")
    string(REGEX REPLACE "^v" "" GIT_TAG ${GIT_TAG})
  endif()
else()
  message(WARNING "Git not found. Using default version 0.1.0")
  set(GIT_TAG "0.1.0")
endif()

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

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(WIN32)
    set(CMAKE_INSTALL_PREFIX "$ENV{ProgramFiles}/WinKernelLite" CACHE PATH "Installation directory" FORCE)
  endif()
endif()

message(STATUS "Installation directory: ${CMAKE_INSTALL_PREFIX}")

include(ProcessorCount)
ProcessorCount(NUM_PROCESSORS)
if(NUM_PROCESSORS EQUAL 0)
    set(NUM_PROCESSORS 8)
endif()
set(CMAKE_BUILD_PARALLEL_LEVEL ${NUM_PROCESSORS})
set(CMAKE_JOB_POOLS compile=${NUM_PROCESSORS} link=2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Automatically discover header and source files
file(GLOB HEADER_FILES "include/*.h" "include/*.h.in")
file(GLOB SOURCE_FILES "src/*.c" "src/*.cpp")

add_custom_target(HeaderFiles SOURCES ${HEADER_FILES})
set_target_properties(HeaderFiles PROPERTIES FOLDER "Core")

add_custom_target(SourceFiles SOURCES ${SOURCE_FILES})
set_target_properties(SourceFiles PROPERTIES FOLDER "Core")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelLite/Version.h"
    @ONLY
)

add_library(WinKernelLite STATIC ${SOURCE_FILES})

# Filter out .h.in files for public interface (only include actual .h files)
file(GLOB PUBLIC_HEADER_FILES "include/*.h")
foreach(HEADER ${PUBLIC_HEADER_FILES})
    target_sources(WinKernelLite PUBLIC 
        $<BUILD_INTERFACE:${HEADER}>
    )
endforeach()

target_include_directories(WinKernelLite PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelLite")

file(GLOB WKL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
foreach(HEADER ${WKL_HEADERS})
    get_filename_component(HEADER_NAME ${HEADER} NAME)
    configure_file(${HEADER} "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelLite/${HEADER_NAME}" COPYONLY)
endforeach()

add_library(WinKernelLite::WinKernelLite ALIAS WinKernelLite)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
      PREFIX "Header Files"
      FILES ${HEADER_FILES}
  )
else()
  source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}"
      PREFIX "WinKernelLite"
      FILES ${HEADER_FILES}
  )
endif()

include(FetchContent)
include(CTest)
enable_testing()

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)

set(ORIGINAL_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/googletest
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of Google Test" FORCE)

FetchContent_MakeAvailable(googletest)

set(CMAKE_INSTALL_PREFIX ${ORIGINAL_CMAKE_INSTALL_PREFIX})

option(ENABLE_COVERAGE "Enable code coverage" ON)
if(ENABLE_COVERAGE)
    set(OPENCPPCOVERAGE_SEARCH_PATHS
        "$ENV{ProgramFiles}/OpenCppCoverage"
        "$ENV{ProgramFiles\(x86\)}/OpenCppCoverage"
        "$ENV{ProgramW6432}/OpenCppCoverage"
        "$ENV{LOCALAPPDATA}/Programs/OpenCppCoverage"
        "$ENV{ChocolateyInstall}/lib/opencppcoverage/tools"
    )

    if(DEFINED ENV{OPENCPPCOVERAGE_PATH})
        set(OpenCppCoverage_EXECUTABLE "$ENV{OPENCPPCOVERAGE_PATH}")
    else()
        foreach(SEARCH_PATH ${OPENCPPCOVERAGE_SEARCH_PATHS})
            if(EXISTS "${SEARCH_PATH}/OpenCppCoverage.exe")
                set(OpenCppCoverage_EXECUTABLE "${SEARCH_PATH}/OpenCppCoverage.exe")
                break()
            endif()
        endforeach()

        if(NOT DEFINED OpenCppCoverage_EXECUTABLE)
            find_program(OpenCppCoverage_EXECUTABLE "OpenCppCoverage.exe")
        endif()
    endif()

    if(OpenCppCoverage_EXECUTABLE)
        message(STATUS "Found OpenCppCoverage: ${OpenCppCoverage_EXECUTABLE}")
        string(REPLACE "/" "\\" SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
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

# Automatically discover test files (exclude backup, experimental, and duplicate files)
file(GLOB ALL_TEST_FILES "tests/*.cpp")
set(TEST_SOURCES)
foreach(TEST_FILE ${ALL_TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME)
    # Exclude files that are backups, experiments, or duplicates
    if(NOT TEST_NAME MATCHES ".*(fixed|minimal|backup|bak|experiment|duplicate|temp|old).*\\.cpp$")
        list(APPEND TEST_SOURCES ${TEST_FILE})
    endif()
endforeach()

# Discover test header files for Visual Studio project visibility
file(GLOB TEST_HEADER_FILES "tests/*.h")

add_executable(runTests ${TEST_SOURCES} ${TEST_HEADER_FILES})
target_link_libraries(runTests PRIVATE gtest_main gtest WinKernelLite)

# Link Windows Socket library for inet_pton and other network functions
if(WIN32)
    target_link_libraries(runTests PRIVATE ws2_32)
endif()


set_target_properties(runTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests"
      PREFIX "Test Files"
      FILES ${TEST_SOURCES} ${TEST_HEADER_FILES}
  )
else()
  source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}"
      PREFIX "WinKernelLite/Tests"
      FILES ${TEST_SOURCES} ${TEST_HEADER_FILES}
  )
endif()

set_target_properties(runTests PROPERTIES FOLDER "Tests")

add_test(NAME runTests COMMAND runTests)

option(BUILD_EXAMPLES "Build example programs" OFF)
option(INSTALL_EXAMPLES "Install example programs" OFF)

add_custom_target(build_examples
    COMMAND ${CMAKE_COMMAND} -DBUILD_EXAMPLES=ON -S ${CMAKE_CURRENT_SOURCE_DIR}/examples -B ${CMAKE_CURRENT_BINARY_DIR}/examples
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/examples --config $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building examples using installed WinKernelLite package"
    VERBATIM
)

add_dependencies(build_examples install_WinKernelLite)

message(STATUS "Examples are configured as a separate build target.")
message(STATUS "Run 'cmake --build build --target install_WinKernelLite' first, then 'cmake --build build --target build_examples'")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(DIRECTORY include/ 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/WinKernelLite
    COMPONENT WinKernelLite
    FILES_MATCHING PATTERN "*.h"
)

install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/include/WinKernelLite/Version.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/WinKernelLite
    COMPONENT WinKernelLite
)

install(TARGETS WinKernelLite
    EXPORT WinKernelLiteTargets
    COMPONENT WinKernelLite
)

export(EXPORT WinKernelLiteTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteTargets.cmake"
    NAMESPACE WinKernelLite::
)

install(EXPORT WinKernelLiteTargets
    FILE WinKernelLiteTargets.cmake
    NAMESPACE WinKernelLite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinKernelLite
    COMPONENT WinKernelLite
)

add_custom_target(install_WinKernelLite
    COMMAND ${CMAKE_COMMAND} 
        -DCMAKE_INSTALL_COMPONENT=WinKernelLite
        -DCMAKE_INSTALL_CONFIG_NAME=$<CONFIG>
        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMENT "Installing WinKernelLite components only"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WinKernelLiteConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinKernelLite
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/WinKernelLiteConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinKernelLite
    COMPONENT WinKernelLite
)
