/*
    The MIT License (MIT)

    Copyright (c) 2020 OpenUVR

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Authors:
    Alec Rohloff
    Zackary Allen
    Kung-Min Lin
    Chengyi Nie
    Hung-Wei Tseng
*/

#ifndef OPENUVR_MAIN_H
#define OPENUVR_MAIN_H

#include <stdint.h>

enum OPENUVR_NETWORK_TYPE
{
    OPENUVR_NETWORK_TCP,
    OPENUVR_NETWORK_UDP,
    OPENUVR_NETWORK_RAW,
    OPENUVR_NETWORK_RAW_RING,
    OPENUVR_NETWORK_INJECT,
    OPENUVR_NETWORK_UDP_COMPAT,
};

enum OPENUVR_ENCODER_TYPE
{
    OPENUVR_ENCODER_H264,
    OPENUVR_ENCODER_H264_CUDA,
    OPENUVR_ENCODER_RGB,
};

struct openuvr_context
{
    void *priv;
};

struct openuvr_context *openuvr_alloc_context(enum OPENUVR_ENCODER_TYPE enc_type, enum OPENUVR_NETWORK_TYPE net_type, uint8_t *pix_buf, unsigned int pbo);
int openuvr_send_frame(struct openuvr_context *context);
int openuvr_init_thread(struct openuvr_context *context);
int openuvr_init_thread_continuous(struct openuvr_context *context);
int openuvr_cuda_copy(struct openuvr_context *context);
void openuvr_close(struct openuvr_context *context);

//"managed" functions which declare and manage the opengl buffers
int openuvr_managed_init(enum OPENUVR_ENCODER_TYPE enc_type, enum OPENUVR_NETWORK_TYPE net_type);
void openuvr_managed_copy_framebuffer();

#endif