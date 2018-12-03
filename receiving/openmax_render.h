#ifndef OPENMAX_RENDER_H
#define OPENMAX_RENDER_H

#define OMX_SKIP64BIT 1
#include <IL/OMX_Core.h>

#define print_state {\
        OMX_STATETYPE state;\
        OMX_GetState(video_decoder, &state);\
        printf("video_decoder is in state %d\n", state);\
}

struct owvr_decoder openmax_render;
int omxr_get_available_buffers(OMX_BUFFERHEADERTYPE **pointers, int num);
int omxr_empty_buffers(OMX_BUFFERHEADERTYPE **pointers, int num);

#endif