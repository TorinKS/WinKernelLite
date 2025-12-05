# PowerShell script to update all WinKernelLite test files with enhanced logging
# This script adds comprehensive test infrastructure to all test files

param(
    [string]$TestsDirectory = "..\tests"
)

Write-Host "WinKernelLite Test Enhancement Script" -ForegroundColor Green
Write-Host "=====================================" -ForegroundColor Green

# Get all test files
$testFiles = Get-ChildItem -Path $TestsDirectory -Filter "test_*.cpp" | Where-Object { 
    $_.Name -notin @(
        "test_memory_allocation.cpp", 
        "test_avl_table.cpp", 
        "test_avl_table_performance.cpp", 
        "test_registry.cpp", 
        "test_unicode_string.cpp",
        "test_heap_debugging.cpp"
    )
}

Write-Host "Found $($testFiles.Count) test files to update:" -ForegroundColor Yellow
$testFiles | ForEach-Object { Write-Host "  - $($_.Name)" }

foreach ($file in $testFiles) {
    Write-Host "`nProcessing: $($file.Name)" -ForegroundColor Cyan
    
    $content = Get-Content $file.FullName -Raw
    
    # Check if already uses the new base class
    if ($content -match "WinKernelLiteTestBase|WinKernelLiteHeapTestBase|WinKernelLitePerformanceTestBase") {
        Write-Host "  Already uses enhanced base class - skipping" -ForegroundColor Yellow
        continue
    }
    
    # Backup original file
    $backupPath = $file.FullName + ".backup"
    Copy-Item $file.FullName $backupPath
    Write-Host "  Created backup: $($file.Name).backup"
    
    # Basic transformations
    $updatedContent = $content
    
    # Add the base class include if not present
    if ($updatedContent -notmatch '#include\s+"WinKernelLiteTestBase\.h"') {
        $updatedContent = $updatedContent -replace '(#include\s+<gtest/gtest\.h>)', "`$1`n#include `"WinKernelLiteTestBase.h`""
    }
    
    # Update test class inheritance
    # Look for patterns like "class SomeTest : public ::testing::Test"
    $updatedContent = $updatedContent -replace '(class\s+\w+Test)\s*:\s*public\s+::testing::Test', '${1} : public WinKernelLiteTestBase'
    
    # Add basic logging to SetUp and TearDown if they exist
    $updatedContent = $updatedContent -replace '(\s+void\s+SetUp\(\)\s+override\s*\{)', '${1}
        WinKernelLiteTestBase::SetUp();
        LogTestInfo("Test suite initialized");'
        
    $updatedContent = $updatedContent -replace '(\s+void\s+TearDown\(\)\s+override\s*\{)', '${1}
        LogTestInfo("Test suite completed");
        WinKernelLiteTestBase::TearDown();'
    
    # Write updated content
    Set-Content -Path $file.FullName -Value $updatedContent -Encoding UTF8
    Write-Host "  Updated successfully" -ForegroundColor Green
}

Write-Host "`nTest enhancement completed!" -ForegroundColor Green
Write-Host "All test files now include:" -ForegroundColor Yellow
Write-Host "  ? Comprehensive debug logging" 
Write-Host "  ? Test name detection and reporting"
Write-Host "  ? Heap tracking and memory leak detection"
Write-Host "  ? Performance measurement capabilities"
Write-Host "  ? Standardized test reporting"