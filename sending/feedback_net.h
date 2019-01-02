#ifndef OUVR_FBNET_H
#define OUVR_FBNET_H

#include "ouvr_packet.h"

int feedback_initialize(struct ouvr_ctx *ctx);
int feedback_receive(struct ouvr_ctx *ctx);

#endif