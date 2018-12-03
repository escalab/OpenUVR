#include "udp.h"
#include "owvr_packet.h"
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

// #define SERVER_IP 0xc0a80102
// #define CLIENT_IP 0xc0a80103

#define SERVER_PORT_BUFFER 21221
#define CLIENT_PORT_BUFFER 21222

#define SEND_SIZE 1450

typedef struct udp_net_context
{
    int fd;
    struct sockaddr_in serv_addr, cli_addr;
    struct msghdr msg;
    struct iovec iov[3];
} udp_net_context;

static int udp_initialize(struct owvr_ctx *ctx)
{
    if (ctx->net_priv != NULL)
    {
        free(ctx->net_priv);
    }
    udp_net_context *c = calloc(1, sizeof(udp_net_context));
    ctx->net_priv = c;
    c->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (c->fd < 0)
    {
        printf("Couldn't create udp socket\n");
        return -1;
    }

    c->serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &c->serv_addr.sin_addr.s_addr); //c->serv_addr.sin_addr.s_addr = htonl(SERVER_IP);
    c->serv_addr.sin_port = htons(SERVER_PORT_BUFFER);

    c->cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &c->cli_addr.sin_addr.s_addr); //c->cli_addr.sin_addr.s_addr = htonl(CLIENT_IP);
    c->cli_addr.sin_port = htons(CLIENT_PORT_BUFFER);

    if (bind(c->fd, (struct sockaddr *)&c->serv_addr, sizeof(c->serv_addr)) < 0)
    {
        printf("Couldn't bind udp\n");
        return -1;
    }
    if (connect(c->fd, (struct sockaddr *)&c->cli_addr, sizeof(c->cli_addr)) < 0)
    {
        printf("Couldn't connect udp\n");
        return -1;
    }
    int flags = fcntl(c->fd, F_GETFL, 0);
    fcntl(c->fd, F_SETFL, flags | (int)O_NONBLOCK);

    c->msg.msg_iov = c->iov;
    c->msg.msg_iovlen = 2;
    return 0;
}

static int udp_send_packet(struct owvr_ctx *ctx, struct owvr_packet *pkt)
{
    udp_net_context *c = ctx->net_priv;
    register ssize_t r;
    unsigned char *start_pos = pkt->data;
    int offset = 0;
    c->iov[0].iov_len = sizeof(pkt->size);
    c->iov[0].iov_base = &(pkt->size);
    c->iov[1].iov_len = SEND_SIZE;
    while (offset < pkt->size)
    {
        c->iov[1].iov_base = start_pos + offset;
        r = sendmsg(c->fd, &c->msg, 0);
        if (r < -1)
        {
            printf("Error on sendmsg: %ld\n", r);
            return -1;
        }
        else if (r > 0)
        {
            offset += c->iov[1].iov_len;
            if (offset + SEND_SIZE > pkt->size)
            {
                c->iov[1].iov_len = pkt->size - offset;
            }
        }
    }
    return 0;
}

struct owvr_network udp_handler = {
    .init = udp_initialize,
    .send_packet = udp_send_packet,
};
