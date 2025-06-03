
#include <stdint.h>
#include <stdio.h>
#include <xz.h>
#include <string.h>

#define XZ_READBUF_SIZE  4096
#define XZ_WRITEBUF_SIZE 4096
#define printf(...)
/****************************************************************************/ /**
 * @brief  Decompress XZ Firmware
 *
 * @param  cache       : cache on ram
 * @param  cache_size  : cache size
 * @param  src         : Source address on flash
 * @param  dest        : Destination address on flash
 * @param  p_dest_size : Pointer for output destination file size
 *
 * @return Decompress result status, 0:success; -1 or other:error
 *
*******************************************************************************/

int xz_decompress(
        uint8_t *cache, uint32_t cache_size,
        uint8_t *src, uint8_t *dest, uint32_t *p_dest_size)
{
    struct xz_buf b;
    struct xz_dec *s;
    enum xz_ret ret;

    printf("cache = %p, cache_size = 0x%08lx, src = %p, dest = %p, dest_size = 0x%08lx\r\n",
            cache, cache_size, src, dest, *p_dest_size);

    *p_dest_size = 0;

    xz_crc32_init();
    simple_malloc_init(cache + XZ_READBUF_SIZE + XZ_WRITEBUF_SIZE,
            cache_size - XZ_READBUF_SIZE - XZ_WRITEBUF_SIZE);

    /*
    * Support up to 32k dictionary. The actually needed memory
    * is allocated once the headers have been parsed.
    */
    s = xz_dec_init(XZ_PREALLOC, 1 << 15);

    if (s == NULL) {
        printf("Memory allocation failed\n");
        return -1;
    }

    b.in = cache;
    b.in_pos = 0;
    b.in_size = 0;
    b.out = cache + XZ_READBUF_SIZE;
    b.out_pos = 0;
    b.out_size = XZ_WRITEBUF_SIZE;

    while (1) {
        if (b.in_pos == b.in_size) {
            printf("XZ Feeding src = %p, size = %ld\r\n", src, XZ_READBUF_SIZE);
            memcpy((uint8_t *)b.in, src, XZ_READBUF_SIZE);

            b.in_size = XZ_READBUF_SIZE;
            b.in_pos = 0;
            src += XZ_READBUF_SIZE;
        }

        ret = xz_dec_run(s, &b);

        if (b.out_pos == XZ_WRITEBUF_SIZE) {
            printf("XZ outputing dest = %p, size = %ld / %ld\r\n",
                    dest, XZ_WRITEBUF_SIZE, *p_dest_size);
            memcpy(dest, b.out, XZ_WRITEBUF_SIZE);

            dest += XZ_WRITEBUF_SIZE;
            *p_dest_size += XZ_WRITEBUF_SIZE;
            b.out_pos = 0;
        }

        if (ret == XZ_OK) {
            continue;
        } else {
            printf("ret = %d\r\n", ret);
        }

        if (b.out_pos > 0) {
            memcpy(dest, b.out, b.out_pos);

            dest += b.out_pos;
            *p_dest_size += b.out_pos;
        }
        break;
    }

    switch (ret) {
        case XZ_STREAM_END:
            printf("XZ_STREAM_END\r\n");
            break;
        case XZ_MEM_ERROR:
            printf("Memory allocation failed\n");
            break;
        case XZ_MEMLIMIT_ERROR:
            printf("Memory usage limit reached\n");
            break;
        case XZ_FORMAT_ERROR:
            printf("Not a .xz file\n");
            break;
        case XZ_OPTIONS_ERROR:
            printf("Unsupported options in the .xz headers\n");
            break;
        case XZ_DATA_ERROR:
        case XZ_BUF_ERROR:
            printf("File is corrupt\n");
            break;
        default:
            printf("XZ Bug!\n");
            break;
    }

    xz_dec_end(s);

    return 0;
}

