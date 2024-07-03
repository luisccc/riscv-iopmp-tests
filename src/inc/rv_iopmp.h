#ifndef _RV_IOPMP_H_
#define _RV_IOPMP_H_

#include "iopmp_tests.h"

#define MODE_OFF        (0ULL)
#define MODE_TOR        (1ULL << 3)
#define MODE_NA         (2ULL << 3)
#define MODE_NA4        (2ULL << 3)
#define MODE_NAPOT      (3ULL << 3)

#define ACCESS_NONE     (0ULL)
#define ACCESS_READ     (1ULL)
#define ACCESS_WRITE    (2ULL)
#define ACCESS_EXEC     (4ULL)

#define NO_ERROR            (0ULL)
#define READ_ERROR          (1ULL)
#define WRITE_ERROR         (2ULL)
#define EXECUTION_ERROR     (3ULL)
#define PARTIAL_ERROR       (4ULL)
#define NOT_HIT_ERROR       (5ULL)
#define UNKOWN_SID_ERROR    (6ULL)

#define IOPMP_VERSION_OFFSET        0x0
#define IOPMP_IMP_OFFSET            0x4
#define IOPMP_HWCFG0_OFFSET         0x8
#define IOPMP_HWCFG1_OFFSET         0xc
#define IOPMP_HWCFG2_OFFSET         0x10
#define IOPMP_ENTRY_OFFSET_OFFSET   0x14
#define IOPMP_ERRREACT_OFFSET       0x18
#define IOPMP_MDCFGLCK_OFFSET       0x48
#define IOPMP_ENTRYLCK_OFFSET       0x4c
#define IOPMP_ERR_REQINFO_OFFSET    0x60
#define IOPMP_ERR_REQID_OFFSET      0x64
#define IOPMP_ERR_REQADDR_OFFSET    0x68
#define IOPMP_ERR_REQADDRH_OFFSET   0x6c
#define IOPMP_MDCFG_OFFSET          0x800
#define IOPMP_SRCMD_EN_OFFSET       0x1000
#define IOPMP_SRCMD_ENH_OFFSET      0x1004
#define IOPMP_ENTRY_ADDR_OFFSET     0x2000
#define IOPMP_ENTRY_ADDRH_OFFSET    0x2004
#define IOPMP_ENTRY_CFG_OFFSET      0x2008

#define IOPMP_REG_ADDR(OFF)     (IOPMP_BASE_ADDR + OFF)

#define REQ_INFO_IP_MASK(value)     (value & 0x1)
#define REQ_INFO_TTYPE_MASK(value)  ((value >> 1) & 0x3)
#define REQ_INFO_ETYPE_MASK(value)  ((value >> 4) & 0x7)

#define REQ_ID_SID(value)           (value & 0xffff)
#define REQ_ID_EID(value)           ((value >> 16) & 0xffff)

void enable_iopmp();
int iopmp_entry_set(unsigned int n, unsigned long prot, unsigned long addr,
	    unsigned long log2len);
void iopmp_mdcfg_config(unsigned int n, unsigned int t);
void iopmp_srcmd_config(unsigned int n, uint64_t mds_bmap, uint8_t lock);
void entry_config(uint64_t addr, uint8_t mode, uint8_t access, uint16_t entry_num);
void set_entry_napot(uint64_t base_addr, uint64_t length, uint8_t access, uint16_t entry_num);
void set_entry_tor(uint64_t base_addr, uint8_t access, uint16_t entry_num);
void set_entry_off(uint64_t base_addr, uint8_t access, uint16_t entry_num);
void srcmd_entry_config(uint16_t* mds, uint8_t number_mds, uint8_t lock, uint8_t entry_num);
void mdcfg_entry_config(uint16_t t, uint8_t entry_num);
void clean_all_entries();

int entry_set(unsigned int n, unsigned long prot, unsigned long addr,
	    unsigned long log2len);

void clean_error_reg();
uint32_t read_error_reqinfo();
uint32_t read_error_reqid();
uint64_t read_error_reqaddr();

#endif  /* _RV_IOPMP_H_ */