#include "ouvr_packet.h"
#include <stdlib.h>

struct ouvr_packet *ouvr_packet_alloc() {
    struct ouvr_packet *pkt = malloc(sizeof(struct ouvr_packet));
    pkt->data = malloc(10000000);
    pkt->size = 0;
    return pkt;
}

void ouvr_packet_free(struct ouvr_packet *pkt) {
    free(pkt->data);
    free(pkt);
}