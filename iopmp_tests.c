#include <iopmp_tests.h>
#include <rv_iopmp.h>
#include <idma.h>

/**
 *  Carry out a multi-beat DMA transfer with the IOPMP OFF, using physical addresses
 *  Check if all the transfers fail
 */
bool iopmp_off()
{

    TEST_START();

    fence_i();
    VERBOSE("IOPMP OFF");

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

    //# Config iDMA and init transfer
    idma_setup(0, start_raddr1, start_waddr1, 24);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    bool check;
    check =    ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x00) &&
                (read64(start_waddr1 + 16) == 0x00));
    TEST_ASSERT("iDMA Memory transfer with IOPMP OFF successfull, should fail", check);
    TEST_END();
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

    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 32;

    //# Core writes known values to memory
    //write64(start_waddr     , 0x00);
    write64(start_raddr     , 0xFFFF);

    // Set entry 0 with read permissions
    set_entry_napot(start_raddr, 8, ACCESS_READ, 0);
    // Set entry 1 with write permissions
    set_entry_napot(start_waddr, 8, ACCESS_WRITE, 1);

    //# Config iDMA and init transfer
    idma_setup(0, start_raddr, start_waddr, 4);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    bool check;
    check = (read64(start_waddr) == 0xFFFF);
    TEST_ASSERT("iDMA Memory transfer with IOPMP napot entry 0, ended with incorrect values", check);
    TEST_END();
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
    idma_setup(0, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    bool check;
    check =    ((read64(start_waddr     ) == 0x00) &&
                (read64(start_waddr + 8 ) == 0x10) &&
                (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("iDMA Memory transfer with IOPMP napot entry 0, ended with incorrect values", check);

    start_waddr = start_raddr + 64;

    // Set entry 0 with read permissions
    set_entry_napot(start_raddr, 32, ACCESS_READ, 0);
    // Set entry 1 with write permissions
    set_entry_napot(start_waddr, 32, ACCESS_WRITE, 1);

    //# Config iDMA and init transfer
    idma_setup(0, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    check =    ((read64(start_waddr     ) == 0x00) &&
                (read64(start_waddr + 8 ) == 0x10) &&
                (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("iDMA Memory transfer with IOPMP napot entry 0 and 1, ended with INcorrect values", check);

    TEST_END();
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
    idma_setup(0, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    bool check = ((read64(start_waddr    ) == 0x00) &&
                 (read64(start_waddr + 8 ) == 0x10) &&
                 (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("iDMA Memory transfer with IOPMP TOR entry 0 and 1, ended with incorrect values", check);

    // Now test four entries with distinct areas
    start_waddr = start_raddr + 64;

    write64(start_waddr     , 0x00);
    write64(start_waddr + 8 , 0x00);
    write64(start_waddr + 16, 0x00);

    set_entry_off(start_raddr, ACCESS_READ, 0);
    set_entry_tor(start_raddr + 32, ACCESS_READ, 1);

    set_entry_off(start_waddr, ACCESS_WRITE, 2);
    set_entry_tor(start_waddr + 32, ACCESS_WRITE, 3);

    //# Config iDMA and init transfer
    idma_setup(0, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    check = ((read64(start_waddr     ) == 0x00) &&
            (read64(start_waddr + 8 ) == 0x10) &&
            (read64(start_waddr + 16) == 0x20));

    TEST_ASSERT("iDMA Memory transfer with IOPMP TOR entry 0 and 1, ended with incorrect values", check);

    TEST_END();
}

/**
 *  Carry out a multi-beat DMA transfer with the IOPMP ON, using physical addresses
 *  As we are testing priority, we need that the configs from the previous entry surpass the one that matches down the line
 */
bool priority_tests()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");
    
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 24;

    for (int i = 1; i < NUMBER_ENTRIES; i++)
    {
        //# Core writes known values to memory
        write64(start_raddr     , 0x00);
        write64(start_raddr + 8 , 0x10);
        write64(start_raddr + 16, 0x20);

        write64(start_waddr     , 0x00);
        write64(start_waddr + 8 , 0x00);
        write64(start_waddr + 16, 0x00);

        // Set entry 0 with read and write permissions for the written zone
        set_entry_napot(start_raddr, 64, ACCESS_NONE, i - 1);
        set_entry_napot(start_raddr, 64, ACCESS_READ | ACCESS_WRITE, i);

        //# Config iDMA and init transfer
        idma_setup(0, start_raddr, start_waddr, 24);
        if (idma_exec_transfer(0) != 0)
            {ERROR("iDMA misconfigured")}

        bool check;
        check =    ((read64(start_waddr     ) == 0x00) &&
                    (read64(start_waddr + 8 ) == 0x00) &&
                    (read64(start_waddr + 16) == 0x00));

        // This way, previous entries will not interfere, and allows for a more thorough testing
        start_raddr += 64;
        start_waddr += 64;

        TEST_ASSERT("iDMA Memory transfer successfull, but should have been blocked", check);
    }

    for (int i = 0; i < NUMBER_ENTRIES - 1; i++)
    {
        //# Core writes known values to memory
        write64(start_raddr     , 0x00);
        write64(start_raddr + 8 , 0x10);
        write64(start_raddr + 16, 0x20);

        // Set entry 0 with read and write permissions for the written zone
        set_entry_napot(start_raddr, 64, ACCESS_NONE, i + 1);
        set_entry_napot(start_raddr, 64, ACCESS_READ | ACCESS_WRITE, i);

        //# Config iDMA and init transfer
        idma_setup(0, start_raddr, start_waddr, 24);
        if (idma_exec_transfer(0) != 0)
            {ERROR("iDMA misconfigured")}

        bool check;
        check = ((read64(start_waddr    ) == 0x00) &&
                (read64(start_waddr + 8 ) == 0x10) &&
                (read64(start_waddr + 16) == 0x20));

        start_raddr += 64;
        start_waddr += 64;

        TEST_ASSERT("iDMA Memory transfer unsuccessfull, but should have been successfull", check);
    }

    TEST_END();
}

bool error_recording()
{
    TEST_START();

    //# Set iopmp to on
    fence_i();
    enable_iopmp();
    VERBOSE("IOPMP ON");

    uint32_t reqinfo, reqid;
    bool check;
    //# Get addresses
    uintptr_t start_raddr = TEST_PPAGE_BASE;
    uintptr_t start_waddr = start_raddr + 24;

    // Test not hit any rule ------------
    clean_all_entries();
    clean_error_reg();

    //# Config iDMA and init transfer
    idma_setup(0, start_raddr, start_waddr, 24);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    reqinfo = read_error_reqinfo();
    check  = (REQ_INFO_IP_MASK(reqinfo) == 1 &&
                REQ_INFO_ETYPE_MASK(reqinfo) == NOT_HIT_ERROR &&
                    REQ_INFO_TTYPE_MASK(reqinfo) == READ_ERROR);
    TEST_ASSERT("Error not registered correctly", check);

    // Test read error rule ------------
    clean_error_reg();

    enable_iopmp();
    set_entry_napot(start_raddr, 4, ACCESS_READ, 0);
    //# Config iDMA and init transfer
    idma_setup(0, start_raddr, start_waddr, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    reqinfo = read_error_reqinfo();
    reqid   = read_error_reqid();
 
    check  = (REQ_INFO_IP_MASK(reqinfo) == 1 &&
                REQ_INFO_ETYPE_MASK(reqinfo) == READ_ERROR &&
                    REQ_INFO_TTYPE_MASK(reqinfo) == READ_ERROR &&
                        REQ_ID_EID(reqid) == 0);

    TEST_ASSERT("Error not registered correctly", check);

    TEST_END();
}