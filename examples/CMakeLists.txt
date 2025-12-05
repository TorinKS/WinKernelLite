# WinKernelLite Examples
# This file configures the DevicesList and RegistryExample projects

cmake_minimum_required(VERSION 3.29)
project(WinKernelLiteExamples)

# Set common properties for all examples
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable Visual Studio folder organization (consistent with main project)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add MSVC compiler flags (consistent with main project)
if (MSVC)
    add_compile_options(/W4)    # Warning level 4
endif()

# Common output directory for all examples
set(EXAMPLES_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/examples")

# Try to use installed WinKernelLite first, fall back to parent project
# This ensures examples use installed headers when available
set(WinKernelLite_PREFER_INSTALLED ON CACHE BOOL "Prefer installed WinKernelLite package over build tree")

if(NOT TARGET WinKernelLite::WinKernelLite OR WinKernelLite_PREFER_INSTALLED)
    # Look for installed WinKernelLite package first
    find_package(WinKernelLite QUIET)
    
    if(WinKernelLite_FOUND)
        message(STATUS "Using installed WinKernelLite package from: ${WinKernelLite_INCLUDE_DIRS}")
        # Undefine any previous target to use the installed one
        if(TARGET WinKernelLite::WinKernelLite)
            message(STATUS "Preferring installed package over build tree")
        endif()
    elseif(NOT TARGET WinKernelLite::WinKernelLite)
        # If not found installed and no build tree target, check if we're part of the main project build
        if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
            message(STATUS "Using WinKernelLite from parent project")
        else()
            message(FATAL_ERROR "WinKernelLite package not found. Please install WinKernelLite first or build from the main project.")
        endif()
    else()
        message(STATUS "Using WinKernelLite from parent project (installed package not found)")
    endif()
endif()

# DevicesList - C project
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/DevicesList/CMakeLists.txt")
    message(STATUS "Adding DevicesList")
    add_subdirectory(DevicesList)
    
    # Set Visual Studio folder and properties
    if(TARGET devices_list)
        set_target_properties(devices_list PROPERTIES
            FOLDER "Examples"
            RUNTIME_OUTPUT_DIRECTORY "${EXAMPLES_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXAMPLES_OUTPUT_DIR}/Debug"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXAMPLES_OUTPUT_DIR}/Release"
        )
    endif()
endif()

# RegistryExample - C++ project
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/RegistryExample/CMakeLists.txt")
    message(STATUS "Adding RegistryExample")
    add_subdirectory(RegistryExample)
    
    # Set Visual Studio folder and properties for both executables
    if(TARGET registry_example)
        set_target_properties(registry_example PROPERTIES
            FOLDER "Examples"
            RUNTIME_OUTPUT_DIRECTORY "${EXAMPLES_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXAMPLES_OUTPUT_DIR}/Debug"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXAMPLES_OUTPUT_DIR}/Release"
        )
    endif()
    
    if(TARGET registry_simple_test)
        set_target_properties(registry_simple_test PROPERTIES
            FOLDER "Examples"
            RUNTIME_OUTPUT_DIRECTORY "${EXAMPLES_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXAMPLES_OUTPUT_DIR}/Debug"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXAMPLES_OUTPUT_DIR}/Release"
        )
    endif()
endif()

# Create a custom target to build all examples
set(ALL_EXAMPLE_TARGETS)
if(TARGET devices_list)
    list(APPEND ALL_EXAMPLE_TARGETS devices_list)
endif()
if(TARGET registry_example)
    list(APPEND ALL_EXAMPLE_TARGETS registry_example)
endif()
if(TARGET registry_simple_test)
    list(APPEND ALL_EXAMPLE_TARGETS registry_simple_test)
endif()

if(ALL_EXAMPLE_TARGETS)
    add_custom_target(build_all_examples
        DEPENDS ${ALL_EXAMPLE_TARGETS}
        COMMENT "Building WinKernelLite examples: DevicesList and RegistryExample"
    )
    set_target_properties(build_all_examples PROPERTIES FOLDER "Examples")
    
    message(STATUS "Created build_all_examples target with: ${ALL_EXAMPLE_TARGETS}")
endif()

# Optional: Install examples if INSTALL_EXAMPLES is ON
if(INSTALL_EXAMPLES)
    foreach(TARGET_NAME ${ALL_EXAMPLE_TARGETS})
        if(TARGET ${TARGET_NAME})
            install(TARGETS ${TARGET_NAME}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
                COMPONENT Examples
            )
        endif()
    endforeach()
endif() 