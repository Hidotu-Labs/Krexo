#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <common/disk.h>
#include <common/kprint.h>

extern EFI_BOOT_SERVICES *gBS;
static EFI_FILE_PROTOCOL *gRoot = NULL;

static UINTN uefi_strlen(const char* str) {
    UINTN len = 0;
    while (str[len]) len++;
    return len;
}

int uefi_disk_init(krexo_disk_t *disk) {
    EFI_STATUS status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    // Locate the filesystem protocol
    status = gBS->LocateProtocol(&fs_guid, NULL, (void**)&fs);
    if (EFI_ERROR(status)) {
        kprintf("UEFI: No FAT32 filesystem protocol found\n");
        return -1;
    }

    // Open the root directory
    status = fs->OpenVolume(fs, &gRoot);
    if (EFI_ERROR(status)) {
        kprintf("UEFI: Failed to open volume\n");
        return -1;
    }

    
    disk->drive_id = 0;
    disk->read_sectors = NULL; 
    return 0;
}

int uefi_read_file(const char* filename, void* buffer, UINTN* size) {
    if (!gRoot) return -1;

    CHAR16 name16[256];
    UINTN i = 0;
    for (i = 0; filename[i] && i < 255; i++) name16[i] = (CHAR16)filename[i];
    name16[i] = 0;

    EFI_FILE_PROTOCOL *file = NULL;
    EFI_STATUS status = gRoot->Open(gRoot, &file, name16, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) return -1;

    status = file->Read(file, size, buffer);
    file->Close(file);

    return EFI_ERROR(status) ? -1 : 0;
}
