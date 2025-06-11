# CMake configuration for examples
cmake_minimum_required(VERSION 3.15)
project(DeviceListExample C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Debug information
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Include debugging script if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../debug_includes.cmake")
    include(${CMAKE_CURRENT_SOURCE_DIR}/../debug_includes.cmake)
endif()

# Find the WinKernelLite package
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};C:/Program Files (x86)/WinKernelLite")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/..")
find_package(WinKernelLite REQUIRED)

# Debug include paths
get_target_property(WINKERNELLITE_INCLUDE_DIRS WinKernelLite::WinKernelLite INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "WinKernelLite include directories: ${WINKERNELLITE_INCLUDE_DIRS}")

# Set include directories properly
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WINKERNELLITE_INCLUDE_DIRS}
    "C:/Program Files (x86)/WinKernelLite/include"
)

# Define header files explicitly
set(HEADER_FILES
    DevicesList.h
)

# Define source files explicitly
set(SOURCE_FILES
    DeviceListApp.c
    DevicesList.c
)

# DeviceListApp - include both source and header files
add_executable(devices_list ${SOURCE_FILES} ${HEADER_FILES})

# Set target-specific include directories
target_include_directories(devices_list PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WINKERNELLITE_INCLUDE_DIRS}
    "C:/Program Files (x86)/WinKernelLite/include"
)

# Link libraries
target_link_libraries(devices_list PRIVATE WinKernelLite::WinKernelLite)

# Set compiler warnings
if(MSVC)
  target_compile_options(devices_list PRIVATE /W4)
else()
  target_compile_options(devices_list PRIVATE -Wall -Wextra -pedantic)
endif()

# Set the headers as properties to ensure they show up in IDEs
set_target_properties(devices_list PROPERTIES
    PUBLIC_HEADER "${HEADER_FILES}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install configuration
install(TARGETS devices_list
    CONFIGURATIONS Debug Release
    RUNTIME DESTINATION bin/examples
    PUBLIC_HEADER DESTINATION include/examples/DevicesList
    COMPONENT Examples
)

# Add a custom target for packaging
if(WIN32)
    add_custom_target(package_devices_list
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/package/bin"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/package/include/examples/DevicesList"
        COMMAND ${CMAKE_COMMAND} -E copy 
            "$<TARGET_FILE:devices_list>" 
            "${CMAKE_BINARY_DIR}/package/bin/"
        COMMAND ${CMAKE_COMMAND} -E copy 
            "${CMAKE_CURRENT_SOURCE_DIR}/DevicesList.h" 
            "${CMAKE_BINARY_DIR}/package/include/examples/DevicesList/"
        COMMENT "Creating package with executable and header files"
    )
endif()
