#ifndef OPENUVR_MAIN_H
#define OPENUVR_MAIN_H

enum OPENUVR_NETWORK_TYPE
{
    OPENUVR_NETWORK_TCP,
    OPENUVR_NETWORK_UDP,
    OPENUVR_NETWORK_RAW,
    OPENUVR_NETWORK_UDP_COMPAT,
};

enum OPENUVR_DECODER_TYPE
{
    OPENUVR_DECODER_H264,
    OPENUVR_DECODER_RGB,
};

struct openuvr_context
{
    void *priv;
};

struct openuvr_context *openuvr_alloc_context(enum OPENUVR_DECODER_TYPE dec_type, enum OPENUVR_NETWORK_TYPE net_type);
int openuvr_receive_frame(struct openuvr_context *context);
int openuvr_receive_loop(struct openuvr_context *context);
int openuvr_receive_frame_raw_h264(struct openuvr_context *context);

#endif
