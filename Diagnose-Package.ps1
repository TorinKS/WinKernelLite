# CMake Package Diagnostic Tool
# This script helps diagnose issues with CMake package configuration

# Save the current directory
$originalDirectory = Get-Location

Write-Host "WinKernelLite CMake Package Diagnostic Tool" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host

# Check the installation directory
$installDir = "C:\Program Files (x86)\WinKernelLite"
Write-Host "Checking installation directory: $installDir" -ForegroundColor Yellow
if (Test-Path $installDir) {
    Write-Host "✓ Installation directory exists" -ForegroundColor Green
    
    # Check include directory
    $includeDir = Join-Path $installDir "include"
    if (Test-Path $includeDir) {
        Write-Host "✓ Include directory exists: $includeDir" -ForegroundColor Green
        
        # Check specific headers
        $headers = @(
            "WinKernelLite\KernelHeapAlloc.h",
            "WinKernelLite\LinkedList.h",
            "WinKernelLite\UnicodeString.h",
            "WinKernelLite\UnicodeStringUtils.h",
            "WinKernelLite\Version.h"
        )
        
        foreach ($header in $headers) {
            $headerPath = Join-Path $includeDir $header
            if (Test-Path $headerPath) {
                Write-Host "  ✓ Header exists: $header" -ForegroundColor Green
            } else {
                Write-Host "  ✗ Header missing: $header" -ForegroundColor Red
            }
        }
    } else {
        Write-Host "✗ Include directory missing" -ForegroundColor Red
    }
    
    # Check CMake config directory
    $cmakeDir = Join-Path $installDir "lib\cmake\WinKernelLite"
    if (Test-Path $cmakeDir) {
        Write-Host "✓ CMake config directory exists: $cmakeDir" -ForegroundColor Green
        
        # Check config files
        $configFiles = @(
            "WinKernelLiteConfig.cmake",
            "WinKernelLiteConfigVersion.cmake",
            "WinKernelLiteTargets.cmake"
        )
        
        foreach ($file in $configFiles) {
            $filePath = Join-Path $cmakeDir $file
            if (Test-Path $filePath) {
                Write-Host "  ✓ Config file exists: $file" -ForegroundColor Green
                
                # Check for the problematic "COMPONENT" string in the Targets file
                if ($file -eq "WinKernelLiteTargets.cmake") {
                    $content = Get-Content $filePath -Raw
                    if ($content -match "COMPONENT") {
                        Write-Host "    ✗ Found 'COMPONENT' in targets file - this may cause problems" -ForegroundColor Red
                    } else {
                        Write-Host "    ✓ No 'COMPONENT' string in targets file" -ForegroundColor Green
                    }
                }
            } else {
                Write-Host "  ✗ Config file missing: $file" -ForegroundColor Red
            }
        }
    } else {
        Write-Host "✗ CMake config directory missing" -ForegroundColor Red
    }
} else {
    Write-Host "✗ Installation directory missing" -ForegroundColor Red
}

Write-Host
Write-Host "Testing CMake Find Package..." -ForegroundColor Yellow

# Create a temporary test project
$tempDir = Join-Path $env:TEMP "WinKernelLiteTest"
if (Test-Path $tempDir) {
    Remove-Item -Path $tempDir -Recurse -Force
}
New-Item -ItemType Directory -Path $tempDir | Out-Null

# Create a simple CMakeLists.txt
$cmakeContent = @"
cmake_minimum_required(VERSION 3.15)
project(WinKernelLiteTest)

message(STATUS "Testing find_package for WinKernelLite")
find_package(WinKernelLite REQUIRED)

message(STATUS "Found WinKernelLite: \${WinKernelLite_FOUND}")
message(STATUS "WinKernelLite_INCLUDE_DIR: \${WinKernelLite_INCLUDE_DIR}")

get_target_property(INCLUDE_DIRS WinKernelLite::WinKernelLite INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "WinKernelLite include dirs: \${INCLUDE_DIRS}")

add_executable(test_app main.cpp)
target_link_libraries(test_app PRIVATE WinKernelLite::WinKernelLite)
"@

$mainContent = @"
#include <stdio.h>
#include <WinKernelLite/Version.h>

int main() {
    printf("WinKernelLite Version: %s\n", WINKERNELLITE_VERSION);
    return 0;
}
"@

Set-Content -Path (Join-Path $tempDir "CMakeLists.txt") -Value $cmakeContent
Set-Content -Path (Join-Path $tempDir "main.cpp") -Value $mainContent

# Run CMake configure
Set-Location $tempDir
Write-Host "Running CMake in $tempDir..." -ForegroundColor Yellow
cmake -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/WinKernelLite" -B build 2>&1 | ForEach-Object { "$_" }

# Check if configuration was successful
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ CMake configuration successful" -ForegroundColor Green
    
    # Try building the test project
    Write-Host "Building test project..." -ForegroundColor Yellow
    cmake --build build 2>&1 | ForEach-Object { "$_" }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Build successful" -ForegroundColor Green
    } else {
        Write-Host "✗ Build failed" -ForegroundColor Red
    }
} else {
    Write-Host "✗ CMake configuration failed" -ForegroundColor Red
}

# Restore original directory
Set-Location $originalDirectory

Write-Host
Write-Host "Diagnostic complete." -ForegroundColor Cyan
Write-Host "If you're still experiencing issues, make sure to check the CMakeLists.txt file for errors."
Write-Host "You can reinstall the library with: cmake --install build --component WinKernelLite"
