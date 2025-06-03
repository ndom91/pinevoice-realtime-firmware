#if 1
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

static uint8_t *mallocBuf = NULL;
static uint32_t malloced = 0;
static uint32_t bufsize = 0;

void simple_malloc_init(uint8_t *buf, uint32_t len)
{
    mallocBuf = buf;
    malloced = 0;
    bufsize = len;
}
void *simple_malloc(uint32_t size)
{
    uint8_t *p;
    printf("Simple Malloc %d\r\n", size);

    if (malloced + size < bufsize) {
        p = mallocBuf + malloced;
        malloced += size;
        return p;
    }

    return NULL;
}

void simple_free(void *p)
{
    printf("Simple Free %08x\r\n", p);
    return;
}
#else
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <aos/kernel.h>

static uint8_t *mallocBuf = NULL;
static uint32_t malloced = 0;
static uint32_t bufsize = 0;

void simple_malloc_init(uint8_t *buf, uint32_t len)
{
}
void *simple_malloc(uint32_t size)
{
    return aos_malloc(size);
}

void simple_free(void *p)
{
    aos_free(p);
}
#endif

