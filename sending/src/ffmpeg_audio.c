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
/**
 * Here for posterity but only works with snd-aloop kernel module. A better solution is to use pulse_audio.c
 */
/*
#include "include/libavcodec/avcodec.h"
#include "include/libavdevice/avdevice.h"
#include "include/libavutil/avutil.h"
#include "include/libavutil/opt.h"
#include "include/libavutil/imgutils.h"
#include "include/libswscale/swscale.h"
#include "alsa/asoundlib.h"

#include "ffmpeg_encode.h"
#include "ouvr_packet.h"

typedef struct ffmpeg_audio_context
{
    AVCodecContext *enc_ctx;
    AVFormatContext *alsa_fmt_ctx;
    AVFrame *frame;
    int idx;
} ffmpeg_audio_context;

static int ffmpeg_initialize(struct ouvr_ctx *ctx)
{
    int ret;
    if (ctx->aud_priv != NULL)
    {
        free(ctx->aud_priv);
    }
    ffmpeg_audio_context *a = calloc(1, sizeof(ffmpeg_audio_context));
    ctx->aud_priv = a;
    AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (enc == NULL)
    {
        PRINT_ERR("couldn't find encoder\n");
        return -1;
    }
    a->enc_ctx = avcodec_alloc_context3(enc);
    a->enc_ctx->time_base = (AVRational){1, 60};
    a->enc_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    a->enc_ctx->sample_rate = 48000;
    a->enc_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    a->enc_ctx->bit_rate = 128000;

    ret = avcodec_open2(a->enc_ctx, enc, NULL);
    if (ret < 0)
    {
        PRINT_ERR("avcodec_open2 failed\n");
        return -1;
    }

    avdevice_register_all();
    a->alsa_fmt_ctx = avformat_alloc_context();
    a->alsa_fmt_ctx->iformat = av_find_input_format("alsa");

    AVDeviceInfoList *devs;
    avdevice_list_devices(a->alsa_fmt_ctx, &devs);
    printf("devinfolist: %d\n", devs->nb_devices);
    for (int i = 0; i < devs->nb_devices; i++)
        printf("%s | %s\n", devs->devices[i]->device_name, devs->devices[i]->device_description);

    ret = avformat_open_input(&a->alsa_fmt_ctx, "loopout", a->alsa_fmt_ctx->iformat, NULL);
    if (ret < 0)
    {
        PRINT_ERR("avformat_open_input failed\n");
        return -1;
    }

    ret = avformat_find_stream_info(a->alsa_fmt_ctx, NULL);
    if (ret < 0)
    {
        PRINT_ERR("avformat_find_stream_info failed\n");
        return -1;
    }

    // int stream_idx = av_find_best_stream(a->alsa_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    // if (stream_idx < 0)
    // {
    //     PRINT_ERR("av_find_best_stream failed\n");
    //     return -1;
    // }

    // apparently ALSA format (format_ctx->streams[ stream_idx ]->codec->codec_id) is 65536=AV_CODEC_ID_PCM_S16LE
    // AVCodec *alsa_dec = avcodec_find_decoder( a->alsa_fmt_ctx->streams[ 0 ]->codecpar->codec_id );
    // if (alsa_dec == NULL)
    // {
    //     PRINT_ERR("avcodec_find_decoder failed\n");
    //     return -1;
    // }

    a->frame = av_frame_alloc();
    a->frame->format = a->enc_ctx->sample_fmt;
    a->frame->channel_layout = a->enc_ctx->channel_layout;
    a->frame->sample_rate = a->enc_ctx->sample_rate;
    a->frame->nb_samples = a->enc_ctx->frame_size;

    return 0;
}

static int ffmpeg_process_frame(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    int ret;
    ffmpeg_audio_context *a = ctx->aud_priv;

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    ret = avcodec_receive_packet(a->enc_ctx, &packet);
    if (ret >= 0)
    {
        memcpy(pkt->data, packet.data, packet.size);
        pkt->size = packet.size;
        printf("size %d\n", pkt->size);
        return 1;
    }
    else if (ret != -11)
    {
        PRINT_ERR("avcodec_receive_packet error: %d\n", ret);
        return -1;
    }

    AVPacket alsa_pkt;
    AVPacket alsa_pkts[16];
    av_init_packet(&alsa_pkt);
    pkt->size = 0;
    const int N = 6;
    for (int i = 0; i < N; i++)
    {
        av_init_packet(&alsa_pkts[i]);
        alsa_pkts[i].data = NULL;
        alsa_pkts[i].size = 0;
        ret = av_read_frame(a->alsa_fmt_ctx, &alsa_pkts[i]);
        if (ret < 0)
        {
            PRINT_ERR("av_read_frame failed\n");
            return -1;
        }
    }

    for (int i = 0; i < N; i++)
    {
        memcpy(pkt->data + pkt->size, alsa_pkts[i].data, alsa_pkts[i].size);
        pkt->size += alsa_pkts[i].size;
    }
    return 1;

    //TODO: delete this stuff or use encoding again

    float flt_buf[1024];
    for (int i = 0; i < 256; i++)
    {
        flt_buf[i] = ((short *)alsa_pkt.data)[i];
    }

    a->frame->nb_samples = 256;
    ret = avcodec_fill_audio_frame(a->frame, 2, a->enc_ctx->sample_fmt, (const uint8_t *)flt_buf, 2048, 32);

    ret = avcodec_send_frame(a->enc_ctx, a->frame);
    if (ret != 0 && ret != -11)
    {
        PRINT_ERR("avcodec_send_frame() failed: %d\n", ret);
        return -1;
    }

    return 0;
}
static void ffmpeg_deinitialize(struct ouvr_ctx *ctx)
{
    void *fdsa = ctx;
    printf("ffmpeg_deinitialize was called but is unimplemented %p\n", fdsa);
}

struct ouvr_audio ffmpeg_audio = {
    .init = ffmpeg_initialize,
    .encode_frame = ffmpeg_process_frame,
    .deinit = ffmpeg_deinitialize,
};
*/