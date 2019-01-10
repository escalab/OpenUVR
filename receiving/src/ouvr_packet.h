#ifndef OUVR_PACKET_H
#define OUVR_PACKET_H
struct ouvr_packet;
struct ouvr_network;
struct ouvr_encoder;
struct ouvr_ctx;

// #define SERVER_IP "172.16.38.214"
// #define CLIENT_IP "172.16.44.23"

#define SERVER_IP "192.168.1.2"
#define CLIENT_IP "192.168.1.3"

struct ouvr_packet
{
    unsigned char *data;
    int size;
};

struct ouvr_packet *ouvr_packet_alloc();
void ouvr_packet_free(struct ouvr_packet *pkt);

struct ouvr_network
{
    int (*init)(struct ouvr_ctx *ctx);
    int (*recv_packet)(struct ouvr_ctx *ctx, struct ouvr_packet *pkt);
};

struct ouvr_decoder
{
    int (*init)(struct ouvr_ctx *ctx);
    int (*process_frame)(struct ouvr_ctx *ctx, struct ouvr_packet *pkt);
    void (*deinit)(struct ouvr_ctx *ctx);
};

struct ouvr_audio
{
    int (*init)(struct ouvr_ctx *ctx);
    int (*process_frame)(struct ouvr_ctx *ctx, struct ouvr_packet *pkt);
    void (*deinit)(struct ouvr_ctx *ctx);
};

struct ouvr_ctx
{
    struct ouvr_network *net;
    void *net_priv;
    struct ouvr_decoder *dec;
    void *dec_priv;
    struct ouvr_audio *aud;
    void *aud_priv;
    int num_packets;
    struct ouvr_packet **packets;
    int flag_send_iframe;
};

#endif
