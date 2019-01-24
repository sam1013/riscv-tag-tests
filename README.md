# TIMBER-V Tests

This is a subproject of [TIMBER-V Toolchain][timberv-riscv-tools].

This project contains various tests for the TIMBER-V Spike simulator:

* `mpu_basic` contains basic tests for the MPU (the PMP) functionality
* `mpu_tag` contains advanced tests combining the MPU with memory tagging
* This project is based on the RISC-V proxy kernel. As such, it contains 
  lots of code not needed for the tag tests.

## Build
To build the project, follow these steps:

* Install the TIMBER-V compiler to `/opt/riscv/64`, as described in [timberv-riscv-tools]
* Add the TIMBER-V compiler to your `PATH`, e.g. `export PATH=/opt/riscv/64/bin:$PATH`
* Create a `build64` directory
* From inside `build64` execute `../configure --prefix=/opt/riscv/64 --host=riscv64-unknown-elf

## Run
* From inside `build64` execute `spike mpu_basic` or `spike mpu_tag`, respectively

## Known issues
* The project is incompatible with 32-bit mode

[timberv-riscv-tools]: https://github.com/sam1013/timberv-riscv-tools/tree/timberv
