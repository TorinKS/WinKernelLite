#include <gtest/gtest.h>
#include <WinKernelLite/Wdm.h>

TEST(TraceTest, TestTraceLevelDefinitions) {
    // Verify that trace level constants have expected values
    EXPECT_EQ(TRACE_LEVEL_NONE, 0);
    EXPECT_EQ(TRACE_LEVEL_CRITICAL, 1);
    EXPECT_EQ(TRACE_LEVEL_FATAL, 1);  // Should be alias for CRITICAL
    EXPECT_EQ(TRACE_LEVEL_ERROR, 2);
    EXPECT_EQ(TRACE_LEVEL_WARNING, 3);
    EXPECT_EQ(TRACE_LEVEL_INFORMATION, 4);
    EXPECT_EQ(TRACE_LEVEL_VERBOSE, 5);
}

TEST(TraceTest, TestCriticalAndFatalEquivalence) {
    // Test that CRITICAL and FATAL have the same value
    EXPECT_EQ(TRACE_LEVEL_CRITICAL, TRACE_LEVEL_FATAL);
    
    // Test that both constants can be used in function calls
    // (This will produce output but won't crash)
    DoTraceEx(TRACE_LEVEL_CRITICAL, APP_GENERAL, "Critical message test");
    DoTraceEx(TRACE_LEVEL_FATAL, APP_GENERAL, "Fatal message test");
    
    // If we get here without crashing, the test passes
    SUCCEED();
}

TEST(TraceTest, TestAllTraceLevelsWork) {
    // Test that all trace levels can be called without crashing
    DoTraceEx(TRACE_LEVEL_NONE, APP_GENERAL, "Test NONE level");
    DoTraceEx(TRACE_LEVEL_CRITICAL, APP_GENERAL, "Test CRITICAL level");
    DoTraceEx(TRACE_LEVEL_FATAL, APP_GENERAL, "Test FATAL level");
    DoTraceEx(TRACE_LEVEL_ERROR, APP_GENERAL, "Test ERROR level");
    DoTraceEx(TRACE_LEVEL_WARNING, APP_GENERAL, "Test WARNING level");
    DoTraceEx(TRACE_LEVEL_INFORMATION, APP_GENERAL, "Test INFORMATION level");
    DoTraceEx(TRACE_LEVEL_VERBOSE, APP_GENERAL, "Test VERBOSE level");
    
    // If we get here without crashing, the test passes
    SUCCEED();
}

TEST(TraceTest, TestDebugComponentConstants) {
    // Test that debug component constants are defined and can be used
    DoTraceEx(TRACE_LEVEL_INFORMATION, APP_GENERAL, "Test APP_GENERAL");
    DoTraceEx(TRACE_LEVEL_INFORMATION, APP_WL_FILTER, "Test APP_WL_FILTER");
    DoTraceEx(TRACE_LEVEL_INFORMATION, APP_DEVICE_FILTER, "Test APP_DEVICE_FILTER");
    
    // If we get here without crashing, the test passes
    SUCCEED();
}
