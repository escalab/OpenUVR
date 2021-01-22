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
#include "ouvr_packet.h"
#include "openmax_audio.h"
#include <stdio.h>

#include <bcm_host.h>

#define OMX_SKIP64BIT 1
#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Audio.h>
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

static OMX_HANDLETYPE audio_decoder;
static OMX_HANDLETYPE audio_render;
static OMX_BUFFERHEADERTYPE *omx_buffer[NUM_BUFS] = {0};

static struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = 100000000};
#define print_state(a) {\
        OMX_STATETYPE state;\
        OMX_GetState(a, &state);\
        printf("it is in state %d\n", state);\
}

static int omxr_audio_init(struct ouvr_ctx *ctx){
    printf("init\n");

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

    // Get handle to audio rendering component
    err = OMX_GetHandle(&audio_render, "OMX.broadcom.audio_render", NULL, &callbacks);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetHandle() returned %x for audio_render\n", err);
        return -1;
    }

    OMX_SendCommand(audio_render, OMX_CommandPortDisable, 100, NULL);
    OMX_SendCommand(audio_render, OMX_CommandPortDisable, 101, NULL);
    nanosleep(&sleep_time, NULL);

    

    //things to set up audio_render
    {
        OMX_AUDIO_PARAM_PORTFORMATTYPE formatType;
        OMX_INIT_STRUCTURE(formatType);
        formatType.nPortIndex = 100;
        formatType.eEncoding = OMX_AUDIO_CodingPCM;
        err = OMX_SetParameter(audio_render, OMX_IndexParamAudioPortFormat, &formatType);
        if(err != OMX_ErrorNone) {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamAudioPortFormat\n", err);
            return -1;
        }

        OMX_PARAM_PORTDEFINITIONTYPE portParam;
        OMX_INIT_STRUCTURE(portParam);
        portParam.nPortIndex = 100;
        err = OMX_GetParameter(audio_render, OMX_IndexParamPortDefinition, &portParam);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_GetParameter() returned %x for audio_render\n", err);
            return -1;
        }
        printf("buffersize: %d, buffercountactual: %d\n", portParam.nBufferSize, portParam.nBufferCountActual);
        portParam.nPortIndex = 100;
        //portParam.nBufferCountActual = 60;
        portParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
        err = OMX_SetParameter(audio_render, OMX_IndexParamPortDefinition, &portParam);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for audio_render\n", err);
            return -1;
        }

        OMX_AUDIO_PARAM_PCMMODETYPE pcm_conf;
        OMX_INIT_STRUCTURE(pcm_conf);
        pcm_conf.nPortIndex = 100;
        pcm_conf.nChannels = 2;
        pcm_conf.eNumData = OMX_NumericalDataSigned;
        pcm_conf.eEndian = OMX_EndianLittle; //TODO: see if this is right
        pcm_conf.bInterleaved = OMX_FALSE; //TODO: see if this is right
        pcm_conf.nBitPerSample = 16; //TODO: see if this is right
        pcm_conf.nSamplingRate = 0;
        pcm_conf.ePCMMode = OMX_AUDIO_PCMModeLinear; //TODO: see if this is right
        pcm_conf.eChannelMapping[0] = OMX_AUDIO_ChannelLF; //TODO: see if this is right
        pcm_conf.eChannelMapping[1] = OMX_AUDIO_ChannelRF; //TODO: see if this is right
        OMX_SetParameter(audio_render, OMX_IndexParamAudioPcm, &pcm_conf);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamAudioPcm\n", err);
            return -1;
        }

        OMX_CONFIG_BRCMAUDIODESTINATIONTYPE dest_conf;
        OMX_INIT_STRUCTURE(dest_conf);
        memcpy(dest_conf.sName, "hdmi", 5);
        printf("%s\n", dest_conf.sName);
        OMX_SetParameter(audio_render, OMX_IndexConfigBrcmAudioDestination, &dest_conf);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamAudioPcm\n", err);
            return -1;
        }
    }

    

    OMX_SendCommand(audio_render, OMX_CommandStateSet, OMX_StateIdle, NULL);
    nanosleep(&sleep_time, NULL);



    {
        OMX_ERRORTYPE err;
        OMX_PARAM_PORTDEFINITIONTYPE portFormat;
        OMX_INIT_STRUCTURE(portFormat);
        portFormat.nPortIndex = 100;

        err = OMX_GetParameter(audio_render, OMX_IndexParamPortDefinition, &portFormat);
        if(err != OMX_ErrorNone) {
            printf("OMX_GetParameter() returned %x for decoder portdefinition\n", err);
            return -1;
        }

        //enable input port
        OMX_SendCommand(audio_render, OMX_CommandPortEnable, 100, NULL);
        nanosleep(&sleep_time, NULL);
        
        for(int i = 0; i < NUM_BUFS; i++) {
            //nAllocLen is 4096
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
    }

    OMX_SendCommand(audio_render, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    nanosleep(&sleep_time, NULL);

    return 0;
}

static int omxr_audio_aac_init(struct ouvr_ctx *ctx){
    printf("init\n");

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

    // Get handle to audio decoding component
    err = OMX_GetHandle(&audio_decoder, "OMX.broadcom.audio_decode", NULL, &callbacks);
    if(err != OMX_ErrorNone) {
        printf("OMX_GetHandle() returned %x for audio_decode\n", err);
        return -1;
    }
    OMX_SendCommand(audio_decoder, OMX_CommandPortDisable, 120, NULL);
    OMX_SendCommand(audio_decoder, OMX_CommandPortDisable, 121, NULL);
    nanosleep(&sleep_time, NULL);
    OMX_SendCommand(audio_decoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
    nanosleep(&sleep_time, NULL);

    //things to set up audio_decoder
    {
        OMX_AUDIO_PARAM_PORTFORMATTYPE formatType;
        OMX_INIT_STRUCTURE(formatType);
        formatType.nPortIndex = 120;
        formatType.eEncoding = OMX_AUDIO_CodingAAC;
        err = OMX_SetParameter(audio_decoder, OMX_IndexParamAudioPortFormat, &formatType);
        if(err != OMX_ErrorNone) {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamAudioPortFormat\n", err);
            return -1;
        }

        OMX_PARAM_PORTDEFINITIONTYPE portParam;
        OMX_INIT_STRUCTURE(portParam);
        portParam.nPortIndex = 120;
        err = OMX_GetParameter(audio_decoder, OMX_IndexParamPortDefinition, &portParam);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_GetParameter() returned %x for audio_decoder\n", err);
            return -1;
        }
        printf("fdsa %d %d\n", portParam.nBufferSize, portParam.nBufferCountActual);
        portParam.nPortIndex = 120;
        //portParam.nBufferCountActual = 60;
        portParam.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
        err = OMX_SetParameter(audio_decoder, OMX_IndexParamPortDefinition, &portParam);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for audio_decoder\n", err);
            return -1;
        }


        printf("start\n");
        OMX_AUDIO_PARAM_AACPROFILETYPE aac_conf;
        OMX_INIT_STRUCTURE(aac_conf);
        aac_conf.nPortIndex = 120;
        aac_conf.nChannels = 2;
        aac_conf.nSampleRate = 0;
        aac_conf.nBitRate = 0;
        aac_conf.nAudioBandWidth = 0;
        aac_conf.nFrameLength = 0;
        aac_conf.eAACProfile = OMX_AUDIO_AACObjectLD; //low delay object (error resilient), TODO: see if this is right
        aac_conf.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW; //TODO: see if this is right
        aac_conf.eChannelMode = OMX_AUDIO_ChannelModeStereo; //TODO: see if this is right
        OMX_SetParameter(audio_decoder, OMX_IndexParamAudioAac, &aac_conf);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamAudioAac\n", err);
            return -1;
        }
        printf("end\n");


        OMX_PARAM_DATAUNITTYPE dutype;
        OMX_INIT_STRUCTURE(dutype);
        dutype.nPortIndex = 120;
        dutype.eUnitType = OMX_DataUnitCodedPicture;
        dutype.eEncapsulationType = OMX_DataEncapsulationElementaryStream;
        OMX_SetParameter(audio_decoder, OMX_IndexParamBrcmDataUnit, &dutype);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_SetParameter() returned %x for OMX_IndexParamBrcmDataUnit\n", err);
            return -1;
        }
    }

    OMX_SendCommand(audio_decoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    nanosleep(&sleep_time, NULL);

    {
        OMX_ERRORTYPE err;
        OMX_PARAM_PORTDEFINITIONTYPE portFormat;
        OMX_INIT_STRUCTURE(portFormat);
        portFormat.nPortIndex = 120;

        err = OMX_GetParameter(audio_decoder, OMX_IndexParamPortDefinition, &portFormat);
        if(err != OMX_ErrorNone) {
            printf("OMX_GetParameter() returned %x for decoder portdefinition\n", err);
            return -1;
        }

        //enable input port
        OMX_SendCommand(audio_decoder, OMX_CommandPortEnable, 120, NULL);
        nanosleep(&sleep_time, NULL);
        
        for(int i = 0; i < NUM_BUFS; i++) {
            //nAllocLen is 81920
            err = OMX_AllocateBuffer(audio_decoder, &omx_buffer[i], 120, NULL, portFormat.nBufferSize);
            if(err != OMX_ErrorNone)
            {
                printf("OMX_AllocateBuffer() returned %x\n", err);
                return -1;
            }
            omx_buffer[i]->nInputPortIndex = 120;
            omx_buffer[i]->nFilledLen      = 0;
            omx_buffer[i]->nOffset         = 0;
            omx_buffer[i]->pAppPrivate     = (void *)i; //(void*)i;
        }
    }

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

    // for decoder->render
    // sets src component to StateIdle only if it is StateLoaded, OMX_SetupTunnel(), enables src and dst ports, sets dst component to StateIdle if it's StateLoaded
    err = OMX_SetupTunnel(audio_decoder, 121, audio_render, 100);
    if(err != OMX_ErrorNone) {
        printf("OMX_SetupTunnel() returned %x for decoder=>render\n", err);
        return -1;
    }
    OMX_SendCommand(audio_decoder, OMX_CommandPortEnable, 120, NULL); //this is returning 0x80001019 because of incorrect configuration of port 120
    OMX_SendCommand(audio_decoder, OMX_CommandPortEnable, 121, NULL);
    OMX_SendCommand(audio_render, OMX_CommandPortEnable, 100, NULL);
    nanosleep(&sleep_time, NULL);

    OMX_SendCommand(audio_render, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    nanosleep(&sleep_time, NULL);

    return 0;
}

static int buf_idx = 0;

static int omxr_audio_process_frame(struct ouvr_ctx *ctx, struct ouvr_packet *pkt){
    OMX_ERRORTYPE err;
    unsigned int remaining_bytes = (unsigned int)pkt->size;
    unsigned char *pos = pkt->data;

    OMX_BUFFERHEADERTYPE *buf = omx_buffer[buf_idx];
    buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
    buf->nOffset = 0;
    buf->nFilledLen = ((OMX_U32)remaining_bytes < buf->nAllocLen ? (OMX_U32)remaining_bytes : buf->nAllocLen);
    memcpy(buf->pBuffer, pos, buf->nFilledLen);

    remaining_bytes -= buf->nFilledLen;
    pos += buf->nFilledLen;

    if(remaining_bytes == 0) {
        buf->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
    }

    err = OMX_EmptyThisBuffer(audio_render, buf);
    if (err != OMX_ErrorNone)
    {
        printf("OMX_EmptyThisBuffer() returned %x\n", err);
        return -1;
    }
    //buf_idx = (buf_idx + 1) % NUM_BUFS;
    // struct timespec sleep_time2 = {.tv_sec = 0, .tv_nsec = 20000000};
    // nanosleep(&sleep_time2, NULL);
        
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
    //printf("empty buffer done callback\n");
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE fill_buffer_done_callback(OMX_HANDLETYPE hComponent, OMX_PTR nAppData, OMX_BUFFERHEADERTYPE *pBuffer)
{
    printf("fill buffer done callback\n");
    return OMX_ErrorNone;
}

static void omxr_audio_deinit(struct ouvr_ctx *ctx){
    OMX_ERRORTYPE err;
    printf("deinit\n");
    nanosleep(&sleep_time, NULL);
    for(int i = 0; i < NUM_BUFS; i++) {
        err = OMX_FreeBuffer(audio_render, 100, omx_buffer[i]);
        if(err != OMX_ErrorNone)
        {
            printf("OMX_FreeBuffer() returned %x\n", err);
            return -1;
        }
    }
    OMX_Deinit();
    bcm_host_deinit();
    printf("deinited\n");
}

struct ouvr_audio openmax_audio = {
    .init = omxr_audio_init,
    .process_frame = omxr_audio_process_frame,
    .deinit = omxr_audio_deinit,
};