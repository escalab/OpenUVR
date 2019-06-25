#include "inject.h"
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
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define MY_SLL_IFINDEX 6

#define SEND_SIZE 1450

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
#define WLAN_FC_TYPE_DATA 2
#define WLAN_FC_SUBTYPE_DATA 0

static const uint8_t u8aRadiotapHeader[] = {0x00, 0x00, 0x18, 0x00, 0x0f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*0x10 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00};
const uint8_t ipllc[8] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0xb5};
static uint8_t *header_buf;
static uint8_t *data;

static long header_size;

typedef struct inject_net_context
{
    uint8_t eth_header[14];
    int fd;
    struct sockaddr_ll inject_addr;
    struct msghdr msg;
    struct iovec iov[3];
} inject_net_context;

static int inject_initialize(struct ouvr_ctx *ctx)
{
    if (ctx->net_priv != NULL)
    {
        free(ctx->net_priv);
    }
    inject_net_context *c = calloc(1, sizeof(inject_net_context));
    ctx->net_priv = c;
    c->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_802_EX1));
    if (c->fd < 0)
    {
        PRINT_ERR("Couldn't create socket\n");
        return -1;
    }

    //When you send packets, it is enough to specify sll_family, sll_addr, sll_halen, sll_ifindex, and sll_protocol.
    c->inject_addr.sll_family = AF_PACKET;
    //this is the hardcoded index of wlp2s0 device
    c->inject_addr.sll_ifindex = MY_SLL_IFINDEX;
    //ethertype ETH_P_802_EX1 (0x88b5) is reserved for private use
    c->inject_addr.sll_protocol = htons(ETH_P_802_EX1);
    memcpy(c->inject_addr.sll_addr, c->eth_header, 6);
    c->inject_addr.sll_halen = 6;
    c->inject_addr.sll_pkttype = PACKET_OTHERHOST;
    //msg.msg_flags = MSG_DONTWAIT;
    if (bind(c->fd, (struct sockaddr *)&c->inject_addr, sizeof(c->inject_addr)) == -1)
    {
        printf("bind bad\n");
        exit(1);
    }

    // int flags = fcntl(c->fd, F_GETFL, 0);
    // fcntl(c->fd, F_SETFL, flags | (int)O_NONBLOCK);

    c->msg.msg_name = &c->inject_addr;
    c->msg.msg_namelen = sizeof(c->inject_addr);
    c->msg.msg_control = 0;
    c->msg.msg_controllen = 0;
    c->msg.msg_iov = c->iov;
    c->msg.msg_iovlen = 1;

    /* The parts of our packet */
    uint8_t *rt; /* radiotap */
    struct ieee80211_hdr *hdr;
    uint8_t *llc;

    /* Other useful bits */
    uint8_t fcchunk[2]; /* 802.11 header frame control */

    header_size = sizeof(u8aRadiotapHeader) + sizeof(struct ieee80211_hdr) + sizeof(ipllc) + 4 + SEND_SIZE;
    header_buf = (uint8_t *)malloc(header_size);

    /* Put our pointers in the right place */
    rt = (uint8_t *)header_buf;
    hdr = (struct ieee80211_hdr *)(rt + sizeof(u8aRadiotapHeader));
    llc = (uint8_t *)(hdr + 1);
    data = (uint8_t *)(llc + sizeof(llc));

    /* The radiotap header has been explained already */
    memcpy(rt, u8aRadiotapHeader, sizeof(u8aRadiotapHeader));

    fcchunk[0] = ((WLAN_FC_TYPE_DATA << 2) | (WLAN_FC_SUBTYPE_DATA << 4));
    fcchunk[1] = 0x02;
    memcpy(&hdr->frame_control, &fcchunk[0], 2 * sizeof(uint8_t));

    hdr->duration_id = 0xffff;

    uint8_t dmac[6] = {0xb8, 0x27, 0xeb, 0x6c, 0xa7, 0xdd};
    uint8_t smac[6] = {0x00, 0x0e, 0x8e, 0x5c, 0x2e, 0x53};
    memcpy(&hdr->addr1[0], dmac, 6 * sizeof(uint8_t));
    memcpy(&hdr->addr2[0], smac, 6 * sizeof(uint8_t));
    memcpy(&hdr->addr3[0], smac, 6 * sizeof(uint8_t));
    hdr->seq_ctrl = 0;
    memcpy(llc, ipllc, 8 * sizeof(uint8_t));

    c->iov[0].iov_base = header_buf;
    c->iov[0].iov_len = header_size;

    return 0;
}

static int inject_send_packet(struct ouvr_ctx *ctx, struct ouvr_packet *pkt)
{
    inject_net_context *c = ctx->net_priv;
    register ssize_t r;
    uint8_t *start_pos = pkt->data;
    int offset = 0;
    int data_size = SEND_SIZE;
    memcpy(data, &pkt->size, 4);
    while (offset < pkt->size)
    {
        memcpy(data + 4, start_pos + offset, data_size);
        r = send(c->fd, header_buf, header_size, 0);
        if (r < -1)
        {
            PRINT_ERR("sendmsg returned %ld\n", r);
            return -1;
        }
        else if (r > 0)
        {
            offset += data_size;
            if (offset + SEND_SIZE > pkt->size)
            {
                data_size = pkt->size - offset;
            }
        }
    }
    return 0;
}

static void inject_deinitialize(struct ouvr_ctx *ctx)
{
    inject_net_context *c = ctx->net_priv;
    close(c->fd);
    free(ctx->net_priv);
}

struct ouvr_network inject_handler = {
    .init = inject_initialize,
    .send_packet = inject_send_packet,
    .deinit = inject_deinitialize,
};
