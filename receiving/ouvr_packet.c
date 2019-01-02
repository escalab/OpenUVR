#include "owvr_packet.h"
#include <stdlib.h>

struct owvr_packet *owvr_packet_alloc() {
    struct owvr_packet *pkt = malloc(sizeof(struct owvr_packet));
    pkt->data = malloc(10000000);
    pkt->size = 0;
    return pkt;
}

void owvr_packet_free(struct owvr_packet *pkt) {
    free(pkt->data);
    free(pkt);
}