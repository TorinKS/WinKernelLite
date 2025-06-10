cmake_minimum_required(VERSION 3.15)
project(WinKernelLiteExamples C)

# Add examples directory to module path so we can include sub-examples
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# Find the WinKernelLite package
find_package(WinKernelLite REQUIRED)

# Simple version info example
# add_executable(version_info version_info.c)
# target_link_libraries(version_info PRIVATE WinKernelLite::WinKernelLite)

# Add DevicesList example if the directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/DevicesList/CMakeLists.txt")
    add_subdirectory(DevicesList)
endif()

# Add VersionInfo example if the directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo/CMakeLists.txt")
    add_subdirectory(VersionInfo)
endif()

# Add DevicesExample example if the directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/DevicesExample/CMakeLists.txt")
    add_subdirectory(DevicesExample)
endif()

# Set output directory for all examples
# set_target_properties(version_info PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
# )

# Install examples
# install(TARGETS version_info
#     RUNTIME DESTINATION bin/examples
#     COMPONENT Examples
# )

# Add a custom target to build all examples
add_custom_target(build_all_examples ALL
    DEPENDS version_info
    COMMENT "Building all examples"
)

# If any of our examples were added as subdirectories, add dependencies
if(TARGET devices_list)
    add_dependencies(build_all_examples devices_list)
endif()

if(TARGET version_info_extended)
    add_dependencies(build_all_examples version_info_extended)
endif()

if(TARGET devices_example)
    add_dependencies(build_all_examples devices_example)
endif()

# Add custom target to run all examples
add_custom_target(run_all_examples
    COMMAND version_info
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    COMMENT "Running all examples"
    DEPENDS build_all_examples
)

# Conditionally add commands to run other examples
if(TARGET devices_list)
    add_custom_command(TARGET run_all_examples POST_BUILD
        COMMAND devices_list
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    )
endif()

if(TARGET version_info_extended)
    add_custom_command(TARGET run_all_examples POST_BUILD
        COMMAND version_info_extended
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    )
endif()

if(TARGET devices_example)
    add_custom_command(TARGET run_all_examples POST_BUILD
        COMMAND devices_example
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    )
endif()