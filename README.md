# TIMBER-V Tests

This project contains various tests for the TIMBER-V spike simulator:

* `mpu_basic` contains basic tests for the MPU (the PMP) functionality
* `mpu_tag` contains advanced tests combining the MPU with memory tagging
* This project is based on the RISC-V proxy kernel. As such, it contains 
  lots of code not needed for the tag tests.

## Build
To build the project, follow these steps:

* add riscv compiler to PATH, e.g. `export PATH=$PATH:/opt/riscv/bin`
* generate `build` directory
* from inside `build` execute `../configure --prefix=/opt/riscv/ --host=riscv64-unknown-elf

## Run

* add riscv compiler to PATH, e.g. `export PATH=$PATH:/opt/riscv/bin`
* from inside `build` execute `spike mpu_basic` or `spike mpu_tag`, respectively

## Known issues
* The project is incompatible with 32-bit mode
