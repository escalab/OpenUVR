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

#include "include/libavcodec/avcodec.h"
#include "include/libavutil/pixdesc.h"
#include "include/libavutil/imgutils.h"
#include "include/libswscale/swscale.h"

#include "ssim_dummy_net.h"
#include "ssim_plugin.h"

typedef struct ssim_dummy_net_context
{
    AVCodecContext *dec_ctx;
    uint8_t *grayscale_buf;
} ssim_dummy_net_context;

enum AVPixelFormat myGetFormat(struct AVCodecContext *s, const enum AVPixelFormat *fmt)
{
    int i = 0;
    while (fmt[i] != -1)
        PRINT_ERR("fdsafdsa %s\n", av_get_pix_fmt_name(fmt[i++]));
    return fmt[0];
}

static int ssim_dummy_net_initialize(struct ouvr_ctx *ctx)
{
    ssim_dummy_net_context *d = calloc(1, sizeof(ssim_dummy_net_context));
    ctx->net_priv = d;

    AVCodec *dec = avcodec_find_decoder_by_name("h264_crystalhd");
    if (dec == NULL)
    {
        PRINT_ERR("couldn't find decoder\n");
        return -1;
    }

    d->dec_ctx = avcodec_alloc_context3(dec);
    d->dec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    // d->dec_ctx->get_format = myGetFormat;

    int ret = avcodec_open2(d->dec_ctx, dec, NULL);
    if (ret < 0)
    {
        PRINT_ERR("avcodec_open2 failed\n");
        return -1;
    }

    d->grayscale_buf = malloc(1920 * 1080);

    return 0;
}

static int analyze_similarity(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    int ret;
    ssim_dummy_net_context *d = ctx->net_priv;

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = pkt->data;
    packet.size = pkt->size;

    ret = avcodec_send_packet(d->dec_ctx, &packet);
    if (ret != 0 && ret != -11)
    {
        PRINT_ERR("avcodec_send_packet() failed: %d\n", ret);
        return -1;
    }

    AVFrame *frame = av_frame_alloc();

    ret = avcodec_receive_frame(d->dec_ctx, frame);
    if (ret >= 0)
    {
        PRINT_ERR("%d %d %d\n", frame->linesize[0], frame->linesize[1], frame->linesize[2]);
        py_ssim_set_cmp_image_data_or_compute_ssim(frame->data[0]);
        av_frame_free(&frame);
        return 1;
    }
    else if (ret != -11)
    {
        PRINT_ERR("avcodec_receive_frame() failed: %d\n", ret);
        return -1;
    }

    av_frame_free(&frame);

    return 0;
}

struct ouvr_network ssim_dummy_net_handler = {
    .init = ssim_dummy_net_initialize,
    .send_packet = analyze_similarity,
};
