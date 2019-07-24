#include "ouvr_packet.h"
#include "openmax_render.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <bcm_host.h>

#define OMX_SKIP64BIT 1
#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>

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

// static pthread_mutex_t decode_lock = PTHREAD_MUTEX_INITIALIZER;
// static pthread_cond_t decode_cond = PTHREAD_COND_INITIALIZER;
// int buffer_is_free = 1;
static sem_t decode_sem;

static OMX_HANDLETYPE video_decoder;
static OMX_HANDLETYPE video_render;
static OMX_BUFFERHEADERTYPE *omx_buffer[NUM_BUFS] = {0};
static int start_times[NUM_BUFS] = {0};

static struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = 100000000};

static int omxr_initialize(struct ouvr_ctx *ctx)
{
    // pthread_mutex_init(&decode_lock, NULL);
    // pthread_cond_init(&decode_cond, NULL);
    sem_init(&decode_sem, 0, NUM_BUFS - 1);

    // must be called on raspberry pi before making GPU calls
    bcm_host_init();

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

    // Get handle to video decoding component
    err = OMX_GetHandle(&video_decoder, "OMX.broadcom.video_decode", NULL, &callbacks);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetHandle() returned %x for video_decode\n", err);
        return -1;
    }
    OMX_SendCommand(video_decoder, OMX_CommandPortDisable, 130, NULL);
    OMX_SendCommand(video_decoder, OMX_CommandPortDisable, 131, NULL);
    nanosleep(&sleep_time, NULL);
    OMX_SendCommand(video_decoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
    nanosleep(&sleep_time, NULL);

    //things to set up video_decoder
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
        OMX_INIT_STRUCTURE(formatType);
        formatType.nPortIndex = 130;
        formatType.eCompressionFormat = OMX_VIDEO_CodingAVC;
        formatType.xFramerate = 1966080;
        err = OMX_SetParameter(video_decoder, OMX_IndexParamVideoPortFormat, &formatType);
        if(err != OMX_ErrorNone) {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamVideoPortFormat\n", err);
            return -1;
        }

        OMX_PARAM_PORTDEFINITIONTYPE portParam;
        OMX_INIT_STRUCTURE(portParam);
        portParam.nPortIndex = 130;
        err = OMX_GetParameter(video_decoder, OMX_IndexParamPortDefinition, &portParam);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_GetParameter() returned %x for video_decoder\n", err);
            return -1;
        }
        portParam.nPortIndex = 130;
        portParam.nBufferCountActual = 60;
        portParam.format.video.nFrameWidth  = 1920;
        portParam.format.video.nFrameHeight = 1080;
        err = OMX_SetParameter(video_decoder, OMX_IndexParamPortDefinition, &portParam);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for video_decoder\n", err);
            return -1;
        }

        //tells decoder to not emit corrupted frames
        OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
        OMX_INIT_STRUCTURE(concanParam);
        concanParam.bStartWithValidFrame = OMX_TRUE;
        err = OMX_SetParameter(video_decoder, OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamBrcmVideoDecodeErrorConcealment\n", err);
            return -1;
        }

        OMX_NALSTREAMFORMATTYPE nalStreamFormat;
        OMX_INIT_STRUCTURE(nalStreamFormat);
        nalStreamFormat.nPortIndex = 130;
        nalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;
        err = OMX_SetParameter(video_decoder, (OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSelect, &nalStreamFormat);
        if (err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamNalStreamFormatSelect\n", err);
            return -1;
        }
    }

    OMX_SendCommand(video_decoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    nanosleep(&sleep_time, NULL);

    if(allocate_decoder_input_buffers() != 0) {
        return -1;
    }


    //senddecoderconfig (apparently this isn't necessary?)
    {
        //these are the first 53 bytes of a given packet
        unsigned char extradata[53] = {0x0, 0x0, 0x0, 0x1, 0x67, 0x4d, 0x40, 0x32, 0x95, 0xa0, 0x1e, 0x0, 0x89, 0xf9, 0x70, 0x11, 0x0, 0x0, 0x3, 0x3, 0xe8, 0x0, 0x0, 0xea, 0x60, 0xe0, 0x0, 0x0, 0x3, 0x1, 0x31, 0x2d, 0x0, 0x0, 0x3, 0x0, 0x13, 0x12, 0xd0, 0x1b, 0xbc, 0xb8, 0x3e, 0x95, 0x40, 0x0, 0x0, 0x0, 0x1, 0x68, 0xee, 0x3c, 0x80};

        omx_buffer[0]->nOffset = 0;
        omx_buffer[0]->nFilledLen = (OMX_U32)53;

        memset((unsigned char *)omx_buffer[0]->pBuffer, 0x0, omx_buffer[0]->nAllocLen);
        memcpy((unsigned char *)omx_buffer[0]->pBuffer, extradata, omx_buffer[0]->nFilledLen);
        omx_buffer[0]->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;

        err = OMX_EmptyThisBuffer(video_decoder, omx_buffer[0]);
        if (err != OMX_ErrorNone)
        {
            printf("OMX_EmptyThisBuffer() returned %x for senddecoderconfig\n", err);
            return -1;
        }
        nanosleep(&sleep_time, NULL);
    }

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

        configDisplay.set        = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_MODE | OMX_DISPLAY_SET_SRC_RECT| OMX_DISPLAY_SET_DEST_RECT | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_PIXEL);
        configDisplay.noaspect   = OMX_FALSE;
        configDisplay.mode       = OMX_DISPLAY_MODE_LETTERBOX;

        configDisplay.src_rect.x_offset   = 0;
        configDisplay.src_rect.y_offset   = 0;
        configDisplay.src_rect.width      = 0;
        configDisplay.src_rect.height     = 0;

        configDisplay.dest_rect.x_offset   = 960;
        configDisplay.dest_rect.y_offset   = 100;
        configDisplay.dest_rect.width      = 960;
        configDisplay.dest_rect.height     = 540;

        configDisplay.fullscreen = OMX_FALSE;

        configDisplay.pixel_x = 1;
        configDisplay.pixel_y = 1;

        OMX_SetConfig(video_render, OMX_IndexConfigDisplayRegion, &configDisplay);
        if(err != OMX_ErrorNone) {
            printf("OMX_SetConfig() returned %x for second OMX_IndexConfigDisplayRegion\n", err);
            return -1;
        }
    }

    OMX_SendCommand(video_render, OMX_CommandStateSet, OMX_StateIdle, NULL);
    nanosleep(&sleep_time, NULL);

    // for decoder->render
    // sets src component to StateIdle only if it is StateLoaded, OMX_SetupTunnel(), enables src and dst ports, sets dst component to StateIdle if it's StateLoaded
    err = OMX_SetupTunnel(video_decoder, 131, video_render, 90);
    if(err != OMX_ErrorNone) {
        printf("OMX_SetupTunnel() returned %x for decoder=>render\n", err);
        return -1;
    }
    OMX_SendCommand(video_decoder, OMX_CommandPortEnable, 130, NULL);
    OMX_SendCommand(video_decoder, OMX_CommandPortEnable, 131, NULL);
    OMX_SendCommand(video_render, OMX_CommandPortEnable, 90, NULL);
    nanosleep(&sleep_time, NULL);

    OMX_SendCommand(video_render, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    nanosleep(&sleep_time, NULL);

    return 0;
}

static int allocate_decoder_input_buffers()
{
    OMX_ERRORTYPE err;
    OMX_PARAM_PORTDEFINITIONTYPE portFormat;
    OMX_INIT_STRUCTURE(portFormat);
    portFormat.nPortIndex = 130;

    err = OMX_GetParameter(video_decoder, OMX_IndexParamPortDefinition, &portFormat);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetParameter() returned %x for decoder portdefinition\n", err);
        return -1;
    }

    //enable input port
    OMX_SendCommand(video_decoder, OMX_CommandPortEnable, 130, NULL);
    nanosleep(&sleep_time, NULL);
    
    for(int i = 0; i < NUM_BUFS; i++) {
        //nAllocLen is 81920
        err = OMX_AllocateBuffer(video_decoder, &omx_buffer[i], 130, NULL, portFormat.nBufferSize);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_AllocateBuffer() returned %x\n", err);
            return -1;
        }
        omx_buffer[i]->nInputPortIndex = 130;
        omx_buffer[i]->nFilledLen      = 0;
        omx_buffer[i]->nOffset         = 0;
        omx_buffer[i]->pAppPrivate     = (void *)i; //(void*)i;
    }
    return 0;
}

// OMX_GetState(handle, OMX_STATETYPE *);
// OMX_SendCommand(m_handle, OMX_CommandStateSet, OMX_STATETYPE, 0);
// OMX_SendCommand(m_handle, OMX_CommandPortEnable, portNum, NULL);
// OMX_SetupTunnel(handleOutput, nPortOutput, handleInput, nPortInput);

static int buf_idx = 0;
static int omxr_process_packet(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    OMX_ERRORTYPE err;
    unsigned int remaining_bytes = (unsigned int)pkt->size;
    unsigned char *pos = pkt->data;

#ifdef TIME_DECODING
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
#endif

    // I'll take this to mean that it's an i-frame
    if(pkt->data[4] != 0x6) {
        ctx->flag_send_iframe = 0;
    }

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

        if(remaining_bytes == 0) {
            buf->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
#ifdef TIME_DECODING
            start_times[buf_idx] = start_time.tv_usec;
#endif
        }

        err = OMX_EmptyThisBuffer(video_decoder, buf);
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
#ifdef TIME_DECODING
float avg_dec_time = 0;
#endif

static OMX_ERRORTYPE empty_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer)
{
    //printf("empty buffer done callback\n");

#ifdef TIME_DECODING
    struct timeval end_time;
    int idx = pBuffer->pAppPrivate;
    if(start_times[idx] > 0){
        gettimeofday(&end_time, NULL);
        int elapsed = end_time.tv_usec - start_times[idx];
        if(elapsed < 0) elapsed += 1000000;
            avg_dec_time = 0.998 * avg_dec_time + 0.002 * elapsed;
        printf("\r\033[60Cdec avg: %f, actual: %d", avg_dec_time, elapsed);
        start_times[idx] = 0;
    }
#endif

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

static void omxr_deinitialize()
{
    OMX_Deinit();
    bcm_host_deinit();
}

struct ouvr_decoder openmax_render = {
    .init = omxr_initialize,
    .process_frame = omxr_process_packet,
    .deinit = omxr_deinitialize,
};

int omxr_get_available_buffers(OMX_BUFFERHEADERTYPE **pointers, int num)
{
    for(int i = 0; i < num; i++) {
        OMX_BUFFERHEADERTYPE *buf = omx_buffer[buf_idx];
        buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
        buf->nOffset = 0;
        buf->nFilledLen = 0;
        pointers[i] = buf;
        buf_idx = (buf_idx + 1) % NUM_BUFS;
    }
    return num;
}

int omxr_empty_buffers(OMX_BUFFERHEADERTYPE **pointers, int num)
{
    OMX_ERRORTYPE err;
    for(int i = 0; i < num - 1; i++) {
        err = OMX_EmptyThisBuffer(video_decoder, pointers[i]);
        if (err != OMX_ErrorNone)
        {
            printf("OMX_EmptyThisBuffer() returned %x\n", err);
            exit(1);
        }
    }

    pointers[num - 1]->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
    err = OMX_EmptyThisBuffer(video_decoder, pointers[num - 1]);
    if (err != OMX_ErrorNone)
    {
        printf("OMX_EmptyThisBuffer() returned %x\n", err);
        exit(1);
    }

    return num;    
}
