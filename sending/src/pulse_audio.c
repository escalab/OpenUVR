#include <stdio.h>

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include "asoundlib_audio.h"
#include "ouvr_packet.h"

typedef struct pulse_audio_context
{
    pa_simple *s;
    pa_sample_spec ss;
} pulse_audio_context;

static pulse_audio_context pac;

static int pulse_initialize(struct ouvr_ctx *ctx)
{
    pac.ss.format = PA_SAMPLE_S16LE;
    pac.ss.channels = 2;
    pac.ss.rate = 44100;

    
    return 0;
}

static int pulse_process_frame(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    pac.s = pa_simple_new(NULL,    // Use the default server.
                          "thing", // Our application's name.
                          PA_STREAM_RECORD,
                          NULL,        // Use the default device.
                          "has stuff", // Description of our stream.
                          &pac.ss,     // Our sample format.
                          NULL,        // Use default channel map
                          NULL,        // Use default buffering attributes.
                          NULL         // Ignore error code.
    );
    
    int err = 17;
    printf("asdf\n");
    int ret = pa_simple_read(pac.s, pkt->data, 1024, &err);
    if (ret < 0)
    {
        PRINT_ERR("err: %d\n", err);
        exit(1);
    }
    pkt->size = 4096;
    printf("fdsa\n");

    pa_simple_free(pac.s);

    return 1;
}

static void pulse_deinitialize(struct ouvr_ctx *ctx)
{
    printf("TODO: implement pulse_deinitialize\n");
}

struct ouvr_audio pulse_audio = {
    .init = pulse_initialize,
    .encode_frame = pulse_process_frame,
    .deinit = pulse_deinitialize,
};
