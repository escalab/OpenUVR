#ifndef OPENUVR_MAIN_H
#define OPENUVR_MAIN_H

enum OPENUVR_NETWORK_TYPE
{
    OPENUVR_NETWORK_TCP,
    OPENUVR_NETWORK_UDP,
    OPENUVR_NETWORK_RAW,
    OPENUVR_NETWORK_UDP_COMPAT,
};

enum OPENUVR_ENCODER_TYPE
{
    OPENUVR_ENCODER_H264,
    OPENUVR_ENCODER_H264_CUDA,
    OPENUVR_ENCODER_RGB,
};

struct openuvr_context
{
    void *priv;
};

struct openuvr_context *openuvr_alloc_context(enum OPENUVR_ENCODER_TYPE enc_type, enum OPENUVR_NETWORK_TYPE net_type, unsigned char *pix_buf);
int openuvr_send_frame(struct openuvr_context *context);
int openuvr_init_thread(struct openuvr_context *context);
int openuvr_send_frame_async(struct openuvr_context *context);
int openuvr_init_thread_continuous(struct openuvr_context *context);
int openuvr_cuda_copy(struct openuvr_context *context, void *ptr);
void openuvr_close(struct openuvr_context *context);

#endif