#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

unsigned long chip_shm_address_mapping(uint32_t self_id, uint32_t owner, unsigned long addr) {
  if (self_id == owner) {
    return addr;
  } else if (self_id == 0 && owner == 1) {
    if ((addr & 0x70000000) == 0x70000000) {
      return ((addr & 0x0FFFFFFF) | 0x30000000);
    } else if ((addr & 0x60000000) == 0x60000000) {
      return ((addr & 0x0FFFFFFF) | 0x20000000);
    }
    return addr;
  } else if (self_id == 1 && owner == 0) {
    if ((addr & 0x30000000) == 0x30000000) {
      return ((addr & 0x0FFFFFFF) | 0x70000000);
    }
    return addr;
  }
  return 0;
}

int bl_printf(const char *fmt, ...)
{
    static char buf[512];

    va_list argp;
    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf),fmt, argp);
    va_end(argp);
    return 0;
}

