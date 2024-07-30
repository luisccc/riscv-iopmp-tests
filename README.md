# RISCV IOPMP Architectural Tests

## License

This work is licensed under the Apache-2.0 License. See the [LICENSE](./LICENSE) file for details.

## Table of Contents

- [About this Project](#about-this-project)
- [Configuring the Tests](#configuring-the-tests)
- [Building the application](#building-the-application)

***

## About this Project 
This repository hosts a framework of architectural tests for the RISC-V IOPMP. It was developed as an adaptation of the [RISC-V Hypervisor extension tests](https://github.com/ninolomata/riscv-hyp-tests) framework to test a [RISC-V IOPMP IP](https://github.com/zero-day-labs/riscv-iopmp) within a [CVA6-based SoC](https://github.com/zero-day-labs/cva6/tree/feat/iopmp). To perform DMA transfers, we use the [PULP iDMA](https://github.com/pulp-platform/iDMA) module.

The Table below lists the included tests. Each one addresses one or more architectural features conforming to the [RISC-V IOPMP Specification](https://github.com/riscv-non-isa/iopmp-spec). All tests can be individually enabled or disabled before compilation.

| Test | Description |
|-|-|
|||
|**IOPMP Tests**|
| **iopmp_off** | Test the IOPMP in *OFF* mode. In this mode the IOPMP should not perform any checks.|
| **napot_smallest_lenght_entries** | Test the IOPMP support for the lowest supported grannularity (4 bytes), using a DMA transfer. |
| **simple_napot_entries** | Test the IOPMP support for napot entries. Perform a multi-beat transaction using a DMA transfer. 1- Read and Write fall inside the same entry; 2 - Read and Write in different entries.|
| **simple_tor_entries** | Test the IOPMP support for TOR entries. Perform a multi-beat transaction using a DMA transfer. 1- Read and Write fall inside the same entry; 2 - Read and Write in different entries.|
| **complex_tor_entries** | Test the IOPMP in partial hit contexts and having to load uncached entries. |
| **priority_tests** | Test the IOPMP support for priority entries. Test all non-priority entries vs priority entries, priority entries should overcome non-priority entries. |
| **error_recording** | Test the IOPMP error recording. |
| **large_napot_entries** | Test the IOPMP support for large regions protected by napot entries.|
|||
|**iDMA Tests**||
| **idma_only**| Test SoC with iDMA module directly connected to the XBAR, i.e., without IOPMP.|

## Configuring the Tests
Before building the application, you must configure some parameters according to the properties of your platform.

- In the [iopmp_tests.h](./src/inc/iopmp_tests.h) file, you can configure some IOPMP-related information. For example, **NUMBER_MASTERS** and **NUMBER_ENTRIES** define the range of entries and masters that will be used to execute the tests. You must configure these values according to the configurations in the actual IP or desired tests.

- You can disable/enable individual tests by commenting/uncommenting the corresponding line in the [test_register.c](./src/test_register.c) file.

- The base address of the programming interfaces of the IOPMP IP and the iDMA devices must be specified in **platform/`${PLAT}`/inc/platform.h** and **platform/`${PLAT}`/plat_dma.c**

## Building the application
:information_source: The **riscv64-unknown-elf-** toolchain must be in your `${PATH}` to build the application.

### Target platform

The target platform on which the test will run must be specified by setting the `${PLAT}` environment variable. Currently, we only support the [CVA6-based platform](https://github.com/zero-day-labs/cva6/tree/feat/iopmp).

| Platform | ${PLAT} |
| - | - |
| *CVA6* | `cva6` |

:information_source: Originally, the hypervisor extension test framework supported multiple platforms (QEMU, Rocket, CVA6). However, to the best of our knowledge, only the CVA6-based platform has support for the RISC-V IOPMP. We keep the platform definition mechanism to enable the integration of support for other platforms through further contributions.

### Output level

The output level can be specified via the `LOG_LEVEL` environment variable (default is `LOG_INFO`). 

Options include:
`LOG_NONE`, `LOG_ERROR`, `LOG_INFO`, `LOG_DETAIL`, `LOG_WARNING`, `LOG_VERBOSE`, `LOG_DEBUG`

### Compile

The build process is as follows:

```bash
$ git clone https://github.com/luisccc/riscv-iopmp-tests
$ cd riscv-iopmp-tests
$ make PLAT=${target_platform} LOG_LEVEL=3
```

The output files are located in the **build/`${PLAT}`** folder (**rv_iopmp_test.elf** and **rv_iopmp_test.bin**).
