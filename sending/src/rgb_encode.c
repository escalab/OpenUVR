#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
#include "include/libavutil/opt.h"
#include "include/libavutil/imgutils.h"
#include "include/libswscale/swscale.h"

#include "rgb_encode.h"
#include "ouvr_packet.h"

typedef struct rgb_encode_context
{
    int unused;
} rgb_encode_context;

static int const srcstride[1] = {-1920 * 3};

static int rgb_initialize(struct ouvr_ctx *ctx)
{
    if (ctx->enc_priv != NULL)
    {
        free(ctx->enc_priv);
    }
    rgb_encode_context *e = calloc(1, sizeof(rgb_encode_context));
    ctx->enc_priv = e;

    return 0;
}

static int rgb_process_frame(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    int offset_src = 0;
    int offset_dst = 0;
    for (int y = 0; y < 1080; y++)
    {
        for (int x = 0; x < 1920; x++)
        {
            *(int *)(pkt->data + offset_dst) = *(int *)(ctx->pix_buf + offset_src);
            offset_src += 4;
            offset_dst += 3;
        }
    }

    // memcpy(pkt->data, ctx->pix_buf, 1920 * 1080 * 3);
    pkt->size = 1920 * 1080 * 3;

    return 1;
}
static void rgb_deinitialize(struct ouvr_ctx *ctx)
{
    free(ctx->enc_priv);
}

struct ouvr_encoder rgb_encode = {
    .init = rgb_initialize,
    .process_frame = rgb_process_frame,
    .deinit = rgb_deinitialize,
};
