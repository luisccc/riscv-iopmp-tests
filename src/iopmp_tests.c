#include <iopmp_tests.h>
#include <rv_iopmp.h>
#include <plat_dma.h>
#include <idma.h>

#define SEED 100
#define MDS_PER_SID 3
#define ITERATIONS  5

uint16_t configured_mds = 0;
uint16_t md_t[NUMBER_MDS] = {0};
uint16_t sid_mds[NUMBER_MASTERS][MDS_PER_SID];

unsigned long read_cycles(void)
{
  unsigned long cycles;
  asm volatile ("rdcycle %0" : "=r" (cycles));
  return cycles;
}

/**
 *  iDMA device IDs 
 */
extern uint64_t idma_ids[];

/**
 *  iDMA Configuration Registers
 */
extern uintptr_t idma_src[];
extern uintptr_t idma_dest[];
extern uintptr_t idma_nbytes[];
extern uintptr_t idma_config[];
extern uintptr_t idma_status[];
extern uintptr_t idma_nextid[];
extern uintptr_t idma_done[];
extern uintptr_t idma_ipsr[];

bool value_in_array(uint16_t val, uint16_t *arr, uint8_t n) {
    for(uint8_t i = 0; i < n; i++) {
        if(arr[i] == val)
            return true;
    }
    return false;
}

void set_random_mds()
{
    uint16_t t, i;

    t = (rand() % 4 )+ 1;
    for (i = 0; i < NUMBER_MDS - 1; i++)
    {   
        mdcfg_entry_config(t, i);
        md_t[i] = t;
        t += (rand() % 4 )+ 1; // Between 1 and 4

        if(t > NUMBER_ENTRIES)
            break;
    }

    mdcfg_entry_config(NUMBER_ENTRIES, i);
    md_t[i] = NUMBER_ENTRIES;

    configured_mds = i;
}

void set_random_sids()
{
    for (int i = 0; i < NUMBER_MASTERS; i++)
    {   
        for (int j = 0; j < MDS_PER_SID; )
        {
            bool check = false;
            uint16_t md = (rand() % configured_mds );

            check = value_in_array(md, sid_mds[i], MDS_PER_SID);

            if (!check)
            {
                sid_mds[i][j] = md;
                j++;
            }
        }
        srcmd_entry_config(sid_mds[i], MDS_PER_SID, 0, i);
    }
}

void set_multiple_napot_entries(uint8_t start_entry, uint8_t end_entry, uint8_t access, uint64_t base_addr, uint64_t length)
{
    for (int i = start_entry; i <= end_entry; i++)
    {   
        set_entry_napot(base_addr, length, access, i);
    }
}

uint64_t set_random_napot_entries(uint8_t start_entry, uint8_t end_entry, uint8_t access, uint64_t base_addr)
{
    // uint64_t base_addr = TEST_PPAGE_BASE;

    // Although the length is not totally random,
    // this way, we make sure that it correctly aligns for napot entries
    uint64_t length = 1 << ((rand() % 1 ) + 9);  // Minimum 8 bytes, max 2 kb from current dma buffers
    VERBOSE("Configuring from %d to %d entries", start_entry, end_entry);
    for (int i = start_entry; i <= end_entry; i++)
    {   
        set_entry_napot(base_addr, length, access, i); 

        base_addr += length;
    }

    return length;
}

/**
 *  Carry out a multi-beat DMA transfer with the IOPMP OFF, using physical addresses
 *  Check if all the transfers fail
 */
bool iopmp_off()
{

    TEST_START();

    fence_i();
    VERBOSE("IOPMP OFF");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    //# Get addresses
    uintptr_t start_raddr1 = TEST_PPAGE_BASE;
    uintptr_t start_waddr1 = TEST_PPAGE_BASE + 32;

    //# Write known values to memory
    write64(start_raddr1     , 0x00);
    write64(start_raddr1 + 8 , 0x10);
    write64(start_raddr1 + 16, 0x20);

    write64(start_waddr1     , 0x00);
    write64(start_waddr1 + 8 , 0x00);
    write64(start_waddr1 + 16, 0x00);

    //INFO("Test");
    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr1, start_waddr1, 24);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    bool check;
    check =    ~((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x00) &&
                (read64(start_waddr1 + 16) == 0x00));
    TEST_ASSERT("", check);
    
}

/**
 *  Carry out a single-beat DMA transfer with the IOPMP ON, using physical addresses
 *  Check if the transaction is accepted, with an only 8 byte region
 */
bool napot_smallest_lenght_entries()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    uint16_t md = 0;
    mdcfg_entry_config(5, 0);
    srcmd_entry_config(&md, 1, 0, 0);
    
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 32;

    //# Core writes known values to memory
    write64(start_raddr     , 0x00);
    write64(start_waddr     , 0xFFFF);
    fence_i();

    // Set entry 0 with read permissions
    set_entry_napot(start_raddr, 8, ACCESS_READ, 0);
    // // Set entry 1 with write permissions
    set_entry_napot(start_waddr, 8, ACCESS_WRITE, 1);

    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 4);
    if (idma_exec_transfer(dma_ut) != 0)
        {INFO("iDMA misconfigured")}
    
    bool check;
    fence_i();
    check = (read64(start_waddr) == 0x00);
    TEST_ASSERT("", check);
}

/**
 *  Carry out a multi-beat DMA transfer with the IOPMP ON, using physical addresses
 *  1 - Simple make the iDMA read and write fall inside the same entry
 *  2 - Two entries are set in the IOPMP to ensure read and write permissions for different addresses
 *  
 */
bool simple_napot_entries()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    uint16_t md = 0;
    mdcfg_entry_config(5, 0);
    srcmd_entry_config(&md, 1, 0, 0);
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 24;

    //# Core writes known values to memory
    write64(start_raddr     , 0x00);
    write64(start_raddr + 8 , 0x10);
    write64(start_raddr + 16, 0x20);

    write64(start_waddr     , 0x00);
    write64(start_waddr + 8 , 0x00);
    write64(start_waddr + 16, 0x00);

    // Set entry 0 with read and write permissions for the written zone
    set_entry_napot(start_raddr, 64, ACCESS_READ | ACCESS_WRITE, 0);

    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    fence_i();
    //# Check transfer
    bool check;
    check =    ((read64(start_waddr     ) == 0x00) &&
                (read64(start_waddr + 8 ) == 0x10) &&
                (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("SAME ENTRY", check);

    start_waddr = start_raddr + 64;

    // Set entry 0 with read permissions
    set_entry_napot(start_raddr, 32, ACCESS_READ, 0);
    // Set entry 1 with write permissions
    set_entry_napot(start_waddr, 32, ACCESS_WRITE, 1);

    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    fence_i();
    //# Check transfer
    check =   ((read64(start_waddr     ) == 0x00) &&
                (read64(start_waddr + 8 ) == 0x10) &&
                (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("SEPARATE ENTRIES", check);
}

/**
 *  Carry out a multi-beat DMA transfer with the IOPMP ON, using physical addresses (TOR requires 2 entries for a zone)
 *  1 - Simple make the iDMA read and write fall inside the same entries
 *  2 - four entries are set in the IOPMP to ensure read and write permissions for different addresses
 *  
 */
bool simple_tor_entries()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];
    
    uint16_t md = 0;
    mdcfg_entry_config(5, 0);
    srcmd_entry_config(&md, 1, 0, 0);
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 24;

    //# Core writes known values to memory
    write64(start_raddr     , 0x00);
    write64(start_raddr + 8 , 0x10);
    write64(start_raddr + 16, 0x20);

    write64(start_waddr     , 0x00);
    write64(start_waddr + 8 , 0x00);
    write64(start_waddr + 16, 0x00);

    // Put 1 TOR entry allowing reads and writes
    set_entry_off(start_raddr, ACCESS_NONE, 0);
    set_entry_tor(start_raddr + 64, ACCESS_READ | ACCESS_WRITE, 1);

    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    bool check = ((read64(start_waddr    ) == 0x00) &&
                 (read64(start_waddr + 8 ) == 0x10) &&
                 (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("SAME ENTRY", check);
    // Flush cache
    fence_i();
    // Now test four entries with distinct areas
    start_raddr = TEST_PPAGE_BASE;
    start_waddr = start_raddr + 64;

    write64(start_waddr     , 0x00);
    write64(start_waddr + 8 , 0x00);
    write64(start_waddr + 16, 0x00);

    set_entry_off(start_raddr, ACCESS_READ, 0);
    set_entry_tor(start_raddr + 32, ACCESS_READ, 1);

    set_entry_off(start_waddr, ACCESS_WRITE, 2);
    set_entry_tor(start_waddr + 32, ACCESS_WRITE, 3);

    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    // Flush cache
    fence_i();
    //# Check transfer
    check = ((read64(start_waddr     ) == 0x00) &&
            (read64(start_waddr + 8 ) == 0x10) &&
            (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("SEPARATE ENTRIES", check);
}

bool complex_tor_entries()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];
    
    clean_all_entries();
    uint16_t md[2] = {0,2};
    mdcfg_entry_config(2, 0);
    mdcfg_entry_config(5, 1);
    mdcfg_entry_config(9, 2);
    srcmd_entry_config(md, 2, 0, 0);

    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 256;

    //# Core writes 128 bytes of known values to memory 
    uintptr_t tmp = start_raddr;
    for(int i = 0; i < 128/8; i++){
        write64(tmp, i);
        tmp += 8;
    }

    fence_i();
    // Put 2 consecutive TOR entries allowing reads
    // Allows the test of the partial hit and jump to TOR_OP
    set_entry_off(start_raddr, ACCESS_NONE, 4);
    set_entry_tor(start_raddr + 24, ACCESS_READ, 5);
    set_entry_tor(start_raddr + 128, ACCESS_READ, 6);

    // Put 1 TOR entry allowing writes
    set_entry_off(start_waddr, ACCESS_NONE, 7);
    set_entry_tor(start_waddr + 128, ACCESS_WRITE, 8);

    VERBOSE("DMA transfer");
    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 128);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    bool check = 1;
    for(int i = 0; i < 128/8; i++){
        int value = read64(start_waddr);

        check &= (value == i);
        
        if(!check)
            ERROR("Error on position %d. Should be %d, is %d", i, i, value);

        start_waddr += 8;
    }

    TEST_ASSERT("", check);
    clean_all_entries();
}

/**
 *  Carry out a multi-beat DMA transfer with the IOPMP ON, using physical addresses
 *  As we are testing priority, we need that the configs from the priority entries surpass the one that matches down the line
 */
bool priority_tests()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];
    
    uint16_t md = 0;
    mdcfg_entry_config(NUMBER_ENTRIES, 0); // Give access to all entries, we are not testing that here
    srcmd_entry_config(&md, 1, 0, 0);
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 24;

    write32(IOPMP_REG_ADDR(IOPMP_HWCFG2_OFFSET), NUMBER_PRIO_ENTRIES); // Set number of prio entries

    bool check = 1;
    for (int j = 0; j < NUMBER_PRIO_ENTRIES; j++)
    {
        clean_all_entries();
        // Set prio_entry with no permissions for the written zone
        VERBOSE("Prio Entry: %d", j);

        for (int i = j + 1; i < NUMBER_ENTRIES; i++)
        {
            //INFO("Entry: %d", i);
            //# Core writes known values to memory
            write64(start_raddr     , 0x00);
            write64(start_raddr + 8 , 0x10);
            write64(start_raddr + 16, 0x20);

            write64(start_waddr     , 0x00);
            write64(start_waddr + 8 , 0x00);
            write64(start_waddr + 16, 0x00);

            // Set entry 0 with read and write permissions for the written zone
            set_entry_napot(start_raddr, 64, ACCESS_NONE, j);
            set_entry_napot(start_raddr, 64, ACCESS_READ | ACCESS_WRITE, i);

            //# Config iDMA and init transfer
            idma_setup(dma_ut, start_raddr, start_waddr, 24);
            if (idma_exec_transfer(dma_ut) != 0)
                {ERROR("iDMA misconfigured")}

            check &=    ((read64(start_waddr     ) == 0x00) &&
                        (read64(start_waddr + 8 ) == 0x00) &&
                        (read64(start_waddr + 16) == 0x00));
            
            // This way, more ranges are tested and previous entries will not interfere, and allows for a more thorough testing
            start_raddr += 64;
            start_waddr += 64;

            // Flush cache
            fence_i();
        }
    }

    TEST_ASSERT("", check);
}

/**
 * Carry out DMA transfers that must trigger an error in error recording interface.
 * */
bool error_recording()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    uint32_t reqinfo, reqid;
    bool check;
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 24;

    uint16_t md = 0;
    mdcfg_entry_config(32, 0);
    srcmd_entry_config(&md, 1, 0, 0);
    
    // Test not hit any rule ------------
    clean_all_entries();
    clean_error_reg();

    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    reqinfo = read_error_reqinfo();
    check  = (REQ_INFO_IP_MASK(reqinfo) == 1 &&
                REQ_INFO_ETYPE_MASK(reqinfo) == NOT_HIT_ERROR &&
                    REQ_INFO_TTYPE_MASK(reqinfo) == READ_ERROR);
    TEST_ASSERT("NOT HIT ANY RULE", check);

    // Test read error rule ------------
    clean_error_reg();

    enable_iopmp();
    set_entry_napot(start_raddr, 4, ACCESS_READ, 0);
    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    reqinfo = read_error_reqinfo();
    reqid   = read_error_reqid();
 
    check  = (REQ_INFO_IP_MASK(reqinfo) == 1 &&
                REQ_INFO_ETYPE_MASK(reqinfo) == READ_ERROR &&
                    REQ_INFO_TTYPE_MASK(reqinfo) == READ_ERROR &&
                        REQ_ID_EID(reqid) == 0);

    TEST_ASSERT("READ ERROR RULE", check);
}


#define N_BYTES 256
bool large_napot_entries()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    uint16_t md = 0;
    mdcfg_entry_config(5, 0);
    srcmd_entry_config(&md, 1, 0, 0);

    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + N_BYTES * 2;
    
    VERBOSE("Writing");
    //# Core writes known values to memory 
    for(int i = 0; i < N_BYTES/8; i++){
        write64(start_raddr, i);
        start_raddr += 8;
    }
    for(int i = 0; i < N_BYTES/8; i++){
        write64(start_waddr, 0);
        start_waddr += 8;
    }
    
    start_raddr = TEST_PPAGE_BASE;
    start_waddr = start_raddr + N_BYTES * 2;

    VERBOSE("Configuring entries");
    // // Set entry 0 with read and write permissions for the written zone
    set_entry_napot(start_raddr, N_BYTES, ACCESS_READ, 0);
    // // Set entry 0 with read and write permissions for the written zone
    set_entry_napot(start_waddr, N_BYTES, ACCESS_WRITE, 1);

    VERBOSE("DMA transfer");
    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, N_BYTES);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    for(int i = 0; i < N_BYTES/8; i++){
        int value = read64(start_waddr);
        if(value != i)
            VERBOSE("Error on position %d. Should be %d, is %d", i, i, value);

        start_waddr += 8;
    }

    
}

/**
 * 
 * Carry out multi-beat DMA transfers across multiple MDs belonging to the tested master
 * The transfers should be successfull, not triggering error recording
 * 
*/
bool full_iopmp_belonging_mds()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut[N_DMA];
    for (int i = 0; i < N_DMA; i++)
    {
        dma_ut[i] = (void*)idma_addr[idma_idx];
    }

    srand( SEED );
    uint64_t transaction, test_length;
    set_random_mds();
    set_random_sids();

    uintptr_t start_raddr = TEST_PPAGE_BASE;
    bool check = true;
    for (int i = 0; i < NUMBER_MASTERS; i++)
    {   
        INFO("Testing Master: %d", i);
        // Test belonging MDS
        for (int j = 0; j < MDS_PER_SID; j++)
        {
            uint64_t iterations = 0;
            uint8_t  start_entry = 0, end_entry = 0;

            if(sid_mds[i][j] != 0)
                start_entry = md_t[sid_mds[i][j]- 1];
            end_entry = md_t[sid_mds[i][j]];
            iterations = end_entry - start_entry;

            //INFO("Random entries");
            test_length = set_random_napot_entries(start_entry, end_entry - 1, ACCESS_READ | ACCESS_WRITE, start_raddr);
            
            //test_length = 24;
            uintptr_t tmp = start_raddr;
            //# Core writes known values to memory
            for(int i = 0; i < test_length/8; i++)
            {
                write64(tmp, i);

                tmp += 8;
            }

            VERBOSE("Iterations: %d", iterations);

            uintptr_t start_waddr = start_raddr;
            //# Config iDMA and init transfer, dependent on the configured entries
            // Test the full range of the MD
            // As we start at "1" because we jump the first length immediatly, make one less iteration
            for (int k = 0; k < iterations - 1; k++)
            {
                start_waddr += test_length;

                //INFO("Starting DMA %d with start_raddr: %x, start_waddr: %x, lenght: %d", i, start_raddr, start_waddr, test_length);
                idma_setup(dma_ut[i], start_raddr, start_waddr, test_length);
                if (idma_exec_transfer(dma_ut[i]) != 0)
                    {ERROR("iDMA misconfigured")}
                
                uint32_t reqinfo = read_error_reqinfo();
                check  &= (REQ_INFO_IP_MASK(reqinfo) == 0);
                VERBOSE("Check: %d", check);

                tmp = start_waddr;
                for(int i = 0; i < test_length/8; i++)
                {
                    int value = read64(tmp);
                    check = value != i;
                    // TEST_ASSERT("Error registered incorrectly", check);
                    if(check)
                        INFO("Error on position %d. Should be %d, is %d", i, i, value);
                        
                    tmp += 8;
                }

                clean_error_reg();
                // Flush cache
                fence_i();
            }

            // "Clean" entries, as they may affect testing as they are random, but share some regions inside the randomness
            set_multiple_napot_entries(start_entry, end_entry, ACCESS_READ | ACCESS_WRITE, MEM_BASE, 8);

        }
    }
    TEST_ASSERT("", check);
}

/**
 * 
 * Carry out multi-beat DMA transfers across multiple MDs not beloning to the tested master
 * The transfers should be unsuccessfull, triggering error recording
 * 
*/
bool full_iopmp_non_belonging_mds()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut[N_DMA];
    for (int i = 0; i < N_DMA; i++)
    {
        dma_ut[i] = (void*)idma_addr[idma_idx];
    }

    srand( SEED );
    uint64_t transaction, test_length;
    bool test_check = true;
    
    set_random_mds();
    set_random_sids();

    uintptr_t start_raddr = TEST_PPAGE_BASE;

    for (int i = 0; i < NUMBER_MASTERS; i++)
    {   
        INFO("Testing Master: %d", i);
        for (int j = 0; j < configured_mds; j++)
        {
            bool check = false;

            // Check if the md doesnt belong to the current master in test
            check = value_in_array(j, sid_mds[i], MDS_PER_SID);

            if (!check) // If it doesnt belong, test
            {
                uint64_t iterations = 0, length = 24;
                uint8_t  start_entry = 0, end_entry = 0;

                if(j != 0)
                    start_entry = md_t[j - 1];
                end_entry = md_t[j];
                iterations = end_entry - start_entry;

                // Give full permission, to actually test if the transfer fails, although multiple entries allowing
                test_length = set_random_napot_entries(start_entry, end_entry - 1, ACCESS_READ | ACCESS_WRITE, start_raddr);

                //# Config iDMA and init transfer, dependent on the configured entries
                // Test the full range of the MD
                // As we start at "1" because we jump the first length immediatly, make one less iteration
                for (int k = 0; k < iterations - 1; k++)
                {
                    uintptr_t start_waddr = start_raddr;

                    //INFO("Starting DMA %d with start_raddr: %x, start_waddr: %x, lenght: %d", i, start_raddr, start_waddr, test_length);
                    idma_setup(dma_ut[i], start_raddr, start_waddr, test_length);
                    if (idma_exec_transfer(dma_ut[i]) != 0)
                        {ERROR("iDMA misconfigured")}
                    
                    uint32_t reqinfo = read_error_reqinfo();
                    // Error on the reading of the DMA is already enough to assure us it went well
                    test_check  &= (REQ_INFO_IP_MASK(reqinfo) == 1) | REQ_INFO_TTYPE_MASK(reqinfo) == READ_ERROR;
                    //INFO("Check: %d", check);
                    

                    start_raddr += test_length;
                    clean_error_reg();
                    // Flush cache
                    fence_i();
                }

                // "Clean" entries, as they may affect testing as they are random, but share some regions inside the randomness
                set_multiple_napot_entries(start_entry, end_entry, ACCESS_READ | ACCESS_WRITE, MEM_BASE, 8);
            }
        }
    }
    TEST_ASSERT("", test_check);
}

bool idma_only()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    //# Set iopmp to on
    enable_iopmp();
    fence_i();
    VERBOSE("IOPMP ON");
    
    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    uint16_t md = 0;
    mdcfg_entry_config(260, 0);
    srcmd_entry_config(&md, 1, 0, 0);

    
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE+ 0x1000;
    uintptr_t start_waddr = start_raddr + N_BYTES * 2;
    
    VERBOSE("Writing");
    // Core writes known values to memory 
    for(int i = 0; i < N_BYTES/8; i++){
        write64(start_raddr, i);
        start_raddr += 8;
    }
    for(int i = 0; i < N_BYTES/8; i++){
        write64(start_waddr, 0);
        start_waddr += 8;
    }
    
    write64(start_raddr, 23);
    start_raddr = TEST_PPAGE_BASE+ 0x1000;
    start_waddr = start_raddr + N_BYTES * 2;

    VERBOSE("DMA transfer");
    //# Config iDMA and init transfer
    idma_setup(dma_ut, start_raddr, start_waddr, N_BYTES);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    for(int i = 0; i < N_BYTES/8; i++){
        int value = read64(start_waddr);
        if(value != i)
            INFO("Error on position %d. Should be %d, is %d", i, i, value);

        start_waddr += 8;
    }

    
}

// #define N_SAMPLES 20
// //uint32_t sizes[4] = {8, 512, 1024, 2024};
// uint32_t sizes[2] = {1, 0};

// bool general_latency_test(){
//     TEST_START();
//     uint64_t stamp_start = 0;
//     uint64_t stamp_end = 0;
//     uint64_t avg_lat = 0;
//     uint64_t stamp = 0;

//     //# Set iopmp to on
//     enable_iopmp();
//     fence_i();
//     VERBOSE("IOPMP ON");

//     uint16_t md = 0;
//     mdcfg_entry_config(260, 0);
//     srcmd_entry_config(&md, 1, 0, 0);
//     //# Get addresses
//     uintptr_t start_raddr = TEST_PPAGE_BASE;
//     uintptr_t start_waddr = start_raddr + 2048;

//     VERBOSE("Configuring entries");

//     for(int i = 0; i < 257; i *= 2){
//         printf("Entry %d\n", i);

//         // Set entry i with read and write permissions for the written zone
//         set_entry_napot(start_raddr, 4096, ACCESS_READ | ACCESS_WRITE, i);

//         for(int j = 0; j < 2; j++){
//             avg_lat = 0;

//             for(int x = 0; x < N_SAMPLES; x++){
//                 fence_i();
//                 VERBOSE("Writing");
//                 write64(start_raddr, 0xDEADBEEF);
//                 write64(start_waddr, 0);

//                 write64(idma_src[0], (uint64_t)start_raddr);    // Source address
//                 write64(idma_dest[0], (uint64_t)start_waddr);   // Destination address
//                 write64(idma_nbytes[0], sizes[j]);              // N of bytes to be transferred
//                 write64(idma_config[0], 0);                     // iDMA config: Disable decouple, deburst and serialize

//                 // Start stamp
//                 stamp_start = read_cycles();//CSRR(CSR_CYCLES);

//                 // Check if iDMA was set up properly and init transfer
//                 uint64_t trans_id = read64(idma_nextid[0]);
//                 if (!trans_id)
//                     {ERROR("iDMA misconfigured")}

//                 // Poll transfer status
//                 while (read64(idma_done[0]) != trans_id)
//                     ;

//                 // End stamp
//                 stamp_end = read_cycles();//CSRR(CSR_CYCLES);
                
//                 fence_i();
//                 if (read64(start_waddr) != 0xDEADBEEF)
//                     {ERROR("Transfer does not match")}

//                 avg_lat += (stamp_end - stamp_start);
//             }

//             printf("%d Size - Transfer average latency (in cycles): %llu\n", sizes[j], avg_lat/N_SAMPLES); ///N_SAMPLES);
//         }
//         set_entry_napot(0, 4096, ACCESS_READ | ACCESS_WRITE, i);
//         if( i == 0)
//             i = 1;
//     }

    
// }

// bool latency_testing_reader()
// {
//     TEST_START();
//     uint64_t stamp_start = 0;
//     uint64_t stamp_end = 0;
//     uint64_t avg_lat = 0;
//     uint64_t stamp = 0;

//     //# Set iopmp to on
//     fence_i();
//     //# Set iopmp to on
//     enable_iopmp();
//     fence_i();
//     VERBOSE("IOPMP ON");

//     uint16_t md = 0;
//     mdcfg_entry_config(260, 0);
//     srcmd_entry_config(&md, 1, 0, 0);

//     //# Get addresses
//     uintptr_t start_raddr = TEST_PPAGE_BASE;

//     uint64_t index = 0;
//     for(int i = 1; i < 257; i *= 2){
//         printf("Entry %d\n", i);

//         // Set entry i with read and write permissions for the written zone
//         set_entry_napot(start_raddr, 4096, ACCESS_READ | ACCESS_WRITE, i - 1);

//         for(int j = 0; j < 2; j++){
//             avg_lat = 0;

//             for(int x = 1; x < N_SAMPLES; x++){
//                 fence_i();
//                 write64(start_raddr, 0xDEADBEEF);

//                 write64(idma_nbytes[0], sizes[j]);              // N of bytes to be transferred
//                 write64(idma_src[0], (uint64_t)start_raddr);    // Source address

//                 // Start stamp
//                 stamp_start = read_cycles();//CSRR(CSR_CYCLES);
//                 write64(idma_dest[0], x + N_SAMPLES*index);   // Destination address
                

//                 // Poll transfer status
//                 while (read64(idma_done[0]) !=  x + N_SAMPLES*index)
//                     ;

//                 // End stamp
//                 stamp_end = read_cycles();//CSRR(CSR_CYCLES);
                
//                 fence_i();
//                 avg_lat += (stamp_end - stamp_start);
//                 // CVA6 node interconnect work-around: add delay to free axi bus (axi_node does not allow parallel transactions -> need to upgrade axi xbar)
//                 for (int i = 0; i < 100; i++)
//                     asm volatile ("nop" :  : ); // nop operation
//             }

//             index++;
//             printf("%d Size - Transfer average latency (in cycles): %llu\n", sizes[j], avg_lat/N_SAMPLES); ///N_SAMPLES);
//         }
//         set_entry_napot(0, 4096, ACCESS_READ | ACCESS_WRITE, i-1);
//     }
    
//     INFO("%d , %d", read64(idma_done[0]), index);

    
// }