#ifndef OPENWVR_MAIN_H
#define OPENWVR_MAIN_H

enum OPENWVR_NETWORK_TYPE
{
    OPENWVR_NETWORK_TCP,
    OPENWVR_NETWORK_UDP,
    OPENWVR_NETWORK_RAW,
    OPENWVR_NETWORK_UDP_COMPAT,
};

enum OPENWVR_ENCODER_TYPE
{
    OPENWVR_ENCODER_H264,
    OPENWVR_ENCODER_H264_CUDA,
    OPENWVR_ENCODER_RGB,
};

struct openwvr_context
{
    void *priv;
};

struct openwvr_context *openwvr_alloc_context(enum OPENWVR_ENCODER_TYPE enc_type, enum OPENWVR_NETWORK_TYPE net_type, unsigned char *pix_buf);
int openwvr_send_frame(struct openwvr_context *context);
int openwvr_init_thread(struct openwvr_context *context);
int openwvr_send_frame_async(struct openwvr_context *context);
int openwvr_init_thread_continuous(struct openwvr_context *context);
int openwvr_cuda_copy(struct openwvr_context *context, void *ptr);
void openwvr_close(struct openwvr_context *context);

#endif