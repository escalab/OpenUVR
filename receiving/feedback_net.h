#ifndef OWVR_FBNET_H
#define OWVR_FBNET_H

#include "owvr_packet.h"

int feedback_initialize(struct owvr_ctx *ctx);
int feedback_send(struct owvr_ctx *ctx);

#endif