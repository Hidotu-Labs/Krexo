# Krexo Bootloader

Krexo is a modular x86_64 bootloader supporting both BIOS and UEFI environments. It is designed to share as much code as possible between platforms through a common C library.

## Features

- BIOS Support: 16-bit Stage 1 transitioning into a 32-bit Protected Mode C Stage 2.
- UEFI Support: 64-bit EFI application using EDK2 headers.
- Shared Core: Common C implementations for core functions like printf and string handling.
- Toolchain: Uses Clang/LLD for UEFI and GCC/NASM for BIOS.

## Repository Structure

- `bios/`: BIOS stage 1 (assembly) and stage 2 (C).
- `uefi/`: UEFI entry point and platform drivers.
- `common/`: Platform-independent logic and libraries.
- `include/`: Public and internal header files.
- `lib/edk2/`: TianoCore EDK2 headers.

## Building and Running

### Requirements
- NASM
- x86_64-elf-gcc
- Clang & LLD
- QEMU

### BIOS
```bash
make run_bios
```

### UEFI
```bash
make run_uefi
```

## Roadmap

- Memory Map: Unified memory region reporting.
- Framebuffer: GOP (UEFI) and VBE (BIOS) graphical output.
- Filesystem: FAT32 driver for loading configurations and kernels.
- ELF64: Executable loader for the final kernel.
- Protocol: Specification for passing boot parameters to the OS.
