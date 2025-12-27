#include "../framework/test_framework.h"
#include "../../include/cli_parser.h"
#include <stdio.h>
#include <string.h>

// Test suite for CLI parsing
TEST_SUITE(cli_parsing);

static void cli_parsing_setup(void) {
    // Setup code if needed
}

static void cli_parsing_teardown(void) {
    // Teardown code if needed
}

// Test that checks CLI header with valid data
TEST(test_cli_header_valid) {
    // Create a minimal valid CLI header in memory
    uint8_t cli_data[72]; // Size of CLI header
    memset(cli_data, 0, sizeof(cli_data));
    
    // Set up some basic CLI header values
    cli_data[0] = 72;  // cb = 72 bytes
    cli_data[4] = 2;   // major_runtime_version = 2
    cli_data[6] = 5;   // minor_runtime_version = 5
    cli_data[16] = 1;  // flags = COMIMAGE_FLAGS_ILONLY
    
    // Parse the CLI header
    cli_header_t* header = parse_cli_header(cli_data, sizeof(cli_data));
    
    // Check that parsing succeeded
    TEST_ASSERT_NOT_NULL(header);
    
    // Check that the header is valid
    TEST_ASSERT_TRUE(header->is_valid);
    
    // Check basic fields
    TEST_ASSERT_EQUAL(72, header->cb);
    TEST_ASSERT_EQUAL(2, header->major_runtime_version);
    TEST_ASSERT_EQUAL(5, header->minor_runtime_version);
    
    // Check flag helper functions
    TEST_ASSERT_TRUE(cli_header_is_il_only(header));
    TEST_ASSERT_FALSE(cli_header_is_32bit_required(header));
    TEST_ASSERT_FALSE(cli_header_is_library(header));
    TEST_ASSERT_FALSE(cli_header_is_strong_name_signed(header));
    TEST_ASSERT_FALSE(cli_header_has_native_entrypoint(header));
    
    // Clean up
    free_cli_header(header);
    
    return TEST_PASSED;
}

// Test that checks CLI header with invalid data (NULL pointer)
TEST(test_cli_header_null_pointer) {
    // Parse with NULL pointer
    cli_header_t* header = parse_cli_header(NULL, 10);
    
    // Should return NULL for NULL input
    TEST_ASSERT_NULL(header);
    
    return TEST_PASSED;
}

// Test that checks CLI header with incomplete data
TEST(test_cli_header_incomplete) {
    // Create incomplete data (smaller than CLI header size)
    uint8_t incomplete_data[32];
    memset(incomplete_data, 0, sizeof(incomplete_data));
    
    // Parse the incomplete data
    cli_header_t* header = parse_cli_header(incomplete_data, sizeof(incomplete_data));
    
    // Check that parsing succeeded (we still return a header structure)
    TEST_ASSERT_NOT_NULL(header);
    
    // Check that the header is not valid
    TEST_ASSERT_FALSE(header->is_valid);
    
    // Clean up
    free_cli_header(header);
    
    return TEST_PASSED;
}

// Test flag helper functions
TEST(test_cli_header_flags) {
    // Create a CLI header with various flags set
    uint8_t cli_data[72];
    memset(cli_data, 0, sizeof(cli_data));
    
    cli_data[0] = 72;  // cb = 72 bytes
    
    // Set multiple flags
    cli_data[16] = 0x01; // COMIMAGE_FLAGS_ILONLY (bit 0)
    cli_data[17] = 0x02; // COMIMAGE_FLAGS_32BITREQUIRED (bit 9 when combined)
    
    // Parse the CLI header
    cli_header_t* header = parse_cli_header(cli_data, sizeof(cli_data));
    
    // Check that parsing succeeded
    TEST_ASSERT_NOT_NULL(header);
    TEST_ASSERT_TRUE(header->is_valid);
    
    // Check flag helper functions
    TEST_ASSERT_TRUE(cli_header_is_il_only(header));
    // Note: We're not setting the correct bit pattern for 32BITREQUIRED
    // 32BITREQUIRED is bit 1, so we need to set cli_data[17] = 0x00 and cli_data[16] = 0x02
    TEST_ASSERT_FALSE(cli_header_is_32bit_required(header));
    TEST_ASSERT_FALSE(cli_header_is_library(header));
    TEST_ASSERT_FALSE(cli_header_is_strong_name_signed(header));
    TEST_ASSERT_FALSE(cli_header_has_native_entrypoint(header));
    
    // Clean up
    free_cli_header(header);
    
    // Test with 32BITREQUIRED flag correctly set
    memset(cli_data, 0, sizeof(cli_data));
    cli_data[0] = 72;
    cli_data[16] = 0x02; // COMIMAGE_FLAGS_32BITREQUIRED (bit 1)
    
    header = parse_cli_header(cli_data, sizeof(cli_data));
    TEST_ASSERT_NOT_NULL(header);
    TEST_ASSERT_TRUE(header->is_valid);
    TEST_ASSERT_TRUE(cli_header_is_32bit_required(header));
    free_cli_header(header);
    
    return TEST_PASSED;
}

int main(void) {
    printf("=== CLI Parsing Tests ===\n\n");
    
    // Register the test suite
    test_register_suite(&suite_cli_parsing);
    
    // Set current suite
    current_suite = &suite_cli_parsing;
    
    // Register test cases
    static test_case_t test_cli_valid = {
        .suite_name = "cli_parsing",
        .test_name = "test_cli_header_valid",
        .test_func = test_cli_header_valid_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cli_null_pointer = {
        .suite_name = "cli_parsing",
        .test_name = "test_cli_header_null_pointer",
        .test_func = test_cli_header_null_pointer_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cli_incomplete = {
        .suite_name = "cli_parsing",
        .test_name = "test_cli_header_incomplete",
        .test_func = test_cli_header_incomplete_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cli_flags = {
        .suite_name = "cli_parsing",
        .test_name = "test_cli_header_flags",
        .test_func = test_cli_header_flags_wrapper,
        .next = NULL
    };
    
    test_register_case(&test_cli_valid);
    test_register_case(&test_cli_null_pointer);
    test_register_case(&test_cli_incomplete);
    test_register_case(&test_cli_flags);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("cli_parsing");
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }
    
    return test_runner_state.failed_tests > 0 ? 1 : 0;
}