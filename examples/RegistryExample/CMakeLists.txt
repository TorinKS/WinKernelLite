cmake_minimum_required(VERSION 3.29)
project(RegistryExample CXX)

# Set C++ standard (consistent with main project)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add MSVC compiler flags (consistent with main project)
if (MSVC)
    add_compile_options(/W4)    # Warning level 4
endif()

# Find WinKernelLite package
find_package(WinKernelLite REQUIRED)

# Display package information for debugging
if(WinKernelLite_FOUND)
    message(STATUS "Found WinKernelLite package")
    if(WinKernelLite_INCLUDE_DIRS)
        message(STATUS "WinKernelLite include directories: ${WinKernelLite_INCLUDE_DIRS}")
    endif()
    if(WinKernelLite_VERSION)
        message(STATUS "WinKernelLite version: ${WinKernelLite_VERSION}")
    endif()
else()
    message(FATAL_ERROR "WinKernelLite package not found. Please install WinKernelLite first.")
endif()

# Add executables
add_executable(registry_example registry_example.cpp)
add_executable(registry_simple_test simple_registry_test.cpp)

# Link with WinKernelLite
target_link_libraries(registry_example PRIVATE WinKernelLite::WinKernelLite)
target_link_libraries(registry_simple_test PRIVATE WinKernelLite::WinKernelLite)

# Add ntdll.lib dependency for Windows NT functions
if(MSVC)
    target_link_libraries(registry_example PRIVATE ntdll)
    target_link_libraries(registry_simple_test PRIVATE ntdll)
endif()

# Set output directory
set_target_properties(registry_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/Release"
)

set_target_properties(registry_simple_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/Release"
)
