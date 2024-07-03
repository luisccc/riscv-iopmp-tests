#include <rvh_test.h>
#include <iopmp_tests.h>

/**
 *  List of tests.
 *  To disable a test from the application, comment the corresponding line
 */

// IOPMP
#if NUMBER_MASTERS > 1
    TEST_REGISTER(full_iopmp_non_belonging_mds);
    TEST_REGISTER(full_iopmp_belonging_mds);
#else
    TEST_REGISTER(complex_tor_entries);
    TEST_REGISTER(error_recording);
    TEST_REGISTER(priority_tests);
    TEST_REGISTER(large_napot_entries);
    TEST_REGISTER(simple_tor_entries);
    TEST_REGISTER(simple_napot_entries);
    TEST_REGISTER(napot_smallest_lenght_entries);
#endif

// TEST_REGISTER(latency_testing_reader);
// TEST_REGISTER(general_latency_test);
// TEST_REGISTER(idma_only);
TEST_REGISTER(iopmp_off);
