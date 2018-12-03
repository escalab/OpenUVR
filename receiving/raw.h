#ifndef OWVR_RAW_H
#define OWVR_RAW_H

#include "owvr_packet.h"

struct owvr_network raw_handler;
//int raw_receive_and_decode();
int raw_receive_and_decode(struct owvr_ctx *ctx);

#endif