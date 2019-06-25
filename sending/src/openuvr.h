#ifndef OPENUVR_MAIN_H
#define OPENUVR_MAIN_H

#include <stdint.h>

enum OPENUVR_NETWORK_TYPE
{
    OPENUVR_NETWORK_TCP,
    OPENUVR_NETWORK_UDP,
    OPENUVR_NETWORK_RAW,
    OPENUVR_NETWORK_RAW_RING,
    OPENUVR_NETWORK_INJECT,
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

struct openuvr_context *openuvr_alloc_context(enum OPENUVR_ENCODER_TYPE enc_type, enum OPENUVR_NETWORK_TYPE net_type, uint8_t *pix_buf, unsigned int pbo);
int openuvr_send_frame(struct openuvr_context *context);
int openuvr_init_thread(struct openuvr_context *context);
int openuvr_init_thread_continuous(struct openuvr_context *context);
int openuvr_cuda_copy(struct openuvr_context *context);
void openuvr_close(struct openuvr_context *context);

//"managed" functions which declare and manage the opengl buffers
int openuvr_managed_init(enum OPENUVR_ENCODER_TYPE enc_type, enum OPENUVR_NETWORK_TYPE net_type);
void openuvr_managed_copy_framebuffer();

#endif