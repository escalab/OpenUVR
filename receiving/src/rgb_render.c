#include "ouvr_packet.h"
#include "rgb_render.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include <bcm_host.h>

#define OMX_SKIP64BIT 1
#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Image.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>

#define print_state {\
        OMX_STATETYPE state;\
        OMX_GetState(video_render, &state);\
        printf("video_render is in state %d\n", state);\
}


static int allocate_decoder_input_buffers();

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP
#define NUM_BUFS 3

static OMX_ERRORTYPE event_handler_callback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
static OMX_ERRORTYPE empty_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer);
static OMX_ERRORTYPE fill_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer);

// static pthread_mutex_t decode_lock = PTHREAD_MUTEX_INITIALIZER;
// static pthread_cond_t decode_cond = PTHREAD_COND_INITIALIZER;
// int buffer_is_free = 1;
static sem_t decode_sem;

static OMX_HANDLETYPE video_render;
static OMX_BUFFERHEADERTYPE *omx_buffer[NUM_BUFS] = {0};

static struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = 100000000};

static int rgb_initialize(struct ouvr_ctx *ctx)
{
    // pthread_mutex_init(&decode_lock, NULL);
    // pthread_cond_init(&decode_cond, NULL);
    sem_init(&decode_sem, 0, NUM_BUFS - 1);

    // must be called on raspberry pi before making GPU calls
    bcm_host_init();

    // Initialize omxplayer
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


    // Get handle to video decoding component
    err = OMX_GetHandle(&video_render, "OMX.broadcom.video_render", NULL, &callbacks);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetHandle() returned %x for video_render\n", err);
        return -1;
    }

    OMX_SendCommand(video_render, OMX_CommandPortDisable, 90, NULL);
    nanosleep(&sleep_time, NULL);

    OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
    OMX_INIT_STRUCTURE(configDisplay);
    configDisplay.nPortIndex = 90;
    configDisplay.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_ALPHA | OMX_DISPLAY_SET_TRANSFORM | OMX_DISPLAY_SET_LAYER | OMX_DISPLAY_SET_NUM);
    configDisplay.alpha = 255;
    configDisplay.num = 0;
    configDisplay.layer = 0;
    configDisplay.transform = OMX_DISPLAY_ROT0;
    err = OMX_SetConfig(video_render, OMX_IndexConfigDisplayRegion, &configDisplay);
    if(err != OMX_ErrorNone)
    {
        printf("OMX_SetConfig() returned %x for OMX_IndexConfigDisplayRegion\n", err);
        return -1;
    }

    //SetVideoRect
    {
        OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
        OMX_INIT_STRUCTURE(configDisplay);
        configDisplay.nPortIndex = 90;

        configDisplay.set        = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_MODE | OMX_DISPLAY_SET_SRC_RECT | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_PIXEL);
        configDisplay.noaspect   = OMX_FALSE;
        configDisplay.mode       = OMX_DISPLAY_MODE_LETTERBOX;

        configDisplay.src_rect.x_offset   = 0;
        configDisplay.src_rect.y_offset   = 0;
        configDisplay.src_rect.width      = 0;
        configDisplay.src_rect.height     = 0;

        configDisplay.fullscreen = OMX_TRUE;

        configDisplay.pixel_x = 1;
        configDisplay.pixel_y = 1;

        OMX_SetConfig(video_render, OMX_IndexConfigDisplayRegion, &configDisplay);
        if(err != OMX_ErrorNone) {
            printf("OMX_SetConfig() returned %x for second OMX_IndexConfigDisplayRegion\n", err);
            return -1;
        }
    }

    /** Configure compression options */
    OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
    OMX_INIT_STRUCTURE(formatType);
    formatType.nPortIndex = 90;
    formatType.eCompressionFormat = OMX_VIDEO_CodingUnused;
    formatType.eColorFormat = OMX_COLOR_Format24bitRGB888;
    err = OMX_SetParameter(video_render, OMX_IndexParamVideoPortFormat, &formatType);
    if(err != OMX_ErrorNone) {
        printf("OMX_SetParameter() returned %x for OMX_IndexParamVideoPortFormat\n", err);
        return -1;
    }



    /** Configure video options */
    OMX_PARAM_PORTDEFINITIONTYPE configVideo;
    OMX_INIT_STRUCTURE(configVideo);
    configVideo.nPortIndex = 90;
    err = OMX_GetParameter(video_render, OMX_IndexParamPortDefinition, &configVideo);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetParameter() returned %x for render portdefinition configVideo\n", err);
        return -1;
    }
    configVideo.format.video.nFrameWidth = 1920;
    configVideo.format.video.nFrameHeight = 1080;
    configVideo.format.video.nStride = 1920*3;
    configVideo.format.video.nSliceHeight = 0;
    err = OMX_SetParameter(video_render, OMX_IndexParamPortDefinition, &configVideo);
    if(err != OMX_ErrorNone)
    {
        printf("OMX_SetParameter() returned %x for video_render configVideo\n", err);
        return -1;
    }


    OMX_SendCommand(video_render, OMX_CommandStateSet, OMX_StateIdle, NULL);
    nanosleep(&sleep_time, NULL);
    
    OMX_SendCommand(video_render, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    nanosleep(&sleep_time, NULL);
    
    OMX_SendCommand(video_render, OMX_CommandPortEnable, 90, NULL);
    nanosleep(&sleep_time, NULL);
    
    if(allocate_decoder_input_buffers() != 0) {
        return -1;
    }

    return 0;
}

static int allocate_decoder_input_buffers()
{
    OMX_ERRORTYPE err;
    OMX_PARAM_PORTDEFINITIONTYPE portFormat;
    OMX_INIT_STRUCTURE(portFormat);
    portFormat.nPortIndex = 90;

    err = OMX_GetParameter(video_render, OMX_IndexParamPortDefinition, &portFormat);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetParameter() returned %x for render portdefinition\n", err);
        return -1;
    }

    //enable input port
    // OMX_SendCommand(video_render, OMX_CommandPortEnable, 90, NULL);
    // nanosleep(&sleep_time, NULL);
    
    for(int i = 0; i < NUM_BUFS; i++) {
        //nAllocLen is 81920
        err = OMX_AllocateBuffer(video_render, &omx_buffer[i], 90, NULL, portFormat.nBufferSize);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_AllocateBuffer() returned %x\n", err);
            return -1;
        }
        omx_buffer[i]->nInputPortIndex = 90;
        omx_buffer[i]->nFilledLen      = 0;
        omx_buffer[i]->nOffset         = 0;
        omx_buffer[i]->pAppPrivate     = (void *)17 + i; //(void*)i;
    }
    return 0;
}

// OMX_GetState(handle, OMX_STATETYPE *);
// OMX_SendCommand(m_handle, OMX_CommandStateSet, OMX_STATETYPE, 0);
// OMX_SendCommand(m_handle, OMX_CommandPortEnable, portNum, NULL);
// OMX_SetupTunnel(handleOutput, nPortOutput, handleInput, nPortInput);

static int buf_idx = 0;
static int rgb_process_packet(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    OMX_ERRORTYPE err;
    unsigned int remaining_bytes = (unsigned int)pkt->size;
    unsigned char *pos = pkt->data;

    while(remaining_bytes) {
        // pthread_mutex_lock(&decode_lock);
        // while(!buffer_is_free)
        //     pthread_cond_wait(&decode_cond, &decode_lock);
        sem_wait(&decode_sem);
        //buffer_is_free = 0;
        OMX_BUFFERHEADERTYPE *buf = omx_buffer[buf_idx];
        buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
        buf->nOffset = 0;
        buf->nFilledLen = ((OMX_U32)remaining_bytes < buf->nAllocLen ? (OMX_U32)remaining_bytes : buf->nAllocLen);
        memcpy(buf->pBuffer, pos, buf->nFilledLen);

        remaining_bytes -= buf->nFilledLen;
        pos += buf->nFilledLen;

        if(remaining_bytes == 0)
            buf->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        err = OMX_EmptyThisBuffer(video_render, buf);
        if (err != OMX_ErrorNone)
        {
            printf("OMX_EmptyThisBuffer() returned %x\n", err);
            return -1;
        }
        buf_idx = (buf_idx + 1) % NUM_BUFS;
        // pthread_mutex_unlock(&decode_lock);
    }
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
    //printf("Event handler callback %x %d\n", nData1, nData2);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE empty_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer)
{
    //printf("empty buffer done callback\n");

    // pthread_mutex_lock(&decode_lock);
    // buffer_is_free = 1;
    // pthread_cond_broadcast(&decode_cond);
    // pthread_mutex_unlock(&decode_lock);
    sem_post(&decode_sem);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE fill_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer)
{
    printf("fill buffer done callback\n");
    return OMX_ErrorNone;
}

static void rgb_deinitialize()
{
    OMX_Deinit();
    bcm_host_deinit();
}

struct ouvr_decoder rgb_render = {
    .init = rgb_initialize,
    .process_frame = rgb_process_packet,
    .deinit = rgb_deinitialize,
};