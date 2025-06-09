# Versioning System

WinKernelLite uses Git tags for automatic versioning. This document explains how the versioning system works and how to use it in your own projects.

## How It Works

1. The CMake build system extracts the latest Git tag when you configure the project.
2. The tag is parsed into major, minor, and patch version components.
3. A `Version.h` header is generated with version macros.
4. The version information is included in the CMake package configuration.

## Setting Up in Your Own Project

To add similar Git-based versioning to your project:

1. Create a `Version.h.in` template file with version macros.
2. Add Git tag extraction code to your CMakeLists.txt.
3. Configure the version header from the template.
4. Include the version header in your build.

## Version Header Template

```c
// Version.h.in
#ifndef PROJECT_VERSION_H
#define PROJECT_VERSION_H

#define PROJECT_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define PROJECT_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define PROJECT_VERSION_PATCH @PROJECT_VERSION_PATCH@
#define PROJECT_VERSION "@PROJECT_VERSION@"
#define PROJECT_GIT_TAG "@GIT_TAG@"

#endif // PROJECT_VERSION_H
```

## CMake Configuration

```cmake
# Find Git
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    ERROR_VARIABLE GIT_ERROR
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT GIT_RESULT EQUAL 0)
    message(STATUS "Git tag not found, using default version")
    set(GIT_TAG "1.0.0")
  else()
    # Remove 'v' prefix if present
    string(REGEX REPLACE "^v" "" GIT_TAG ${GIT_TAG})
  endif()
else()
  set(GIT_TAG "1.0.0")
endif()

# Parse version components
string(REPLACE "." ";" VERSION_LIST ${GIT_TAG})
list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(LENGTH VERSION_LIST VERSION_LIST_LENGTH)
if(VERSION_LIST_LENGTH GREATER 2)
  list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)
else()
  set(PROJECT_VERSION_PATCH 0)
endif()

# Set project version
project(YourProject 
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# Configure version header
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/Version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/Version.h"
  @ONLY
)
```

## Usage in Code

```c
#include <Version.h>

void print_version() {
  printf("Version: %s\n", PROJECT_VERSION);
  printf("Major: %d\n", PROJECT_VERSION_MAJOR);
  printf("Minor: %d\n", PROJECT_VERSION_MINOR);
  printf("Patch: %d\n", PROJECT_VERSION_PATCH);
}
```

## Tagging Workflow

To set a new version:

```bash
# Create a new tag
git tag -a v1.2.3 -m "Version 1.2.3"

# Push the tag
git push origin v1.2.3
```

## Benefits

- Automatic versioning from Git tags
- Consistent version information across code and package
- No need to manually update version numbers in multiple places
- Easy integration with CI/CD pipelines
