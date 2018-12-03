#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
#include "include/libavutil/opt.h"
#include "include/libavutil/imgutils.h"
#include "include/libswscale/swscale.h"

#include "rgb_encode.h"
#include "owvr_packet.h"

typedef struct rgb_encode_context
{
    int unused;
} rgb_encode_context;

static int const srcstride[1] = {-1920 * 3};

static int rgb_initialize(struct owvr_ctx *ctx)
{
    int ret;
    if (ctx->enc_priv != NULL)
    {
        free(ctx->enc_priv);
    }
    rgb_encode_context *e = calloc(1, sizeof(rgb_encode_context));
    ctx->enc_priv = e;

    return 0;
}

#include <time.h>

static int rgb_process_frame(struct owvr_ctx *ctx, struct owvr_packet *pkt)
{
    //wait a little longer for receiving side to keep up
    struct timespec wait_time = {.tv_sec = 0, .tv_nsec = 80000000};
    nanosleep(&wait_time, NULL);

    for (int x = 0; x < 1080; x++)
    {
        for (int y = 0; y < 1920 * 3; y += 8)
        {
            *(long *)(pkt->data + 1920 * 3 * (1079 - x) + y) = *(long *)(ctx->pix_buf + 1920 * 3 * x + y);
        }
    }

    // memcpy(pkt->data, ctx->pix_buf, 1920 * 1080 * 3);
    pkt->size = 1920 * 1080 * 3;

    return 1;
}
static void rgb_deinitialize(struct owvr_ctx *ctx)
{
    void *fdsa = ctx;
    printf("fdsa %p\n", fdsa);
}

struct owvr_encoder rgb_encode = {
    .init = rgb_initialize,
    .process_frame = rgb_process_frame,
    .deinit = rgb_deinitialize,
};
