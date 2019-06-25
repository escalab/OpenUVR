#include "raw_ring.h"
#include "ouvr_packet.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/in.h>
// for memset
#include <string.h>
// for ETH_P_802_EX1
#include <linux/if_ether.h>
// for sockaddr_ll
#include <linux/if_packet.h>
#include <sys/mman.h>

#include <time.h>
#include <errno.h>
#include <poll.h>

#define MY_SLL_IFINDEX 3

#define SEND_SIZE 1450
#define NUM_BLOCKS 8
#define NUM_FRAMES (NUM_BLOCKS * 2)
#define FRAME_SIZE 2048

typedef struct raw_ring_net_context
{
    int fd;
    struct sockaddr_ll raw_addr;
    uint8_t *ring_buf;
} raw_ring_net_context;

static uint8_t const global_eth_header[14] = {0xb8, 0x27, 0xeb, 0x6c, 0xa7, 0xdd, 0x00, 0x0e, 0x8e, 0x5c, 0x2e, 0x53, 0x88, 0xb5};
static const uint8_t ipllc[8] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0xb5};

struct ieee80211_hdr
{
    uint16_t /*__le16*/ frame_control;
    uint16_t /*__le16*/ duration_id;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t /*__le16*/ seq_ctrl;
    //uint8_t addr4[6];
} __attribute__((packed));

static int raw_ring_initialize(struct ouvr_ctx *ctx)
{
    if (ctx->net_priv != NULL)
    {
        free(ctx->net_priv);
    }
    raw_ring_net_context *c = calloc(1, sizeof(raw_ring_net_context));
    ctx->net_priv = c;
    c->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_802_EX1));
    if (c->fd < 0)
    {
        PRINT_ERR("Couldn't create socket\n");
        return -1;
    }

    struct tpacket_req txr_req = {
        .tp_block_size = 4096,
        .tp_frame_size = FRAME_SIZE,
        .tp_block_nr = NUM_BLOCKS,
        .tp_frame_nr = NUM_FRAMES,
    };

    if (setsockopt(c->fd, SOL_PACKET, PACKET_TX_RING, &txr_req, sizeof(txr_req)) != 0)
    {
        PRINT_ERR("Couldn't create tx ring\n");
        return -1;
    }
    int qdisc_opt = 1;
    if (setsockopt(c->fd, SOL_PACKET, PACKET_QDISC_BYPASS, &qdisc_opt, sizeof(qdisc_opt)) != 0)
    {
        PRINT_ERR("Couldn't set qdisc option\n");
        return -1;
    }

    c->raw_addr.sll_family = AF_PACKET;
    c->raw_addr.sll_ifindex = MY_SLL_IFINDEX;
    if (bind(c->fd, (struct sockaddr *)&c->raw_addr, sizeof(c->raw_addr)) == -1)
    {
        printf("bind bad\n");
        exit(1);
    }

    c->ring_buf = mmap(0, NUM_BLOCKS * 4096, PROT_READ | PROT_WRITE, MAP_SHARED, c->fd, 0);
    if (c->ring_buf == NULL)
    {
        PRINT_ERR("Couldn't map tx ring buffer\n");
        exit(1);
    }

    const uint8_t static_header[] = {0xb8, 0x27, 0xeb, 0x6c, 0xa7, 0xdd, 0x00, 0x0e, 0x8e, 0x5c, 0x2e, 0x53, 0x88, 0xb5};
    for (int i = 0; i < NUM_FRAMES; i++)
    {
        uint8_t *cur_frame = c->ring_buf + i * FRAME_SIZE;
        memcpy(cur_frame + 32, static_header, sizeof(static_header));
    }
    return 0;
}

static int ring_idx = 0;

static int raw_ring_send_packet(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    raw_ring_net_context *c = ctx->net_priv;
    uint8_t *cur = pkt->data;
    uint8_t *end = pkt->data + pkt->size;
    while (cur < end)
    {
        uint8_t *frame_offset = c->ring_buf + (ring_idx * FRAME_SIZE);
        struct tpacket_hdr *header = (struct tpacket_hdr *)frame_offset;
        // if (header->tp_status != TP_STATUS_AVAILABLE)
        // {
        //     PRINT_ERR("Shouldn't Happen\n");
        //     exit(1);
        //     // struct pollfd pollset;
        //     // pollset.fd = c->fd;
        //     // pollset.events = POLLOUT;
        //     // pollset.revents = 0;
        //     // int ret = poll(&pollset, 1, 100);
        //     // if (ret < 0)
        //     // {
        //     //     PRINT_ERR("poll failed\n");
        //     //     exit(1);
        //     // }
        // }
        uint8_t *data = frame_offset + 32 + 14;
        *(int *)data = pkt->size;
        memcpy(data + sizeof(pkt->size), cur, SEND_SIZE);
        header->tp_len = SEND_SIZE + 14;
        header->tp_status = 1;
        ring_idx = (ring_idx + 1) % NUM_FRAMES;
        cur += SEND_SIZE;
    }

    ssize_t r = send(c->fd, 0, 0, MSG_DONTWAIT);
    if (r < 0)
    {
        PRINT_ERR("send returned %ld, errno=%d\n", r, errno);
        return -1;
    }

    return 0;
}

static void raw_ring_deinitialize(struct ouvr_ctx *ctx)
{
    raw_ring_net_context *c = ctx->net_priv;
    close(c->fd);
    free(ctx->net_priv);
}

struct ouvr_network raw_ring_handler = {
    .init = raw_ring_initialize,
    .send_packet = raw_ring_send_packet,
    .deinit = raw_ring_deinitialize,
};