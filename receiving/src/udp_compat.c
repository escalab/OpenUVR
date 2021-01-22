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
#include "udp.h"
#include "ouvr_packet.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// for memset
#include <string.h>

#include <time.h>
#include <sys/time.h>

#define SERVER_PORT_BUFFER 21221
#define CLIENT_PORT_BUFFER 21222

#define RECV_SIZE 2048

typedef struct udp_compat_net_context
{
    int fd;
    struct sockaddr_in serv_addr, cli_addr;
    struct msghdr msg;
    struct iovec iov[3];
} udp_compat_net_context;

static int udp_initialize(struct ouvr_ctx *ctx)
{
    if (ctx->net_priv != NULL)
    {
        free(ctx->net_priv);
    }
    udp_compat_net_context *c = calloc(1, sizeof(udp_compat_net_context));
    ctx->net_priv = c;

    c->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (c->fd < 0){
        printf("Couldn't create socket\n");
        return -1;
    }

    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_addr.s_addr = htonl(SERVER_IP);
    // serv_addr.sin_port = htons(SERVER_PORT_BUFFER);

    c->cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &c->cli_addr.sin_addr.s_addr);
    c->cli_addr.sin_port = htons(CLIENT_PORT_BUFFER);

    if(bind(c->fd, (struct sockaddr *)&c->cli_addr, sizeof(c->cli_addr)) < 0){
        printf("Couldn't bind\n");
        return -1;
    }
    // if(connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    //     printf("Couldn't connect\n");
    //     return -1;
    // }
    int flags = fcntl(c->fd, F_GETFL, 0);
    fcntl(c->fd, F_SETFL, flags | (int)O_NONBLOCK);
    
    c->msg.msg_iov = c->iov;
    c->msg.msg_iovlen = 1;
    return 0;
}

static int udp_receive_packet(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    udp_compat_net_context *c = ctx->net_priv;
    register ssize_t r;
    unsigned char *start_pos = pkt->data;
    int offset = 0;
    struct timespec time_of_last_receive = {.tv_sec = 0, .tv_nsec = 0}, time_temp;
    pkt->size = 0;
    c->iov[0].iov_len = RECV_SIZE;
    while (1)
    {
        c->iov[0].iov_base = start_pos + offset;
        r = recvmsg(c->fd, &c->msg, 0);
        if (r < -1)
        {
            printf("Reading error: %d\n", r);
            return -1;
        }
        else if (r > 0)
        {
            //printf("%d\n",expected_size);
            offset += r;
            pkt->size += r;
            clock_gettime(CLOCK_MONOTONIC, &time_of_last_receive);
        }
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &time_temp);
            long elapsed = time_temp.tv_nsec - time_of_last_receive.tv_nsec;
            if(time_temp.tv_sec > time_of_last_receive.tv_sec)
                elapsed += 1000000000;
            if(elapsed > 3000000 && time_of_last_receive.tv_sec > 0)
                break;
        }
    }
    return 0;
}

struct ouvr_network udp_compat_handler = {
    .init = udp_initialize,
    .recv_packet = udp_receive_packet,
};