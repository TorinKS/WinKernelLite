/*
 * Copyright 2025 WinKernelLite Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WINKERNELLITE_TEST_BASE_H
#define WINKERNELLITE_TEST_BASE_H

#include <gtest/gtest.h>
#include <string>
#include <ctime>
#include <chrono>
#include <vector>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdarg>

// WinKernelLite headers
#include <WinKernelLite/Debug.h>
#include <WinKernelLite/KernelHeap.h>

/**
 * @brief Comprehensive base class for all WinKernelLite tests
 * 
 * This class provides complete test infrastructure including:
 * - Automatic test name detection and logging
 * - Debug logging setup with file output
 * - Heap tracking and memory leak detection
 * - Performance measurement and timing
 * - Detailed heap usage analysis
 * - Standardized test reporting
 */
class WinKernelLiteTestBase : public ::testing::Test {
protected:
    void SetUp() override {
        // Record test start time
        test_start_time_ = std::chrono::high_resolution_clock::now();
        
        // Get test information
        const ::testing::TestInfo* const test_info = 
            ::testing::UnitTest::GetInstance()->current_test_info();
        
        if (test_info) {
            test_suite_name_ = test_info->test_case_name();
            test_name_ = test_info->name();
            full_test_name_ = test_suite_name_ + "." + test_name_;
        } else {
            test_suite_name_ = "Unknown";
            test_name_ = "Unknown";
            full_test_name_ = "Unknown.Unknown";
        }
        
        // Initialize debug logging with comprehensive settings
        InitializeDebugLogging();
        
        // Set test name for heap logging correlation
        SetCurrentTestName(full_test_name_.c_str());
        
        // Initialize heap with memory tracking
        InitializeHeapWithTracking();
        
        // Store initial heap state for delta calculations
        CaptureInitialHeapState();
        
        // Log test start
        LogTestStart();
    }
    
    void TearDown() override {
        // Record test end time and calculate duration
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            test_end_time - test_start_time_);
        test_duration_ms_ = duration.count();
        
        // Log test completion
        LogTestEnd();
        
        // Log heap usage analysis
        LogHeapUsageAnalysis();
        
        // Log performance summary
        LogPerformanceSummary();
        
        // Check for memory leaks and generate reports
        CheckMemoryAndGenerateReports();
        
        // Cleanup
        CleanupResources();
    }

private:
    // Test timing and identification
    std::chrono::high_resolution_clock::time_point test_start_time_;
    long long test_duration_ms_;
    std::string test_suite_name_;
    std::string test_name_;
    std::string full_test_name_;
    
    // Performance measurement
    std::chrono::high_resolution_clock::time_point operation_start_time_;
    std::chrono::high_resolution_clock::time_point execution_start_time_;  // For start/end execution timing
    std::vector<long long> operation_timings_;  // in microseconds
    
    // Heap tracking
    SIZE_T initial_allocation_count_;
    SIZE_T initial_total_bytes_;
    SIZE_T initial_current_bytes_;
    SIZE_T initial_peak_bytes_;
    SIZE_T initial_double_free_count_;
    SIZE_T initial_freed_entry_count_;
    
    // Memory status tracking
    std::string memory_status_summary_;
    std::string memory_status_details_;
    bool memory_status_clean_;

    // Missing function implementations for test name tracking
    void SetCurrentTestName(const char* test_name) {
        // Store test name for correlation with heap logging
        // This could be used by heap debugging for better tracing
        // For now, this is a simple stub
        (void)test_name; // Suppress unused parameter warning
    }
    
    void ClearCurrentTestName() {
        // Clear test name at end of test
        // This is a simple stub for now
    }

    void InitializeDebugLogging() {
        // Initialize debug system
        DebugInitialize();
        
        // Set maximum verbosity for comprehensive logging
        DebugSetLevel(DEBUG_LEVEL_TRACE);
        DebugSetComponentMask((DWORD)DEBUG_COMPONENT_ALL);
        
        // Enable all debugging features
        DebugEnableTimestamp(TRUE);
        DebugEnableThreadId(TRUE);
        DebugEnableFileLocation(TRUE);
        
        // Disable console and debugger output for performance
        // but enable file logging
        DebugEnableConsoleOutput(FALSE);
        DebugEnableDebuggerOutput(FALSE);
        
        // Create test-specific log file name
        std::string log_filename = test_suite_name_ + "_debug.log";
        DebugEnableFileOutput(TRUE, log_filename.c_str());
    }
    
    void InitializeHeapWithTracking() {
        // Initialize heap with full tracking enabled
        ASSERT_TRUE(InitHeap()) << "Failed to initialize heap for test: " << full_test_name_;
        
        // Verify heap handle is valid
        GLOBAL_STATE* state = GetGlobalState();
        ASSERT_NE(state, nullptr) << "Failed to get global heap state";
        ASSERT_NE(state->HeapHandle, nullptr) << "Heap handle is null";
        
        // Enable comprehensive memory tracking
        SetFreedMemoryTracking(TRUE);
        SetMaxFreedEntries(1000);  // Track up to 1000 freed allocations
        SetErrorSuppression(FALSE); // Report all errors
    }
    
    void CaptureInitialHeapState() {
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            initial_allocation_count_ = state->AllocationCount;
            initial_total_bytes_ = state->TotalBytesAllocated;
            initial_current_bytes_ = state->CurrentBytesAllocated;
            initial_peak_bytes_ = state->PeakBytesAllocated;
            initial_double_free_count_ = state->DoubleFreeCount;
            initial_freed_entry_count_ = state->FreedEntryCount;
        } else {
            initial_allocation_count_ = 0;
            initial_total_bytes_ = 0;
            initial_current_bytes_ = 0;
            initial_peak_bytes_ = 0;
            initial_double_free_count_ = 0;
            initial_freed_entry_count_ = 0;
        }
        
        // Initialize memory status tracking
        memory_status_summary_ = "UNKNOWN";
        memory_status_details_ = "Not analyzed";
        memory_status_clean_ = false;
    }
    
    void LogTestStart() {
        HEAP_INFO("==============================================");
        HEAP_INFO("=== STARTING TEST: %s ===", full_test_name_.c_str());
        HEAP_INFO("=== Test Suite: %s ===", test_suite_name_.c_str());
        HEAP_INFO("=== Test Case: %s ===", test_name_.c_str());
        HEAP_INFO("=== Start Time: %s ===", GetCurrentTimeString().c_str());
        HEAP_INFO("==============================================");
        
        // Log system information
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            HEAP_INFO("=== HEAP CONFIGURATION ===");
            HEAP_INFO("Heap Handle: %p", state->HeapHandle);
            HEAP_INFO("Track Freed Memory: %s", state->TrackFreedMemory ? "YES" : "NO");
            HEAP_INFO("Max Freed Entries: %zu", state->MaxFreedEntries);
            HEAP_INFO("Suppress Errors: %s", state->SuppressErrors ? "YES" : "NO");
            HEAP_INFO("===============================");
        }
    }
    
    void LogTestEnd() {
        HEAP_INFO("==============================================");
        HEAP_INFO("=== ENDING TEST: %s ===", full_test_name_.c_str());
        HEAP_INFO("=== Duration: %lld ms ===", test_duration_ms_);
        HEAP_INFO("=== End Time: %s ===", GetCurrentTimeString().c_str());
        HEAP_INFO("==============================================");
    }
    
    void LogHeapUsageAnalysis() {
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            SIZE_T allocation_delta = state->AllocationCount - initial_allocation_count_;
            SIZE_T total_bytes_delta = state->TotalBytesAllocated - initial_total_bytes_;
            long long current_bytes_delta = (long long)(state->CurrentBytesAllocated) - (long long)(initial_current_bytes_);
            SIZE_T peak_bytes_delta = state->PeakBytesAllocated - initial_peak_bytes_;
            SIZE_T double_free_delta = state->DoubleFreeCount - initial_double_free_count_;
            SIZE_T freed_entries_delta = state->FreedEntryCount - initial_freed_entry_count_;
            
            HEAP_INFO("=== HEAP USAGE ANALYSIS ===");
            HEAP_INFO("Allocations During Test: +%zu", allocation_delta);
            HEAP_INFO("Total Bytes Allocated: +%zu", total_bytes_delta);
            HEAP_INFO("Current Bytes Change: %+lld", current_bytes_delta);
            HEAP_INFO("Peak Bytes Change: +%zu", peak_bytes_delta);
            HEAP_INFO("Double-Free Attempts: +%zu", double_free_delta);
            HEAP_INFO("Freed Entries Tracked: +%zu", freed_entries_delta);
            HEAP_INFO("===========================");
            
            // Detailed memory problem analysis
            bool has_memory_leaks = (current_bytes_delta > 0);
            bool has_double_frees = (double_free_delta > 0);
            bool has_memory_problems = has_memory_leaks || has_double_frees;
            
            if (has_memory_problems) {
                HEAP_INFO("=== MEMORY PROBLEMS DETECTED ===");
                
                if (has_memory_leaks) {
                    HEAP_INFO(">>  MEMORY LEAK: %lld bytes not freed", current_bytes_delta);
                    HEAP_INFO("   - This indicates memory that was allocated but never freed");
                    HEAP_INFO("   - Review your test for missing ExFreePoolTracked() calls");
                }
                
                if (has_double_frees) {
                    HEAP_INFO(">>  DOUBLE-FREE: %zu attempts to free already freed memory", double_free_delta);
                    HEAP_INFO("   - This indicates calling ExFreePoolTracked() on already freed memory");
                    HEAP_INFO("   - Check for duplicate free calls or use-after-free patterns");
                }
                
                HEAP_INFO("=================================");
            } else if (current_bytes_delta == 0) {
                HEAP_INFO("> MEMORY STATUS: Clean - No leaks or double-frees detected");
            } else {
                HEAP_INFO(">>  MEMORY STATUS: Net negative allocation (%lld bytes)", current_bytes_delta);
            }
            
            // Generate comprehensive memory status summary
            GenerateMemoryStatusSummary(has_memory_leaks, has_double_frees, current_bytes_delta, 
                                      double_free_delta, allocation_delta, total_bytes_delta);
        }
    }
    
    void LogPerformanceSummary() {
        HEAP_INFO("=== PERFORMANCE SUMMARY ===");
        HEAP_INFO("Total Test Duration: %lld ms", test_duration_ms_);
        
        if (!operation_timings_.empty()) {
            HEAP_INFO("Operation Count: %zu", operation_timings_.size());
            
            long long total_ops_time = 0;
            long long min_time = LLONG_MAX;
            long long max_time = 0;
            
            for (long long timing : operation_timings_) {
                total_ops_time += timing;
                if (timing < min_time) min_time = timing;
                if (timing > max_time) max_time = timing;
            }
            
            long long avg_time = total_ops_time / (long long)operation_timings_.size();
            
            HEAP_INFO("Operations Total Time: %lld ?s", total_ops_time);
            HEAP_INFO("Average Operation Time: %lld ?s", avg_time);
            HEAP_INFO("Min Operation Time: %lld ?s", min_time);
            HEAP_INFO("Max Operation Time: %lld ?s", max_time);
            
            // Performance analysis
            if (!operation_timings_.empty() && operation_timings_.size() > 1) {
                // Calculate standard deviation
                double mean = (double)avg_time;
                double variance = 0.0;
                for (long long timing : operation_timings_) {
                    variance += (timing - mean) * (timing - mean);
                }
                variance /= (double)operation_timings_.size();
                double std_dev = sqrt(variance);
                
                HEAP_INFO("Standard Deviation: %.2f ?s", std_dev);
                
                // Performance consistency analysis
                double cv = (std_dev / mean) * 100.0; // Coefficient of variation
                HEAP_INFO("Coefficient of Variation: %.2f%%", cv);
                
                if (cv < 5.0) {
                    HEAP_INFO("EXCELLENT: Very consistent performance");
                } else if (cv < 15.0) {
                    HEAP_INFO("GOOD: Reasonably consistent performance");
                } else if (cv < 30.0) {
                    HEAP_INFO("FAIR: Moderate performance variation");
                } else {
                    HEAP_INFO("POOR: High performance variation");
                }
            }
        } else {
            HEAP_INFO("No timed operations recorded");
        }
        HEAP_INFO("===========================");
    }
    
    void CheckMemoryAndGenerateReports() {
        HEAP_INFO("=== GENERATING MEMORY REPORTS ===");
        
        // Generate comprehensive memory leak report
        PrintMemoryLeaks();
        
        // Generate double-free report  
        PrintDoubleFreeReport();
        
        // Log final statistics
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            HEAP_INFO("=== FINAL TEST STATISTICS ===");
            HEAP_INFO("Total Allocations: %zu", state->AllocationCount);
            HEAP_INFO("Total Bytes Allocated: %zu", state->TotalBytesAllocated);
            HEAP_INFO("Peak Bytes Allocated: %zu", state->PeakBytesAllocated);
            HEAP_INFO("Current Bytes Allocated: %zu", state->CurrentBytesAllocated);
            HEAP_INFO("Double-Free Attempts: %zu", state->DoubleFreeCount);
            HEAP_INFO("Freed Entries Tracked: %zu", state->FreedEntryCount);
            HEAP_INFO("===================================");
        }
        
        // Generate final memory health report
        GenerateFinalMemoryHealthReport();
    }
    
    void GenerateMemoryStatusSummary(bool has_leaks, bool has_double_frees, 
                                   long long bytes_delta, SIZE_T double_free_count,
                                   SIZE_T allocations, SIZE_T total_bytes) {
        std::string status_icon;
        std::string status_text;
        std::string details;
        
        if (!has_leaks && !has_double_frees) {
            status_icon = "?";
            status_text = "MEMORY_CLEAN";
            details = "No memory problems detected";
        } else {
            status_icon = "?";
            status_text = "MEMORY_PROBLEMS";
            
            std::vector<std::string> problems;
            if (has_leaks) {
                char leak_info[128];
                snprintf(leak_info, sizeof(leak_info), "LEAK(%lld bytes)", bytes_delta);
                problems.push_back(leak_info);
            }
            if (has_double_frees) {
                char double_free_info[128];
                snprintf(double_free_info, sizeof(double_free_info), "DOUBLE_FREE(%zu attempts)", double_free_count);
                problems.push_back(double_free_info);
            }
            
            details = "";
            for (size_t i = 0; i < problems.size(); i++) {
                if (i > 0) details += ", ";
                details += problems[i];
            }
        }
        
        HEAP_INFO("=== MEMORY STATUS SUMMARY ===");
        HEAP_INFO("%s TEST: %s", status_icon.c_str(), full_test_name_.c_str());
        HEAP_INFO("%s STATUS: %s", status_icon.c_str(), status_text.c_str());
        HEAP_INFO("%s DETAILS: %s", status_icon.c_str(), details.c_str());
        HEAP_INFO("%s ALLOCATIONS: %zu operations, %zu bytes total", status_icon.c_str(), allocations, total_bytes);
        HEAP_INFO("==============================");
        
        // Store summary for final report
        memory_status_summary_ = status_text;
        memory_status_details_ = details;
        memory_status_clean_ = (!has_leaks && !has_double_frees);
    }
    
    void GenerateFinalMemoryHealthReport() {
        HEAP_INFO("=== FINAL MEMORY HEALTH REPORT ===");
        
        if (memory_status_clean_) {
            HEAP_INFO(">> OVERALL RESULT: MEMORY HEALTH EXCELLENT");
            HEAP_INFO("    + No memory leaks detected");
            HEAP_INFO("    + No double-free attempts");
            HEAP_INFO("    + All allocated memory properly freed");
            HEAP_INFO("    + Test passed all memory safety checks");
        } else {
            HEAP_INFO(">> OVERALL RESULT: MEMORY HEALTH ISSUES DETECTED");
            HEAP_INFO("   + Memory problems found: %s", memory_status_details_.c_str());
            HEAP_INFO("   + Review test implementation for memory management errors");
            HEAP_INFO("   + Check the detailed reports above for specific issues");
            
            // Provide actionable recommendations
            HEAP_INFO("=== RECOMMENDATIONS ===");
            if (memory_status_details_.find("LEAK") != std::string::npos) {
                HEAP_INFO("   >> Memory Leak Resolution:");
                HEAP_INFO("      - Ensure every ExAllocatePoolTracked() has matching ExFreePoolTracked()");
                HEAP_INFO("      - Check for early returns that skip cleanup code");
                HEAP_INFO("      - Verify exception handling paths free allocated memory");
            }
            if (memory_status_details_.find("DOUBLE_FREE") != std::string::npos) {
                HEAP_INFO("   >> Double-Free Prevention:");
                HEAP_INFO("      - Set pointers to NULL after calling ExFreePoolTracked()");
                HEAP_INFO("      - Avoid freeing the same pointer in multiple code paths");
                HEAP_INFO("      - Use RAII patterns or smart pointers where applicable");
            }
            HEAP_INFO("=======================");
        }
        
        // Single-line summary for easy parsing
        const char* result_emoji = memory_status_clean_ ? "+" : "-";
        HEAP_INFO("");
        HEAP_INFO("------------------------");
        HEAP_INFO("> %s MEMORY TEST RESULT: %-40s ?", result_emoji, memory_status_summary_.c_str());
        HEAP_INFO("------------------------");
        HEAP_INFO("");
        HEAP_INFO("===================================");
    }
    
    void CleanupResources() {
        // Clear test name
        ClearCurrentTestName();
        
        // Cleanup heap
        CleanupHeap();
        
        // Cleanup debug system
        DebugCleanup();
    }
    
    std::string GetCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, (int)ms.count());
        
        return std::string(buffer);
    }

protected:
    // Helper methods for derived test classes
    
    /**
     * @brief Log test-specific information
     */
    void LogTestInfo(const char* format, ...) {
        va_list args;
        va_start(args, format);
        char buffer[512];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        HEAP_INFO("[TEST-INFO] %s", buffer);
    }
    
    /**
     * @brief Log test step information
     */
    void LogTestStep(const char* step_name) {
        HEAP_INFO("=== TEST STEP: %s ===", step_name);
    }
    
    /**
     * @brief Get the current test's full name
     */
    const std::string& GetFullTestName() const {
        return full_test_name_;
    }
    
    /**
     * @brief Get the test suite name
     */
    const std::string& GetTestSuiteName() const {
        return test_suite_name_;
    }
    
    /**
     * @brief Get the test case name
     */
    const std::string& GetTestName() const {
        return test_name_;
    }
    
    /**
     * @brief Get test duration in milliseconds
     */
    long long GetTestDurationMs() const {
        return test_duration_ms_;
    }
    
    /**
     * @brief Get the current test duration in milliseconds (calculated from start time to now)
     */
    long long GetCurrentTestDurationMs() const {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - test_start_time_);
        return duration.count();
    }
    
    /**
     * @brief Get the current test duration in microseconds (calculated from start time to now)
     */
    long long GetCurrentTestDurationUs() const {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            current_time - test_start_time_);
        return duration.count();
    }
    
    /**
     * @brief Get the elapsed time since StartOperationTiming() was called (in microseconds)
     * @return Elapsed time in microseconds, or 0 if StartOperationTiming() wasn't called
     */
    long long GetCurrentOperationDurationUs() const {
        if (operation_start_time_.time_since_epoch().count() == 0) {
            return 0; // StartOperationTiming() wasn't called
        }
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            current_time - operation_start_time_);
        return duration.count();
    }
    
    /**
     * @brief Get the elapsed time since StartOperationTiming() was called (in milliseconds)
     * @return Elapsed time in milliseconds, or 0 if StartOperationTiming() wasn't called
     */
    long long GetCurrentOperationDurationMs() const {
        return GetCurrentOperationDurationUs() / 1000;
    }
    
    // ===== EXECUTION TIMING METHODS =====
    
    /**
     * @brief Start timing an execution/operation
     * @note Call this at the beginning of what you want to measure
     */
    void StartExecutionTiming() {
        execution_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    /**
     * @brief End timing an execution and get the duration
     * @return Duration in milliseconds since StartExecutionTiming() was called
     */
    long long EndExecutionTimingMs() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - execution_start_time_);
        return duration.count();
    }
    
    /**
     * @brief End timing an execution and get the duration
     * @return Duration in microseconds since StartExecutionTiming() was called
     */
    long long EndExecutionTimingUs() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - execution_start_time_);
        return duration.count();
    }
    
    // ===== PERFORMANCE MEASUREMENT METHODS =====
    
    /**
     * @brief Start timing an operation for statistics collection
     * @note Call this before an operation you want to record in operation_timings_
     * @note Use StartExecutionTiming()/EndExecutionTiming*() for simple start/end measurements
     */
    void StartOperationTiming() {
        operation_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    /**
     * @brief End timing an operation and record the duration for statistics
     * @note This records the timing in operation_timings_ for later statistical analysis
     * @note Use StartExecutionTiming()/EndExecutionTiming*() for simple start/end measurements
     */
    void EndOperationTiming() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - operation_start_time_);
        operation_timings_.push_back(duration.count());
    }
    
    /**
     * @brief Get the duration of the last timed operation in microseconds
     */
    long long GetLastOperationDurationUs() const {
        if (operation_timings_.empty()) {
            return 0;
        }
        return operation_timings_.back();
    }
    
    /**
     * @brief Get the duration of the last timed operation in milliseconds
     */
    long long GetLastOperationDurationMs() const {
        return GetLastOperationDurationUs() / 1000;
    }
    
    /**
     * @brief Get the average operation duration in microseconds
     */
    long long GetAverageOperationDurationUs() const {
        if (operation_timings_.empty()) {
            return 0;
        }
        
        long long total = 0;
        for (long long timing : operation_timings_) {
            total += timing;
        }
        return total / (long long)operation_timings_.size();
    }
    
    /**
     * @brief Get the average operation duration in milliseconds
     */
    long long GetAverageOperationDurationMs() const {
        return GetAverageOperationDurationUs() / 1000;
    }
    
    /**
     * @brief Get the number of timed operations
     */
    size_t GetOperationCount() const {
        return operation_timings_.size();
    }
    
    /**
     * @brief Clear all recorded operation timings
     */
    void ClearOperationTimings() {
        operation_timings_.clear();
    }
    
    // ===== HEAP ANALYSIS METHODS =====
    
    /**
     * @brief Get the number of allocations made during this test
     */
    SIZE_T GetTestAllocationCount() const {
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            return state->AllocationCount - initial_allocation_count_;
        }
        return 0;
    }
    
    /**
     * @brief Get the total bytes allocated during this test
     */
    SIZE_T GetTestTotalBytesAllocated() const {
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            return state->TotalBytesAllocated - initial_total_bytes_;
        }
        return 0;
    }
    
    /**
     * @brief Get the current bytes change since test start (positive = leak, negative = over-freed)
     */
    long long GetTestCurrentBytesChange() const {
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            return (long long)(state->CurrentBytesAllocated) - (long long)(initial_current_bytes_);
        }
        return 0;
    }
    
    /**
     * @brief Get the peak bytes change since test start
     */
    SIZE_T GetTestPeakBytesChange() const {
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            return state->PeakBytesAllocated - initial_peak_bytes_;
        }
        return 0;
    }
    
    /**
     * @brief Check if the test has memory leaks
     */
    bool HasMemoryLeaks() const {
        return GetTestCurrentBytesChange() > 0;
    }
    
    /**
     * @brief Get the number of double-free attempts during this test
     */
    SIZE_T GetTestDoubleFreeCount() const {
        GLOBAL_STATE* state = GetGlobalState();
        if (state) {
            return state->DoubleFreeCount - initial_double_free_count_;
        }
        return 0;
    }
    
    /**
     * @brief Check if there were any double-free attempts during this test
     */
    bool HasDoubleFreeAttempts() const {
        return GetTestDoubleFreeCount() > 0;
    }
    
    // ===== MEMORY STATUS SUMMARY METHODS =====
    
    /**
     * @brief Get a simple memory status summary
     * @return String like "MEMORY_CLEAN" or "MEMORY_PROBLEMS"
     */
    const std::string& GetMemoryStatusSummary() const {
        return memory_status_summary_;
    }
    
    /**
     * @brief Get detailed memory status information
     * @return String with details like "LEAK(1024 bytes), DOUBLE_FREE(2 attempts)"
     */
    const std::string& GetMemoryStatusDetails() const {
        return memory_status_details_;
    }
    
    /**
     * @brief Check if the test passed all memory safety checks
     * @return true if no memory problems detected, false otherwise
     */
    bool IsMemoryStatusClean() const {
        return memory_status_clean_;
    }
    
    /**
     * @brief Get a comprehensive memory report for the test
     * @return Formatted string with complete memory analysis
     */
    std::string GetMemoryReport() const {
        std::string report;
        const char* status_emoji = memory_status_clean_ ? "?" : "?";
        
        report += "=== MEMORY REPORT ===\n";
        report += std::string(status_emoji) + " Status: " + memory_status_summary_ + "\n";
        report += std::string(status_emoji) + " Details: " + memory_status_details_ + "\n";
        
        if (memory_status_clean_) {
            report += "> Result: All memory safety checks passed\n";
        } else {
            report += "> Result: Memory problems detected - review implementation\n";
        }
        
        report += "=====================\n";
        return report;
    }
};

#endif // WINKERNELLITE_TEST_BASE_H
