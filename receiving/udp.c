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
#include <sys/time.h>

// #define SERVER_IP 0xc0a80102
// #define CLIENT_IP 0xc0a80103

#define SERVER_PORT_BUFFER 21221
#define CLIENT_PORT_BUFFER 21222

#define RECV_SIZE 1450

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
    if (c->fd < 0){
        printf("Couldn't create socket\n");
        return -1;
    }

    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_addr.s_addr = htonl(SERVER_IP);
    // serv_addr.sin_port = htons(SERVER_PORT_BUFFER);

    c->cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &c->cli_addr.sin_addr.s_addr); //c->cli_addr.sin_addr.s_addr = htonl(CLIENT_IP);
    c->cli_addr.sin_port = htons(CLIENT_PORT_BUFFER);

    if(bind(c->fd, (struct sockaddr *)&c->cli_addr, sizeof(c->cli_addr)) < 0){
        printf("Couldn't bind udp\n");
        return -1;
    }
    // if(connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    //     printf("Couldn't connect\n");
    //     return -1;
    // }
    int flags = fcntl(c->fd, F_GETFL, 0);
    fcntl(c->fd, F_SETFL, flags | (int)O_NONBLOCK);
    srand(17); 
    c->msg.msg_iov = c->iov;
    c->msg.msg_iovlen = 2;
    return 0;
}

#ifdef TIME_NETWORK
    static float avg_time = 0;
#endif

static int udp_receive_packet(struct owvr_ctx *ctx, struct owvr_packet *pkt)
{
#ifdef TIME_NETWORK
    struct timeval start_time, end_time;
    int has_received_first = 0;
#endif
    udp_net_context *c = ctx->net_priv;
    register ssize_t r;
    unsigned char *start_pos = pkt->data;
    int offset = 0;
    int expected_size = 1;
    struct timespec time_of_last_receive = {.tv_sec = 0}, time_temp;
    pkt->size = 0;
    c->iov[0].iov_len = sizeof(expected_size);
    c->iov[0].iov_base = &expected_size;
    c->iov[1].iov_len = RECV_SIZE;
    while (offset < expected_size)
    {
        c->iov[1].iov_base = start_pos + offset;
        r = recvmsg(c->fd, &c->msg, 0);
	if (r < -1)
        {
            printf("Reading error: %d\n", r);
            return -1;
        }
        else if (r > 0)
        {
#ifdef TIME_NETWORK
            if(!has_received_first){
                gettimeofday(&start_time, NULL);
                has_received_first = 1;
            }
#endif
            //printf("%d\n",expected_size);
            offset += r - sizeof(expected_size);
            pkt->size += r - sizeof(expected_size);
            clock_gettime(CLOCK_MONOTONIC, &time_of_last_receive);
        }
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &time_temp);
            long elapsed = time_temp.tv_nsec - time_of_last_receive.tv_nsec;
            if(time_temp.tv_sec > time_of_last_receive.tv_sec)
                elapsed += 1000000000;
            if(elapsed > 3000000 && time_of_last_receive.tv_sec > 0) {
                pkt->size = 0;
                ctx->flag_send_iframe = 5;
                break;
            }
        }
    }
#ifdef TIME_NETWORK
    gettimeofday(&end_time, NULL);
    long elapsed = end_time.tv_usec - start_time.tv_usec + (end_time.tv_sec > start_time.tv_sec ? 1000000 : 0);
    avg_time = 0.998 * avg_time + 0.002 * elapsed;
    printf("\rnet avg: %f,  elapsed: %ld", avg_time, elapsed);
#endif
    if(!(rand() % 60) && 0) {
	    printf("dropped\n");
	    pkt->size = 0;
	    ctx->flag_send_iframe = 5;
    }
   // printf("%d\n", pkt->size);
    return 0;
}

struct owvr_network udp_handler = {
    .init = udp_initialize,
    .recv_packet = udp_receive_packet,
};
