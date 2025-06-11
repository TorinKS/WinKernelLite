# Complete Diagnostic Script for WinKernelLite
# This script performs a comprehensive check of the build system

$ErrorActionPreference = "Continue"

Write-Host "WinKernelLite Build System Diagnostic" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

# Check environment
Write-Host "1. Checking environment..." -ForegroundColor Green
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    Write-Host "  ✓ CMake found: $($cmake.Version)" -ForegroundColor Green
} else {
    Write-Host "  ✗ CMake not found in PATH" -ForegroundColor Red
}

$cl = Get-Command cl -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "  ✓ MSVC compiler found" -ForegroundColor Green
} else {
    Write-Host "  ✗ MSVC compiler not found in PATH" -ForegroundColor Red
    Write-Host "    Try running from a Visual Studio Developer Command Prompt" -ForegroundColor Yellow
}

# Check installation
Write-Host ""
Write-Host "2. Checking installation..." -ForegroundColor Green
$installDir = "C:\Program Files (x86)\WinKernelLite"
if (Test-Path $installDir) {
    Write-Host "  ✓ Installation directory found: $installDir" -ForegroundColor Green
    
    # Check for include directory
    $includeDir = Join-Path $installDir "include"
    if (Test-Path $includeDir) {
        Write-Host "  ✓ Include directory found: $includeDir" -ForegroundColor Green
        
        # Check for WinKernelLite subdirectory
        $wklIncludeDir = Join-Path $includeDir "WinKernelLite"
        if (Test-Path $wklIncludeDir) {
            Write-Host "  ✓ WinKernelLite include subdirectory found: $wklIncludeDir" -ForegroundColor Green
            
            # Check for header files
            $headers = Get-ChildItem $wklIncludeDir -Filter "*.h"
            if ($headers.Count -gt 0) {
                Write-Host "  ✓ Found $($headers.Count) header files" -ForegroundColor Green
                foreach ($header in $headers) {
                    Write-Host "    - $($header.Name)" -ForegroundColor Gray
                }
            } else {
                Write-Host "  ✗ No header files found in $wklIncludeDir" -ForegroundColor Red
            }
        } else {
            Write-Host "  ✗ WinKernelLite include subdirectory not found" -ForegroundColor Red
        }
    } else {
        Write-Host "  ✗ Include directory not found" -ForegroundColor Red
    }
    
    # Check for CMake files
    $cmakeDir = Join-Path $installDir "lib\cmake\WinKernelLite"
    if (Test-Path $cmakeDir) {
        Write-Host "  ✓ CMake configuration directory found: $cmakeDir" -ForegroundColor Green
        
        # Check for config files
        $configFile = Join-Path $cmakeDir "WinKernelLiteConfig.cmake"
        if (Test-Path $configFile) {
            Write-Host "  ✓ WinKernelLiteConfig.cmake found" -ForegroundColor Green
        } else {
            Write-Host "  ✗ WinKernelLiteConfig.cmake not found" -ForegroundColor Red
        }
        
        $versionFile = Join-Path $cmakeDir "WinKernelLiteConfigVersion.cmake"
        if (Test-Path $versionFile) {
            Write-Host "  ✓ WinKernelLiteConfigVersion.cmake found" -ForegroundColor Green
        } else {
            Write-Host "  ✗ WinKernelLiteConfigVersion.cmake not found" -ForegroundColor Red
        }
        
        $targetsFile = Join-Path $cmakeDir "WinKernelLiteTargets.cmake"
        if (Test-Path $targetsFile) {
            Write-Host "  ✓ WinKernelLiteTargets.cmake found" -ForegroundColor Green
        } else {
            Write-Host "  ✗ WinKernelLiteTargets.cmake not found" -ForegroundColor Red
        }
    } else {
        Write-Host "  ✗ CMake configuration directory not found" -ForegroundColor Red
    }
} else {
    Write-Host "  ✗ Installation directory not found: $installDir" -ForegroundColor Red
}

# Check build directory
Write-Host ""
Write-Host "3. Checking build directory..." -ForegroundColor Green
$buildDir = Join-Path (Get-Location) "build"
if (Test-Path $buildDir) {
    Write-Host "  ✓ Build directory found: $buildDir" -ForegroundColor Green
    
    # Check for include directory in build
    $buildIncludeDir = Join-Path $buildDir "include"
    if (Test-Path $buildIncludeDir) {
        Write-Host "  ✓ Include directory found in build: $buildIncludeDir" -ForegroundColor Green
        
        # Check for WinKernelLite subdirectory
        $buildWklIncludeDir = Join-Path $buildIncludeDir "WinKernelLite"
        if (Test-Path $buildWklIncludeDir) {
            Write-Host "  ✓ WinKernelLite include subdirectory found in build: $buildWklIncludeDir" -ForegroundColor Green
            
            # Check for Version.h
            $versionH = Join-Path $buildWklIncludeDir "Version.h"
            if (Test-Path $versionH) {
                Write-Host "  ✓ Version.h found in build" -ForegroundColor Green
            } else {
                Write-Host "  ✗ Version.h not found in build" -ForegroundColor Red
            }
        } else {
            Write-Host "  ✗ WinKernelLite include subdirectory not found in build" -ForegroundColor Red
        }
    } else {
        Write-Host "  ✗ Include directory not found in build" -ForegroundColor Red
    }
} else {
    Write-Host "  ✗ Build directory not found: $buildDir" -ForegroundColor Red
}

# Check examples
Write-Host ""
Write-Host "4. Checking examples..." -ForegroundColor Green
$examplesDir = Join-Path (Get-Location) "examples"
if (Test-Path $examplesDir) {
    Write-Host "  ✓ Examples directory found: $examplesDir" -ForegroundColor Green
    
    # Check for example directories
    $exampleDirs = Get-ChildItem $examplesDir -Directory | Where-Object { $_.Name -ne "CMakeFiles" }
    if ($exampleDirs.Count -gt 0) {
        Write-Host "  ✓ Found $($exampleDirs.Count) example directories" -ForegroundColor Green
        foreach ($dir in $exampleDirs) {
            Write-Host "    - $($dir.Name)" -ForegroundColor Gray
            
            # Check for CMakeLists.txt
            $cmakeListsFile = Join-Path $dir.FullName "CMakeLists.txt"
            if (Test-Path $cmakeListsFile) {
                Write-Host "      ✓ CMakeLists.txt found" -ForegroundColor Green
            } else {
                Write-Host "      ✗ CMakeLists.txt not found" -ForegroundColor Red
            }
            
            # Check for source files
            $sourceFiles = Get-ChildItem $dir.FullName -Filter "*.c"
            if ($sourceFiles.Count -gt 0) {
                Write-Host "      ✓ Found $($sourceFiles.Count) source files" -ForegroundColor Green
            } else {
                Write-Host "      ✗ No source files found" -ForegroundColor Red
            }
            
            # Check for build_and_run.bat
            $buildScript = Join-Path $dir.FullName "build_and_run.bat"
            if (Test-Path $buildScript) {
                Write-Host "      ✓ build_and_run.bat found" -ForegroundColor Green
            } else {
                Write-Host "      ✗ build_and_run.bat not found" -ForegroundColor Red
            }
        }
    } else {
        Write-Host "  ✗ No example directories found" -ForegroundColor Red
    }
    
    # Check for FindWinKernelLite.cmake
    $findModule = Join-Path $examplesDir "FindWinKernelLite.cmake"
    if (Test-Path $findModule) {
        Write-Host "  ✓ FindWinKernelLite.cmake found" -ForegroundColor Green
    } else {
        Write-Host "  ✗ FindWinKernelLite.cmake not found" -ForegroundColor Red
    }
} else {
    Write-Host "  ✗ Examples directory not found: $examplesDir" -ForegroundColor Red
}

# Recommendations
Write-Host ""
Write-Host "5. Recommendations:" -ForegroundColor Green

if (-not (Test-Path $installDir)) {
    Write-Host "  → Install the library first:" -ForegroundColor Yellow
    Write-Host "    mkdir build; cd build; cmake ..; cmake --build . --config Debug; cmake --install . --prefix `"C:/Program Files (x86)/WinKernelLite`"" -ForegroundColor Yellow
}

if (-not (Test-Path (Join-Path $buildDir "include\WinKernelLite"))) {
    Write-Host "  → Set up the environment for examples:" -ForegroundColor Yellow
    Write-Host "    .\setup_examples_env.bat" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Diagnostic complete." -ForegroundColor Cyan
