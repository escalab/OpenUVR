#include "owvr_packet.h"
#include "openmax_audio.h"
#include <stdio.h>

#include <bcm_host.h>

#define OMX_SKIP64BIT 1
#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Audio.h>
#include <IL/OMX_Broadcom.h>

#include <libavcodec/avcodec.h>


static int allocate_decoder_input_buffers();

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP
#define NUM_BUFS 16

static OMX_ERRORTYPE event_handler_callback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
static OMX_ERRORTYPE empty_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer);
static OMX_ERRORTYPE fill_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer);

// TODO: translate these globals into a private instance-specific data structure

static OMX_HANDLETYPE audio_render;
static OMX_BUFFERHEADERTYPE *omx_buffer[NUM_BUFS] = {0};

static struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = 100000000};
#define print_state(a) {\
        OMX_STATETYPE state;\
        OMX_GetState(a, &state);\
        printf("it is in state %d\n", state);\
}
static int ffmpeg_audio_init(struct owvr_ctx *ctx){
    printf("init\n");

    // must be called on raspberry pi before making GPU calls
    bcm_host_init();

    avcodec_register_all();
    AVCodec *dec = avcodec_find_decoder_by_name("aac");
    printf("%p\n", dec);

    AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
    dec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;

    int ret = avcodec_open2(dec_ctx, dec, NULL);
    if (ret < 0)
    {
        printf("avcodec_open2 failed\n");
        return -1;
    }


    // audio render stuff
    if (0)
    {
        // Initialize openmax
        OMX_ERRORTYPE err = OMX_Init();
        if(err != OMX_ErrorNone) {
            printf("OMX_Init() returned %x\n", err);
            return -1;
        }

        //define callbacks
        OMX_CALLBACKTYPE callbacks;
        callbacks.EventHandler = event_handler_callback;
        callbacks.EmptyBufferDone = empty_buffer_done_callback;
        callbacks.FillBufferDone = fill_buffer_done_callback;

        // Get handle to audio decoding component
        err = OMX_GetHandle(&audio_render, "OMX.broadcom.audio_render", NULL, &callbacks);
        if(err != OMX_ErrorNone) {
            printf("OMX_GetHandle() returned %x for audio_render\n", err);
            return -1;
        }

        OMX_SendCommand(audio_render, OMX_CommandPortDisable, 100, NULL);
        OMX_SendCommand(audio_render, OMX_CommandPortDisable, 101, NULL);
        nanosleep(&sleep_time, NULL);

        //put stuff here for audio_render i guess

        OMX_SendCommand(audio_render, OMX_CommandStateSet, OMX_StateIdle, NULL);
        nanosleep(&sleep_time, NULL);

        OMX_SendCommand(audio_render, OMX_CommandPortEnable, 100, NULL);
        nanosleep(&sleep_time, NULL);

        OMX_SendCommand(audio_render, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        nanosleep(&sleep_time, NULL);

        if(allocate_decoder_input_buffers() != 0) {
            return -1;
        }
    }

    return 0;
}

static int allocate_decoder_input_buffers()
{
    OMX_ERRORTYPE err;
    OMX_PARAM_PORTDEFINITIONTYPE portFormat;
    OMX_INIT_STRUCTURE(portFormat);
    portFormat.nPortIndex = 100;

    err = OMX_GetParameter(audio_render, OMX_IndexParamPortDefinition, &portFormat);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetParameter() returned %x for render portdefinition\n", err);
        return -1;
    }

    //enable input port
    OMX_SendCommand(audio_render, OMX_CommandPortEnable, 100, NULL);
    nanosleep(&sleep_time, NULL);
    
    for(int i = 0; i < NUM_BUFS; i++) {
        //nAllocLen is 81920
        err = OMX_AllocateBuffer(audio_render, &omx_buffer[i], 100, NULL, portFormat.nBufferSize);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_AllocateBuffer() returned %x\n", err);
            return -1;
        }
        omx_buffer[i]->nInputPortIndex = 100;
        omx_buffer[i]->nFilledLen      = 0;
        omx_buffer[i]->nOffset         = 0;
        omx_buffer[i]->pAppPrivate     = (void *)i; //(void*)i;
    }
    return 0;
}

static int ffmpeg_audio_process_frame(struct owvr_ctx *ctx, struct owvr_packet *pkt){
    printf("process\n");
    return 0;
}

static OMX_ERRORTYPE event_handler_callback(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 nData1,
  OMX_U32 nData2,
  OMX_PTR pEventData)
{
    printf("Event handler callback %x %d\n", nData1, nData2);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE empty_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer)
{
    printf("empty buffer done callback\n");
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE fill_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer)
{
    printf("fill buffer done callback\n");
    return OMX_ErrorNone;
}

static void ffmpeg_audio_deinit(){
    printf("deinit\n");
}

struct owvr_audio ffmpeg_audio = {
    .init = ffmpeg_audio_init,
    .process_frame = ffmpeg_audio_process_frame,
    .deinit = ffmpeg_audio_deinit,
};