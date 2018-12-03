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

int feedback_initialize(struct owvr_ctx *ctx)
{
    feedback_net_context *c = &fb_net;

    c->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (c->fd < 0)
    {
        printf("Couldn't create socket\n");
        return -1;
    }

    c->serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &c->serv_addr.sin_addr.s_addr);
    c->serv_addr.sin_port = htons(SERVER_PORT_FEEDBACK);

    c->cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &c->cli_addr.sin_addr.s_addr);
    c->cli_addr.sin_port = htons(CLIENT_PORT_FEEDBACK);

    if (bind(c->fd, (struct sockaddr *)&c->serv_addr, sizeof(c->serv_addr)) < 0)
    {
        printf("Couldn't bind feedback\n");
        return -1;
    }
    if (connect(c->fd, (struct sockaddr *)&c->cli_addr, sizeof(c->cli_addr)) < 0)
    {
        printf("Couldn't connect feedback\n");
        return -1;
    }
    int flags = fcntl(c->fd, F_GETFL, 0);
    fcntl(c->fd, F_SETFL, flags | (int)O_NONBLOCK);

    c->msg.msg_iov = c->iov;
    c->msg.msg_iovlen = 1;
    return 0;
}

int feedback_receive(struct owvr_ctx *ctx)
{
    feedback_net_context *c = &fb_net;
    register ssize_t r;
    int to_recv = 0;
    c->iov[0].iov_len = sizeof(to_recv);
    c->iov[0].iov_base = &to_recv;

    r = recvmsg(c->fd, &c->msg, 0);
    if (r < -1)
    {
        printf("Error on sendmsg: %ld\n", r);
        return -1;
    }
    else if (r >= 0 && ctx->flag_send_iframe == 0)
    {
        ctx->flag_send_iframe = to_recv;
    }
    return 0;
}