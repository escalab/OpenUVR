#ifndef OUVR_RAW_H
#define OUVR_RAW_H

#include "ouvr_packet.h"

struct ouvr_network raw_handler;
//int raw_receive_and_decode();
int raw_receive_and_decode(struct ouvr_ctx *ctx);

#endif