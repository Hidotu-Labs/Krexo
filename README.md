# Krexo

Krexo is an x86_64 bootloader for BIOS and UEFI. It uses a request-based protocol to pass system information to the kernel, allowing for a decoupled architecture where the kernel defines its requirements via ELF sections.

## Features

- **Dual-Mode Boot**: Supports legacy BIOS (VBE) and modern UEFI (GOP) with high-resolution graphics.
- **Interactive Menu**: Configurable boot menu with automatic countdown and custom colors.
- **Request Protocol**: Limine-inspired interface. The kernel declares requests (Framebuffer, Memory Map) in the `.krexo_requests` section; the bootloader scans and fulfills them before handover.
- **FAT32 Support**: Built-in stack for loading the kernel and `krexo.conf` from the boot partition.
- **Memory Safety**: No-flicker UI using partial redraws (dirty stripes).

## Directory Structure

- `bios/`: 16-bit Stage 1 and 32-bit Stage 2 loaders.
- `uefi/`: 64-bit EFI application.
- `common/`: Shared logic for filesystems, protocols, and rendering.
- `include/`: Common headers for bootloader and kernel.
- `kernel/`: Example kernel implementing the request protocol.

## Build and Run

### Dependencies
- Toolchain: `nasm`, `x86_64-elf-gcc`, `clang`, `lld-link`
- Emulation: `qemu-system-x86_64`, `OVMF`

### Execution

**BIOS**:
```bash
make clean && make run_bios_full
```

**UEFI**:
```bash
make clean && make run_uefi_full
```

## Kernel Specification

The kernel specifies requirements using structures in the `.krexo_requests` ELF section.

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
