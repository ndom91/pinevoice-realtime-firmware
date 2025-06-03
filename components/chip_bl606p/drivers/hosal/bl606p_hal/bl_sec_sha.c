#include <stdio.h>

#include <bl606p_sec_eng.h>

#include "bl_os_system.h"
#include "bl_irq.h"
#include "bl_sec.h"

#define blog_error(...)

typedef struct sha256_link_item {
    SEC_Eng_SHA256_Link_Ctx ctx;
    SEC_Eng_SHA_Link_Config_Type linkCfg;
    uint32_t tmp[16];
    uint32_t pad[16];
} sha256_link_item_t;

#define BL_SHA_ID SEC_ENG_SHA_ID0 // this is the only valid value

int bl_sha_mutex_take()
{
    if (0 != bl_os_mutex_lock(g_bl_sec_sha_mutex)) {
        blog_error("sha semphr take failed\r\n");
        return -1;
    }
    return 0;
}

int bl_sha_mutex_give()
{
    if (0 != bl_os_mutex_unlock(g_bl_sec_sha_mutex)) {
        blog_error("sha semphr give failed\\n");
        return -1;
    }
    return 0;
}

void bl_sha_init(bl_sha_ctx_t *ctx, const bl_sha_type_t type)
{
    const SEC_ENG_SHA_Type sha_type = (SEC_ENG_SHA_Type)type; // bl_sha_type_t is the same as SEC_ENG_SHA_Type in driver

    Sec_Eng_SHA256_Init((SEC_Eng_SHA256_Ctx *)&ctx->sha_ctx, BL_SHA_ID, sha_type, ctx->tmp, ctx->pad);
    Sec_Eng_SHA_Start(BL_SHA_ID);
}

int bl_sha_update(bl_sha_ctx_t *ctx, const uint8_t *input, uint32_t len)
{
    return Sec_Eng_SHA256_Update((SEC_Eng_SHA256_Ctx *)&ctx->sha_ctx, BL_SHA_ID, input, len);
}

int bl_sha_finish(bl_sha_ctx_t *ctx, uint8_t *hash)
{
    return Sec_Eng_SHA256_Finish((SEC_Eng_SHA256_Ctx *)&ctx->sha_ctx, BL_SHA_ID, hash);
}
