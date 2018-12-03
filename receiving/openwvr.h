#ifndef OPENWVR_MAIN_H
#define OPENWVR_MAIN_H

enum OPENWVR_NETWORK_TYPE
{
    OPENWVR_NETWORK_TCP,
    OPENWVR_NETWORK_UDP,
    OPENWVR_NETWORK_RAW,
    OPENWVR_NETWORK_UDP_COMPAT,
};

enum OPENWVR_DECODER_TYPE
{
    OPENWVR_DECODER_H264,
    OPENWVR_DECODER_RGB,
};

struct openwvr_context
{
    void *priv;
};

struct openwvr_context *openwvr_alloc_context(enum OPENWVR_DECODER_TYPE dec_type, enum OPENWVR_NETWORK_TYPE net_type);
int openwvr_receive_frame(struct openwvr_context *context);
int openwvr_receive_loop(struct openwvr_context *context);
int openwvr_receive_frame_raw_h264(struct openwvr_context *context);

#endif
