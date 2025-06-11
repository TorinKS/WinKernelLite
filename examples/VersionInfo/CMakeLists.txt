# CMake configuration for examples
cmake_minimum_required(VERSION 3.15)
project(WinKernelLiteExamples)

# Debug information
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Find the WinKernelLite package
find_package(WinKernelLite REQUIRED)

# Debug include paths
get_target_property(WINKERNELLITE_INCLUDE_DIRS WinKernelLite::WinKernelLite INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "WinKernelLite include directories: ${WINKERNELLITE_INCLUDE_DIRS}")

# Version info example
add_executable(version_info_extended version_info.c)
target_link_libraries(version_info_extended PRIVATE WinKernelLite::WinKernelLite)

# Set output directory
set_target_properties(version_info_extended PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install the example only if INSTALL_EXAMPLES is ON
if(INSTALL_EXAMPLES)
    install(TARGETS version_info_extended
        CONFIGURATIONS Debug Release
        RUNTIME DESTINATION bin/examples
        COMPONENT Examples
    )
endif()
