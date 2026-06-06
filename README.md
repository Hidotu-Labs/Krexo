# Krexo v1.2.2

Krexo is a high-performance x86_64 bootloader for BIOS and UEFI. It uses a request-based protocol to pass system information to the kernel, allowing for a decoupled architecture where the kernel defines its requirements via ELF sections.

## Major Changes in v1.2.x
- **Removed Limine compability**: IT IS NOT COMPATIBLE WITH LIMINE ANYMORE.
- **Started SMP Support**: Started working on SMP Support

## Features

- **Dual-Mode Boot**: Supports legacy BIOS (VBE) and modern UEFI (GOP) with high-resolution graphics.
- **Interactive Menu**: Fully configurable boot menu with automatic countdown and custom themes.
- **Protocol Support**:
  - **Native Krexo**: A request-based interface declarations via ELF sections.
  - **Limine Compat**: Support for Limine protocol requests (Framebuffer, Memory Map, HHDM, etc.).
- **FAT32 Support**: Built-in stack for loading the kernel and `krexo.conf` from the boot partition.
- **Memory Safety**: No-flicker UI using partial redraws (dirty stripes).

## Roadmap

We are working on expanding Krexo's capabilities:

- **SMP Support**: Symmetrical Multiprocessing to wake up and manage multiple CPU cores.
- **Multiboot2 Support**: Implementation of the Multiboot2 protocol for broader kernel interoperability.
- **Linux-Boot**: Direct support for loading Linux kernels (bzImage) and initrd.
- **Limine-Protocol**: Direct support for Limine protocol requests (Framebuffer, Memory Map, HHDM, etc.).
- **Advanced Filesystems**: Adding read support for EXT4 and ISO9660.
- **Security**: UEFI Secure Boot support and signature verification.

## Directory Structure

- `src/bios/`: 16-bit Stage 1 and 32-bit Stage 2 loaders.
- `src/uefi/`: 64-bit EFI application.
- `src/common/`: Shared logic for filesystems, protocols, and rendering.
- `include/`: Common headers for bootloader and kernel.
- `barebones/`: Reference kernel implementations.

## Build and Run

### Dependencies
- Toolchain: `nasm`, `x86_64-elf-gcc`, `clang`, `lld-link`
- Emulation: `qemu-system-x86_64`, `OVMF`

### Execution

**BIOS**:
```bash
make clean && make run_bios
```

**UEFI**:
```bash
make clean && make run_uefi
```

## Kernel Specification

The kernel specifies requirements using structures in the `.krexo_requests` ELF section (Native) or via Limine tags.

```c
#include <common/requests.h>

__attribute__((section(".krexo_requests")))
volatile krexo_fb_request_t fb_request = {
    .header = { 
        .magic = { KREXO_REQUEST_MAGIC_0, KREXO_REQUEST_MAGIC_1 },
        .id = KREXO_FB_REQUEST_ID, 
        .response = 0 
    }
};
```

On handover, the `response` pointer will contain the address of the fulfilled hardware information.

