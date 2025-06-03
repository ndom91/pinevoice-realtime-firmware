
#ifndef __XZ_API_H__
#define __XZ_API_H__

#include <stdint.h>

int xz_decompress(
        uint8_t *cache, uint32_t cache_size,
        uint8_t *src, uint8_t *dest, uint32_t *p_dest_size);

#endif
