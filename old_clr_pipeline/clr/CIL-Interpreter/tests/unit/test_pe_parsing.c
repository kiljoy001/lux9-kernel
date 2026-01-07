#include "../framework/test_framework.h"
#include "../../include/pe_parser.h"
#include <stdio.h>
#include <string.h>

// Test suite for PE parsing
TEST_SUITE(pe_parsing);

static void pe_parsing_setup(void) {
    // Setup code if needed
}

static void pe_parsing_teardown(void) {
    // Teardown code if needed
}

// Test that checks PE header magic number with valid PE data
TEST(test_pe_magic_number_valid) {
    // Create a minimal valid PE file in memory
    // This is a very simplified PE structure for testing
    uint8_t pe_data[0x44]; // Make sure we have enough space
    
    // Initialize with zeros
    memset(pe_data, 0, sizeof(pe_data));
    
    // MS-DOS header signature "MZ"
    pe_data[0] = 0x4D;  // 'M'
    pe_data[1] = 0x5A;  // 'Z'
    
    // PE signature offset (0x3C) points to 0x40
    pe_data[0x3C] = 0x40;  // PE header starts at offset 0x40
    pe_data[0x3D] = 0x00;  // High byte of offset
    
    // PE signature "PE\0\0" at offset 0x40
    pe_data[0x40] = 0x50;  // 'P'
    pe_data[0x41] = 0x45;  // 'E'
    pe_data[0x42] = 0x00;  // '\0'
    pe_data[0x43] = 0x00;  // '\0'
    
    // Parse the PE header
    pe_header_t* header = parse_pe_header(pe_data, sizeof(pe_data));
    
    // Check that parsing succeeded
    TEST_ASSERT_NOT_NULL(header);
    
    // Check that the DOS signature is correct
    TEST_ASSERT_EQUAL(DOS_SIGNATURE, header->dos_signature);
    
    // Check that the PE signature is correct
    TEST_ASSERT_EQUAL(PE_SIGNATURE, header->pe_signature);
    
    // Check that the PE file is valid
    TEST_ASSERT_TRUE(header->is_valid_pe);
    
    // Clean up
    free_pe_header(header);
    
    return TEST_PASSED;
}

// Test that checks PE header magic number with invalid data
TEST(test_pe_magic_number_invalid) {
    // Create invalid data (not a PE file)
    uint8_t invalid_data[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    // Parse the invalid data
    pe_header_t* header = parse_pe_header(invalid_data, sizeof(invalid_data));
    
    // Check that parsing succeeded (we still return a header structure)
    TEST_ASSERT_NOT_NULL(header);
    
    // Check that the DOS signature is not the expected one or PE signature is not expected
    TEST_ASSERT_TRUE(header->dos_signature != DOS_SIGNATURE || 
                     header->pe_signature != PE_SIGNATURE);
    
    // Check that the PE file is not valid
    TEST_ASSERT_FALSE(header->is_valid_pe);
    
    // Clean up
    free_pe_header(header);
    
    return TEST_PASSED;
}

// Test that checks MS-DOS header signature with valid data
TEST(test_dos_header_signature_valid) {
    // Create data with valid DOS signature
    uint8_t dos_data[] = {
        0x4D, 0x5A,  // "MZ" signature
        0x90, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00
    };
    
    // Parse the DOS header only
    pe_header_t* header = parse_pe_header(dos_data, sizeof(dos_data));
    
    // Check that parsing succeeded
    TEST_ASSERT_NOT_NULL(header);
    
    // Check that the DOS signature is correct
    TEST_ASSERT_EQUAL(DOS_SIGNATURE, header->dos_signature);
    
    // Clean up
    free_pe_header(header);
    
    return TEST_PASSED;
}

int main(void) {
    printf("=== PE Parsing Tests ===\n\n");
    
    // Register the test suite
    test_register_suite(&suite_pe_parsing);
    
    // Set current suite
    current_suite = &suite_pe_parsing;
    
    // Register test cases
    static test_case_t test_pe_valid = {
        .suite_name = "pe_parsing",
        .test_name = "test_pe_magic_number_valid",
        .test_func = test_pe_magic_number_valid_wrapper,
        .next = NULL
    };
    
    static test_case_t test_pe_invalid = {
        .suite_name = "pe_parsing",
        .test_name = "test_pe_magic_number_invalid",
        .test_func = test_pe_magic_number_invalid_wrapper,
        .next = NULL
    };
    
    static test_case_t test_dos_valid = {
        .suite_name = "pe_parsing",
        .test_name = "test_dos_header_signature_valid",
        .test_func = test_dos_header_signature_valid_wrapper,
        .next = NULL
    };
    
    test_register_case(&test_pe_valid);
    test_register_case(&test_pe_invalid);
    test_register_case(&test_dos_valid);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("pe_parsing");
    // Note: We don't have test_print_results in our framework, so we'll implement our own simple version
    
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