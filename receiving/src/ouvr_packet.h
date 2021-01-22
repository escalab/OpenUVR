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
