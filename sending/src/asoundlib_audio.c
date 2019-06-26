/**
 * First attempt to capture audio. Here for reference but it stopped working upon a linux update and it requires additional configuration to load "snd-aloop" kernel module and set it as the sink.
 */

/*
#include "alsa/asoundlib.h"

#include "asoundlib_audio.h"
#include "ouvr_packet.h"

typedef struct asoundlib_audio_context
{
    snd_pcm_t *alsa_ctx;
    int buf_size;
} asoundlib_audio_context;

static int asoundlib_initialize(struct ouvr_ctx *ctx)
{
    int ret;
    if (ctx->aud_priv != NULL)
    {
        free(ctx->aud_priv);
    }
    asoundlib_audio_context *a = calloc(1, sizeof(asoundlib_audio_context));
    ctx->aud_priv = a;

    unsigned int rate = 44100;
    snd_pcm_hw_params_t *hw_params;

    ret = snd_pcm_open(&a->alsa_ctx, "loopout", SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_open() failed\n");
        return -1;
    }

    ret = snd_pcm_hw_params_malloc(&hw_params);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_hw_params_malloc() failed\n");
        return -1;
    }

    ret = snd_pcm_hw_params_any(a->alsa_ctx, hw_params);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_hw_params_any() failed\n");
        return -1;
    }

    ret = snd_pcm_hw_params_set_access(a->alsa_ctx, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_hw_params_set_access() failed\n");
        return -1;
    }

    ret = snd_pcm_hw_params_set_format(a->alsa_ctx, hw_params, SND_PCM_FORMAT_S16_LE);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_hw_params_set_format() failed\n");
        return -1;
    }

    ret = snd_pcm_hw_params_set_rate_near(a->alsa_ctx, hw_params, &rate, 0);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_hw_params_set_rate_near() failed\n");
        return -1;
    }

    ret = snd_pcm_hw_params_set_channels(a->alsa_ctx, hw_params, 2);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_hw_params_set_channels() failed\n");
        return -1;
    }

    // calling this makes cpu go crazy. maybe a param is set wrong.
    ret = snd_pcm_hw_params(a->alsa_ctx, hw_params);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_hw_params() failed\n");
        return -1;
    }

    snd_pcm_hw_params_free(hw_params);

    ret = snd_pcm_prepare(a->alsa_ctx);
    if (ret < 0)
    {
        PRINT_ERR("snd_pcm_prepare() failed\n");
        return -1;
    }

    a->buf_size = 128 * snd_pcm_format_width(SND_PCM_FORMAT_S16_LE) / 8 * 2;

    return 0;
}

static int asoundlib_process_frame(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    int ret;
    asoundlib_audio_context *a = ctx->aud_priv;
    pkt->size = 0;

    for (int i = 0; i < 8; i++)
    {
        ret = snd_pcm_readi(a->alsa_ctx, pkt->data + pkt->size, 128);
        pkt->size += ret * 4;
    }

    return 1;
}
static void asoundlib_deinitialize(struct ouvr_ctx *ctx)
{
    asoundlib_audio_context *a = ctx->aud_priv;
    snd_pcm_close(a->alsa_ctx);
    free(a);
    ctx->aud_priv = NULL;
}

struct ouvr_audio asoundlib_audio = {
    .init = asoundlib_initialize,
    .encode_frame = asoundlib_process_frame,
    .deinit = asoundlib_deinitialize,
};
*/