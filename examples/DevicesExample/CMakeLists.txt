cmake_minimum_required(VERSION 3.15)
project(DevicesExample C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find WinKernelLite package
find_package(WinKernelLite REQUIRED)

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

# Install the example (optional)
install(TARGETS devices_example
    RUNTIME DESTINATION bin/examples
    COMPONENT Examples
)
