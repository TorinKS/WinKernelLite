# CMake configuration for examples
cmake_minimum_required(VERSION 3.15)
project(DeviceListApp C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Debug information
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Find the WinKernelLite package
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
add_executable(DeviceListApp ${SOURCE_FILES} ${HEADER_FILES})

# Set target-specific include directories
target_include_directories(DeviceListApp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WINKERNELLITE_INCLUDE_DIRS}
    "C:/Program Files (x86)/WinKernelLite/include"
)

# Link libraries
target_link_libraries(DeviceListApp PRIVATE WinKernelLite::WinKernelLite)

# Set compiler warnings
if(MSVC)
  target_compile_options(DeviceListApp PRIVATE /W4)
else()
  target_compile_options(DeviceListApp PRIVATE -Wall -Wextra -pedantic)
endif()

# Set the headers as properties to ensure they show up in IDEs
set_target_properties(DeviceListApp PROPERTIES
    PUBLIC_HEADER "${HEADER_FILES}"
)

# Install configuration
install(TARGETS DeviceListApp
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/DeviceListApp
)

# Add a custom target for packaging
if(WIN32)
    add_custom_target(package
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/package/bin"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/package/include/DeviceListApp"
        COMMAND ${CMAKE_COMMAND} -E copy 
            "${CMAKE_BINARY_DIR}/Debug/DeviceListApp.exe" 
            "${CMAKE_BINARY_DIR}/package/bin/"
        COMMAND ${CMAKE_COMMAND} -E copy 
            "${CMAKE_CURRENT_SOURCE_DIR}/DevicesList.h" 
            "${CMAKE_BINARY_DIR}/package/include/DeviceListApp/"
        COMMENT "Creating package with executable and header files"
    )
endif()
