cmake_minimum_required(VERSION 3.29)
project(DeviceListExample C)

# Set C standard (consistent with main project)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

# Add executable
add_executable(devices_list 
    DeviceListApp.c
    DevicesList.c
    DevicesList.h
)

# Link with WinKernelLite
target_link_libraries(devices_list PRIVATE WinKernelLite::WinKernelLite)

# Add ntdll.lib dependency for Windows NT functions
if(MSVC)
    target_link_libraries(devices_list PRIVATE ntdll)
endif()
