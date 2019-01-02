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

typedef struct tcp_net_context
{
    int fd;
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
    if (c->fd < 0) {
        printf("Couldn't create socket\n");
        return -1;
    }

    int reuseaddr_val = 1;
    setsockopt(c->fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_val, sizeof(int));
    
    c->serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &c->serv_addr.sin_addr.s_addr);
    c->serv_addr.sin_port = htons(SERVER_PORT_BUFFER);

    c->cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &c->cli_addr.sin_addr.s_addr);
    c->cli_addr.sin_port = htons(CLIENT_PORT_BUFFER);

    if (bind(c->fd, (struct sockaddr *)&c->cli_addr, sizeof(c->cli_addr)) < 0) {
        printf("Couldn't bind\n");
        return -1;
    }
    if (connect(c->fd, (struct sockaddr *)&c->serv_addr, sizeof(c->serv_addr)) < 0) {
        printf("Couldn't connect\n");
        return -1;
    }

    
    return 0;
}

static int tcp_receive_packet(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
#ifdef TIME_NETWORK
    struct timeval start_time, end_time;
    int has_received_first = 0;
#endif
    tcp_net_context *c = ctx->net_priv;

    register int r;
    unsigned char *pos = pkt->data;
    int nleft = 0;
    r = read(c->fd, &nleft, sizeof(nleft));
    if(r != sizeof(nleft)) {
        printf("Error reading nleft: %d\n", r);
        return -1;
    }
    pkt->size = nleft;

#ifdef TIME_NETWORK
    gettimeofday(&start_time, NULL);
#endif

    while (nleft > 0)
    {
        r = read(c->fd, pos, nleft);
        if (r < -1)
        {
            printf("Reading error: %d\n", r);
            return -1;
        }
        else if (r > 0)
        {
            pos += r;
            nleft -= r;
        }
    }

#ifdef TIME_NETWORK
    gettimeofday(&end_time, NULL);
    printf("%ld\n", end_time.tv_usec - start_time.tv_usec + (end_time.tv_sec - start_time.tv_sec)* 1000000);
#endif

    return 0;
}

struct ouvr_network tcp_handler = {
    .init = tcp_initialize,
    .recv_packet = tcp_receive_packet,
};
