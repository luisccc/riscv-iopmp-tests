#include <rv_iopmp.h>
#include <iopmp_tests.h>

/**
 *  IOPMP Memory-mapped registers 
 */
// hwcfg0
uintptr_t hwcfg0_addr = IOPMP_REG_ADDR(IOPMP_HWCFG0_OFFSET);

// hwcfg1
uintptr_t hwcfg1_addr = IOPMP_REG_ADDR(IOPMP_HWCFG1_OFFSET);

// hwcfg2
uintptr_t hwcfg2_addr = IOPMP_REG_ADDR(IOPMP_HWCFG2_OFFSET);

// err_reqinfo
uintptr_t err_reqinfo_addr = IOPMP_REG_ADDR(IOPMP_ERR_REQINFO_OFFSET);

// err_reqid
uintptr_t err_reqid_addr = IOPMP_REG_ADDR(IOPMP_ERR_REQID_OFFSET);

// err_reqaddr
uintptr_t err_reqaddr_addr = IOPMP_REG_ADDR(IOPMP_ERR_REQADDR_OFFSET);

// entry addr
uintptr_t entry_addr_addr[8] = {
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 0*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 1*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 2*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 3*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 4*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 5*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 6*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_ADDR_OFFSET + 7*16)
};

// entry cfg
uintptr_t entry_cfg_addr[8] = {
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 0*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 1*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 2*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 3*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 4*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 5*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 6*16),
    IOPMP_REG_ADDR(IOPMP_ENTRY_CFG_OFFSET + 7*16)
};


void enable_iopmp()
{
    uint32_t config = 0x80000000; // Current spec the enable bit is the last bit of the HWCFG
    
    write32(hwcfg0_addr, config);
}

void entry_config(uint64_t addr, uint8_t mode, uint8_t access, uint8_t entry_num)
{
    write64(entry_addr_addr[entry_num], addr >> 2);

    uint32_t config = ((mode & 0x3) << 3) + (access & 0x7);
    write32(entry_cfg_addr[entry_num], config);
}

// TODO: Check if length is a multiple of the base address
void set_entry_napot(uint64_t base_addr, uint64_t length, uint8_t access, uint8_t entry_num)
{
    uint8_t mode;
    
    if(length < 8) mode = MODE_NA4;
    else mode = MODE_NAPOT;

    uint64_t addr = (base_addr + length/2 - 1);

    entry_config(addr, mode, access, entry_num);
}

void set_entry_tor(uint64_t base_addr, uint8_t access, uint8_t entry_num)
{
    uint8_t mode;
    
    mode = MODE_TOR;

    entry_config(base_addr, mode, access, entry_num);
}

void set_entry_off(uint64_t base_addr, uint8_t access, uint8_t entry_num)
{
    uint8_t mode;
    
    mode = MODE_OFF;

    entry_config(base_addr, mode, access, entry_num);
}

void clean_all_entries()
{
    for(int i  = 0; i < NUMBER_ENTRIES; i++)
    {
        set_entry_off(0, MODE_OFF, i);
    }
}

void clean_error_reg()
{
    uint32_t config = 0x1; // Current spec the ip bit is the first bit of the err_reqinfo
    
    write32(err_reqinfo_addr, config);
}

void wait()
{
    for (int i = 0; i < 500; i++)
    {
        __asm__ volatile(
            "nop");
    }
}

uint32_t read_error_reqinfo()
{
    uint32_t result = read32(err_reqinfo_addr);

    return result;
}

uint32_t read_error_reqid()
{
    uint32_t result = read32(err_reqid_addr);

    return result;
}

uint32_t read_error_reqaddr()
{
    uint32_t result = read32(err_reqaddr_addr);

    return result;
}