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

// #define SERVER_IP 0xc0a80102
// #define CLIENT_IP 0xc0a80103

#define SERVER_PORT_FEEDBACK 21223
#define CLIENT_PORT_FEEDBACK 21224

typedef struct feedback_net_context
{
    int fd;
    struct sockaddr_in serv_addr, cli_addr;
    struct msghdr msg;
    struct iovec iov[3];
} feedback_net_context;

static feedback_net_context fb_net;

int feedback_initialize(struct ouvr_ctx *ctx)
{
    feedback_net_context *c = &fb_net;

    c->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (c->fd < 0){
        printf("Couldn't create socket\n");
        return -1;
    }

    c->serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &c->serv_addr.sin_addr.s_addr);
    c->serv_addr.sin_port = htons(SERVER_PORT_FEEDBACK);

    c->cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &c->cli_addr.sin_addr.s_addr);
    c->cli_addr.sin_port = htons(CLIENT_PORT_FEEDBACK);

    if(bind(c->fd, (struct sockaddr *)&c->cli_addr, sizeof(c->cli_addr)) < 0){
        printf("Couldn't bind feedback\n");
        return -1;
    }
    if(connect(c->fd, (struct sockaddr *)&c->serv_addr, sizeof(c->serv_addr)) < 0){
        printf("Couldn't connect feedback\n");
        return -1;
    }
    int flags = fcntl(c->fd, F_GETFL, 0);
    fcntl(c->fd, F_SETFL, flags | (int)O_NONBLOCK);
    
    c->msg.msg_iov = c->iov;
    c->msg.msg_iovlen = 1;
    return 0;
}

int feedback_send(struct ouvr_ctx *ctx)
{
    feedback_net_context *c = &fb_net;
    if (!ctx->flag_send_iframe) {
        return 0;
    }
    register ssize_t r;
    // tells sending side to ignore any feedback we send for the next [to_send] frames
    // which prevents sending too much bandwidth with several iframes in a row
    int to_send = ctx->flag_send_iframe;
    c->iov[0].iov_len = sizeof(to_send);
    c->iov[0].iov_base = &to_send;

    r = sendmsg(c->fd, &c->msg, 0);
    if (r < -1)
    {
        printf("Reading error: %d\n", r);
        return -1;
    }
    return 0;
}
