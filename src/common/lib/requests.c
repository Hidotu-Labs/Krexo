#include <common/fb.h>
#include <common/kprint.h>
#include <common/requests.h>
#include <stddef.h>

// A simple bump allocator for responses in a safe region
// We'll put responses at 0x80000 (Safe area below 1MB/Kernel)
static uint64_t response_ptr = 0x80000;

static void *allocate_response(size_t size) {
  void *ptr = (void *)(uintptr_t)response_ptr;
  response_ptr += size;
  return ptr;
}

void requests_handle(void *kernel_start, size_t kernel_size, krexo_fb_t *fb,
                     krexo_mmap_t *mmap, const char *cmdline) {
  uint64_t *ptr = (uint64_t *)kernel_start;
  size_t count = kernel_size / 8;

  uint64_t start_marker[] = KREXO_REQUESTS_START_MARKER;
  uint64_t end_marker[] = KREXO_REQUESTS_END_MARKER;

  size_t start_index = 0;
  size_t end_index = 0;

  // 1. Find the markers
  for (size_t i = 0; i < count - 1; i++) {
    if (ptr[i] == start_marker[0] && ptr[i + 1] == start_marker[1]) {
      start_index = i + 2; // Jump past the marker
    }
    if (ptr[i] == end_marker[0] && ptr[i + 1] == end_marker[1]) {
      end_index = i;
      break;
    }
  }

  // Fallback if markers are missing (allow migration to be gradual)
  if (start_index == 0 || end_index == 0 || start_index >= end_index) {
    kprintf("[BOOT] Warning: Request markers NOT found. Falling back to full "
            "scan.\n");
    start_index = 0;
    end_index = count - 3;
  } else {
    kprintf("[BOOT] Request markers found! Scanning range: 0x%x - 0x%x\n",
            start_index * 8, end_index * 8);
  }

  // 2. Scan for requests
  for (size_t i = start_index; i < end_index; i++) {
    if (ptr[i] == KREXO_REQUEST_MAGIC_0 &&
        ptr[i + 1] == KREXO_REQUEST_MAGIC_1) {
      // Found a request! Check ID
      uint64_t id_0 = ptr[i + 2];
      uint64_t id_1 = ptr[i + 3];

      // Framebuffer Request ID
      uint64_t fb_id[] = KREXO_FB_REQUEST_ID;
      if (id_0 == fb_id[0] && id_1 == fb_id[1]) {
        krexo_fb_response_t *resp =
            allocate_response(sizeof(krexo_fb_response_t));
        resp->address = fb->base;
        resp->width = fb->width;
        resp->height = fb->height;
        resp->pitch = fb->pitch;
        resp->bpp = fb->bpp;

        // Set the response pointer (ptr[i+4] is the response field)
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
      }

      // Memory Map Request ID
      uint64_t mmap_id[] = KREXO_MMAP_REQUEST_ID;
      if (id_0 == mmap_id[0] && id_1 == mmap_id[1]) {
        krexo_mmap_response_t *resp =
            allocate_response(sizeof(krexo_mmap_response_t));
        resp->entry_count = mmap->count;
        resp->entries = mmap->entries;

        // Set the response pointer
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
      }

      // Kernel File Request ID
      uint64_t kfile_id[] = KREXO_KERNEL_FILE_REQUEST_ID;
      if (id_0 == kfile_id[0] && id_1 == kfile_id[1]) {
        krexo_kernel_file_response_t *resp =
            allocate_response(sizeof(krexo_kernel_file_response_t));

        // Copy cmdline to safe memory
        size_t cmd_len = 0;
        while (cmdline[cmd_len])
          cmd_len++;
        char *cmd_buf = allocate_response(cmd_len + 1);
        for (size_t j = 0; j < cmd_len; j++)
          cmd_buf[j] = cmdline[j];
        cmd_buf[cmd_len] = 0;

        resp->cmdline = (uint64_t)(uintptr_t)cmd_buf;
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
      }

      // HHDM Request ID
      uint64_t hhdm_id[] = KREXO_HHDM_REQUEST_ID;
      if (id_0 == hhdm_id[0] && id_1 == hhdm_id[1]) {
        krexo_hhdm_response_t *resp =
            allocate_response(sizeof(krexo_hhdm_response_t));
        resp->offset = KREXO_HHDM_OFFSET;
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
        kprintf("[BOOT] HHDM request fulfilled: offset=0x%x\n",
                (uint32_t)(KREXO_HHDM_OFFSET >> 32));
      }
    }
  }
}
