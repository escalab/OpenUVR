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
// to prevent SIGPIPE terminating process when writing to socket that has been closed
#include <signal.h>

#include <time.h>

#define SERVER_PORT_BUFFER 21221
#define CLIENT_PORT_BUFFER 21222

typedef struct tcp_net_context
{
    int fd, send_fd;
    struct sockaddr_in serv_addr, cli_addr;
} tcp_net_context;

static int tcp_initialize(struct ouvr_ctx *ctx)
{
    if (ctx->net_priv != NULL)
    {
        free(ctx->net_priv);
    }
    tcp_net_context *c = calloc(1, sizeof(tcp_net_context));
    ctx->net_priv = c;

    c->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (c->fd < 0)
    {
        PRINT_ERR("Couldn't create socket\n");
        return -1;
    }

    signal(SIGPIPE, SIG_IGN);

    c->serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &c->serv_addr.sin_addr.s_addr);
    c->serv_addr.sin_port = htons(SERVER_PORT_BUFFER);

    if (bind(c->fd, (struct sockaddr *)&c->serv_addr, sizeof(c->serv_addr)) < 0)
    {
        PRINT_ERR("Couldn't bind\n");
        return -1;
    }

    c->send_fd = -1;

    return 0;
}

static int tcp_send_packet(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    tcp_net_context *c = ctx->net_priv;

    register int r;

    if (c->send_fd == -1)
    {
        socklen_t socklen = sizeof(c->cli_addr);
        listen(c->fd, 10);
        c->send_fd = accept(c->fd, (struct sockaddr *)&c->cli_addr, &socklen);
    }

    int nleft = pkt->size;
    r = write(c->send_fd, &nleft, sizeof(nleft));
    if (r != sizeof(nleft))
    {
        PRINT_ERR("Error on writing num left: returned %d\n", r);
        close(c->send_fd);
        c->send_fd = -1;
        return 0;
    }

    uint8_t *pos = pkt->data;
    while (nleft > 0)
    {
        r = write(c->send_fd, pos, nleft);
        if (r < 0)
        {
            PRINT_ERR("Error on write: returned %d\n", r);
            close(c->send_fd);
            c->send_fd = -1;
            return 0;
        }
        else if (r > 0)
        {
            pos += r;
            nleft -= r;
        }
    }
    return 0;
}

struct ouvr_network tcp_handler = {
    .init = tcp_initialize,
    .send_packet = tcp_send_packet,
};
