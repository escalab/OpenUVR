/*
    The MIT License (MIT)

    Copyright (c) 2020 OpenUVR

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Authors:
    Alec Rohloff
    Zackary Allen
    Kung-Min Lin
    Chengyi Nie
    Hung-Wei Tseng
*/
#include "openuvr.h"
#include "ouvr_packet.h"
#include "tcp.h"
#include "udp.h"
#include "raw.h"
#include "udp_compat.h"
#include "openmax_render.h"
#include "rgb_render.h"
#include "openmax_audio.h"
#include "ffmpeg_audio.h"
#include "feedback_net.h"
#include "input_send.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define NUM_PACKETS 10

// tests round-trip time of large blank packet
void test_rtt(){
    struct ouvr_ctx *ctx = calloc(1, sizeof(struct ouvr_ctx));
    ctx->net=&raw_handler;
    ctx->flag_send_iframe = 1;
    struct ouvr_packet *pkt=ouvr_packet_alloc();
    feedback_initialize(ctx);
    ctx->net->init(ctx);
    while(1){
    ctx->net->recv_packet(ctx, pkt);
    ctx->flag_send_iframe = pkt->data[0];
    feedback_send(ctx);
    }
    exit(1);
}

struct openuvr_context *openuvr_alloc_context(enum OPENUVR_DECODER_TYPE dec_type, enum OPENUVR_NETWORK_TYPE net_type)
{
    struct openuvr_context *ret = calloc(1, sizeof(struct openuvr_context));
    struct ouvr_ctx *ctx = calloc(1, sizeof(struct ouvr_ctx));

    //test_rtt();

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
    
    switch (dec_type)
    {
    case OPENUVR_DECODER_RGB:
        ctx->dec = &rgb_render;
        break;
    case OPENUVR_DECODER_H264:
    default:
        ctx->dec = &openmax_render;
    }
    if (ctx->dec->init(ctx) != 0)
    {
        goto err;
    }

    ctx->aud = &openmax_audio;
    if (ctx->aud->init(ctx) != 0)
    {
        goto err;
    }

    //TODO: make packets only hold one again, multiple was necessary for multithreaded receiving/decoding, but such operation didn't work very well
    ctx->packets = calloc(sizeof(struct ouvr_packet *), NUM_PACKETS);
    for(int i = 0; i < NUM_PACKETS; i++)
    {
        ctx->packets[i] = ouvr_packet_alloc();
    }

//    send_input_loop_start();
    ctx->flag_send_iframe = 1;
    
    if(feedback_initialize(ctx) != 0)
    {
        goto err;
    }

    ret->priv = ctx;
    return ret;

err:
    free(ctx);
    free(ret);
    return NULL;
}

#include <sys/time.h>

int openuvr_receive_frame_raw_h264(struct openuvr_context *context)
{
    struct ouvr_ctx *ctx = context->priv;
    raw_receive_and_decode(ctx);
    feedback_send(ctx);
    return 0;
}

int openuvr_receive_frame(struct openuvr_context *context)
{
    struct ouvr_ctx *ctx = context->priv;
    struct ouvr_packet *pkt = ctx->packets[0];
    if (ctx->net->recv_packet(ctx, pkt) != 0)
    {
        return -1;
    }
    if (pkt->size == 4096)
    {
        if(ctx->aud->process_frame(ctx, pkt) != 0)
        {
            return -1;
        }
    } else {
        if (pkt->size && ctx->dec->process_frame(ctx, pkt) != 0)
        {
            return -1;
        }
        feedback_send(ctx);
    }
#if defined (TIME_NETWORK) || defined (TIME_DECODING)
    fflush(stdout);
#endif
    return 0;
}

int idx;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void *receive_loop(void *arg){
    struct ouvr_ctx *ctx = arg;
    while(1){
        int new_idx = (idx + 1) % NUM_PACKETS;
        ctx->net->recv_packet(ctx,ctx->packets[new_idx]);
        pthread_mutex_lock(&mut);
        idx = new_idx;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);
    }
    return NULL;
}

int openuvr_receive_loop(struct openuvr_context *context)
{
    struct ouvr_ctx *ctx = context->priv;
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_loop, ctx);

    while(1){
        pthread_mutex_lock(&mut);
        pthread_cond_wait(&cond, &mut);
        int i = idx;
        pthread_mutex_unlock(&mut);
        if (ctx->dec->process_frame(ctx, ctx->packets[i]) != 0)
        {
            return 1;
        }
    }
    return 0;
}
