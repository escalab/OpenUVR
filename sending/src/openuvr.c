#include "openuvr.h"
#include "ouvr_packet.h"
#include "tcp.h"
#include "udp.h"
#include "raw.h"
#include "udp_compat.h"
#include "ffmpeg_encode.h"
#include "ffmpeg_cuda_encode.h"
#include "rgb_encode.h"
#include "ffmpeg_audio.h"
#include "asoundlib_audio.h"
#include "feedback_net.h"
#include "input_recv.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <sys/time.h>
#include <time.h>

struct openuvr_context *openuvr_alloc_context(enum OPENUVR_ENCODER_TYPE enc_type, enum OPENUVR_NETWORK_TYPE net_type, unsigned char *pix_buf)
{
    struct openuvr_context *ret = calloc(1, sizeof(struct openuvr_context));
    struct ouvr_ctx *ctx = calloc(1, sizeof(struct ouvr_ctx));

    switch (net_type)
    {
    case OPENUVR_NETWORK_TCP:
        ctx->net = &tcp_handler;
        break;
    case OPENUVR_NETWORK_RAW:
        ctx->net = &raw_handler;
        break;
    case OPENUVR_NETWORK_UDP_COMPAT:
        ctx->net = &udp_compat_handler;
        break;
    case OPENUVR_NETWORK_UDP:
    default:
        ctx->net = &udp_handler;
    }
    if (ctx->net->init(ctx) != 0)
    {
        goto err;
    }

    ctx->pix_buf = pix_buf;

    switch (enc_type)
    {
    case OPENUVR_ENCODER_RGB:
        ctx->enc = &rgb_encode;
        break;
    case OPENUVR_ENCODER_H264_CUDA:
        ctx->enc = &ffmpeg_cuda_encode;
        break;
    case OPENUVR_ENCODER_H264:
    default:
        ctx->enc = &ffmpeg_encode;
    }
    if (ctx->enc->init(ctx) != 0)
    {
        goto err;
    }

    // ctx->aud = &asoundlib_audio;
    // if (ctx->aud->init(ctx) != 0)
    // {
    //     goto err;
    // }

    ctx->packet = ouvr_packet_alloc();

    receive_input_loop_start();
    ctx->flag_send_iframe = 0;
    feedback_initialize(ctx);

    ret->priv = ctx;
    return ret;

err:
    free(ctx);
    free(ret);
    return NULL;
}
#ifdef TIME_ENCODING
    float avg_enc_time = 0;
#endif
int openuvr_send_frame(struct openuvr_context *context)
{
    int ret;
    struct ouvr_ctx *ctx = context->priv;
    struct timeval start, end;

    // ret = ctx->aud->encode_frame(ctx, ctx->packet);
    // if (ret < 0)
    // {
    //     return 1;
    // }
    // else if (ret > 0)
    // {
    //     ret = ctx->net->send_packet(ctx, ctx->packet);
    //     if (ret < 0)
    //     {
    //         return 1;
    //     }
    // }

    feedback_receive(ctx);

#ifdef TIME_ENCODING
    gettimeofday(&start, NULL);
#endif
    do
    {
        ret = ctx->enc->process_frame(ctx, ctx->packet);
    } while (ret == 0);
    if (ret < 0)
    {
        return 1;
    }
#ifdef TIME_ENCODING
    gettimeofday(&end, NULL);
    int elapsed = end.tv_usec - start.tv_usec + (end.tv_sec > start.tv_sec ? 1000000 : 0);
    avg_enc_time = 0.998 * avg_enc_time + 0.002 * elapsed;
    printf("\r\033[60Cenc avg: %f, actual: %d    ", avg_enc_time, elapsed);
    fflush(stdout);
#endif

#ifdef TIME_NETWORK
    gettimeofday(&start, NULL);
#endif
    ret = ctx->net->send_packet(ctx, ctx->packet);
    if (ret < 0)
    {
        return 1;
    }
#ifdef TIME_NETWORK
    gettimeofday(&end, NULL);
    printf("%d\n", end.tv_usec - start.tv_usec + (end.tv_sec > start.tv_sec ? 1000000 : 0));
#endif

    return 0;
}

int openuvr_cuda_copy(struct openuvr_context *context, void *ptr)
{
    struct ouvr_ctx *ctx = context->priv;
    if (ctx->enc->cuda_copy)
        ctx->enc->cuda_copy(ctx);
}

long prev_quot = 0;
int should_exit = 0;
pthread_t send_thread;

void *send_loop_continuous(void *arg)
{
    struct openuvr_context *context = arg;

    struct timespec cur_time;
    struct timespec wait_time = {.tv_sec = 0, .tv_nsec = 500000};

    long div = 1e9 / 60;
    long quot = 0;

    while (!should_exit)
    {
        do
        {
            clock_gettime(CLOCK_MONOTONIC, &cur_time);
            quot = cur_time.tv_nsec / div;
            nanosleep(&wait_time, NULL);
        } while (quot == prev_quot);
        prev_quot = quot;

        openuvr_send_frame(context);

        nanosleep(&wait_time, NULL);
    }

    return NULL;
}

int openuvr_init_thread_continuous(struct openuvr_context *context)
{
    pthread_create(&send_thread, NULL, send_loop_continuous, context);
    return 0;
}

void openuvr_close(struct openuvr_context *context)
{
    if (context == NULL || context->priv == NULL)
    {
        return;
    }
    struct ouvr_ctx *ctx = context->priv;
    should_exit = 1;
    pthread_join(send_thread, NULL);
    ctx->enc->deinit(ctx);
    ctx->aud->deinit(ctx);
}