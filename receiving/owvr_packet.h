#ifndef OWVR_PACKET_H
#define OWVR_PACKET_H
struct owvr_packet;
struct owvr_network;
struct owvr_encoder;
struct owvr_ctx;

// #define SERVER_IP "172.16.38.214"
// #define CLIENT_IP "172.16.44.23"

#define SERVER_IP "192.168.1.2"
#define CLIENT_IP "192.168.1.3"

struct owvr_packet
{
    unsigned char *data;
    int size;
};

struct owvr_packet *owvr_packet_alloc();
void owvr_packet_free(struct owvr_packet *pkt);

struct owvr_network
{
    int (*init)(struct owvr_ctx *ctx);
    int (*recv_packet)(struct owvr_ctx *ctx, struct owvr_packet *pkt);
};

struct owvr_decoder
{
    int (*init)(struct owvr_ctx *ctx);
    int (*process_frame)(struct owvr_ctx *ctx, struct owvr_packet *pkt);
    void (*deinit)(struct owvr_ctx *ctx);
};

struct owvr_audio
{
    int (*init)(struct owvr_ctx *ctx);
    int (*process_frame)(struct owvr_ctx *ctx, struct owvr_packet *pkt);
    void (*deinit)(struct owvr_ctx *ctx);
};

struct owvr_ctx
{
    struct owvr_network *net;
    void *net_priv;
    struct owvr_decoder *dec;
    void *dec_priv;
    struct owvr_audio *aud;
    void *aud_priv;
    int num_packets;
    struct owvr_packet **packets;
    int flag_send_iframe;
};

#endif
