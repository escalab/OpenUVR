#ifndef OUVR_PACKET_H
#define OUVR_PACKET_H
struct ouvr_packet;
struct ouvr_network;
struct ouvr_encoder;
struct ouvr_audio;
struct ouvr_ctx;

// #define SERVER_IP "172.16.38.214"
// #define CLIENT_IP "172.16.44.23"

#define SERVER_IP "192.168.1.2"
#define CLIENT_IP "192.168.1.3"

#include <stdio.h>
#include <stdint.h>

#define PRINT_ERR(format, ...) fprintf(stderr, "\33[31;4mOpenUVR Error:%s:%d:\033[24m " format "\033[0m", __FILE__, __LINE__, ##__VA_ARGS__)

struct ouvr_packet
{
    uint8_t *data;
    int size;
};

struct ouvr_packet *ouvr_packet_alloc();
void ouvr_packet_free(struct ouvr_packet *pkt);

struct ouvr_network
{
    int (*init)(struct ouvr_ctx *ctx);
    int (*send_packet)(struct ouvr_ctx *ctx, struct ouvr_packet *pkt);
    void (*deinit)(struct ouvr_ctx *ctx);
};

struct ouvr_encoder
{
    int (*init)(struct ouvr_ctx *ctx);
    int (*process_frame)(struct ouvr_ctx *ctx, struct ouvr_packet *pkt);
    void (*cuda_copy)(struct ouvr_ctx *ctx);
    void (*deinit)(struct ouvr_ctx *ctx);
};

struct ouvr_audio
{
    int (*init)(struct ouvr_ctx *ctx);
    int (*encode_frame)(struct ouvr_ctx *ctx, struct ouvr_packet *pkt);
    void (*deinit)(struct ouvr_ctx *ctx);
};

struct ouvr_ctx
{
    struct ouvr_network *net;
    void *net_priv;
    struct ouvr_encoder *enc;
    void *enc_priv;
    //pointer to private data used by feedback_net
    void *fbn_priv;
    uint8_t *pix_buf;
    unsigned int pbo_handle;
    struct ouvr_audio *aud;
    void *aud_priv;
    struct ouvr_packet *packet;
    int flag_send_iframe;

    //contains variables for use in the main loops found in openuvr.c
    void *main_priv;
};

#endif