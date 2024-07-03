#ifndef IOPMP_TESTS_H
#define IOPMP_TESTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <csrs.h>
#include <instructions.h>
#include <platform.h>
#include <rvh_test.h>

// Base Supervisor Physical Address of 4-kiB pages
#define TEST_PPAGE_BASE (MEM_BASE+(MEM_SIZE/2))     // 0x8000_0000 + 0x0800_0000 = 0x88000000

#define PAGE_SIZE           0x1000ULL     // 4kiB

#define NUMBER_MASTERS       1
#define NUMBER_MDS           16
#define NUMBER_ENTRIES       32
#define NUMBER_PRIO_ENTRIES  4

bool iopmp_off();
bool simple_napot_entries();
bool simple_tor_entries();
bool napot_smallest_lenght_entries();
bool priority_tests();
bool error_recording();
bool full_iopmp_belonging_mds();
bool full_iopmp_non_belonging_mds();
bool large_napot_entries();
bool complex_tor_entries();
bool idma_only();
bool general_latency_test();


#endif /* IOPMP_TESTS_H */