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

enum OPENUVR_NETWORK_TYPE
{
    OPENUVR_NETWORK_TCP,
    OPENUVR_NETWORK_UDP,
    OPENUVR_NETWORK_RAW,
    OPENUVR_NETWORK_UDP_COMPAT,
};

enum OPENUVR_DECODER_TYPE
{
    OPENUVR_DECODER_H264,
    OPENUVR_DECODER_RGB,
};

struct openuvr_context
{
    void *priv;
};

struct openuvr_context *openuvr_alloc_context(enum OPENUVR_DECODER_TYPE dec_type, enum OPENUVR_NETWORK_TYPE net_type);
int openuvr_receive_frame(struct openuvr_context *context);
int openuvr_receive_loop(struct openuvr_context *context);
int openuvr_receive_frame_raw_h264(struct openuvr_context *context);

#endif
