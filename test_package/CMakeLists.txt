cmake_minimum_required(VERSION 3.15)
project(WinKernelLiteTest)

# Find WinKernelLite
set(CMAKE_PREFIX_PATH "C:/Program Files (x86)/WinKernelLite")
find_package(WinKernelLite REQUIRED)

# Print debugging information
message(STATUS "Found WinKernelLite: ${WinKernelLite_FOUND}")
get_target_property(WKL_INCLUDE_DIRS WinKernelLite::WinKernelLite INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "WinKernelLite include directories: ${WKL_INCLUDE_DIRS}")

# Create a simple test executable
add_executable(test_version main.c)
target_link_libraries(test_version PRIVATE WinKernelLite::WinKernelLite)
