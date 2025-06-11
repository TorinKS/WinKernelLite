cmake_minimum_required(VERSION 3.15)
project(DevicesExample C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include debugging script
include(${CMAKE_CURRENT_SOURCE_DIR}/../debug_includes.cmake)

# Find WinKernelLite package
# Look in standard locations and the install directory
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};C:/Program Files (x86)/WinKernelLite")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/..")
find_package(WinKernelLite REQUIRED)

# Print debugging information
message(STATUS "Found WinKernelLite: ${WinKernelLite_FOUND}")
get_target_property(WKL_INCLUDE_DIRS WinKernelLite::WinKernelLite INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "WinKernelLite include directories: ${WKL_INCLUDE_DIRS}")

# Add fallback include directories if needed
if(NOT WKL_INCLUDE_DIRS)
    message(STATUS "Adding fallback include directories")
    include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../build/include"
        "C:/Program Files (x86)/WinKernelLite/include"
    )
endif()

# Create the example executable
add_executable(devices_example DevicesExample.c)

# Link against WinKernelLite
target_link_libraries(devices_example PRIVATE WinKernelLite::WinKernelLite)

# Add ntdll.lib dependency for Windows NT functions
if(MSVC)
    target_link_libraries(devices_example PRIVATE ntdll)
endif()

# Set output directory
set_target_properties(devices_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install the example only if INSTALL_EXAMPLES is ON
if(INSTALL_EXAMPLES)
    install(TARGETS devices_example
        RUNTIME DESTINATION bin/examples
        COMPONENT Examples
    )
endif()
