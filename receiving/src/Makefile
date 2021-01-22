# The MIT License (MIT)

# Copyright (c) 2020 OpenUVR

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Authors:
# Alec Rohloff
# Zackary Allen
# Kung-Min Lin
# Chengyi Nie
# Hung-Wei Tseng
#

CFLAGS=-std=c11 -Wall -Wextra -D_GNU_SOURCE=1 -O3 -I/opt/vc/include $(TIME_FLAGS)
CC=gcc

OBJS=openuvr.o tcp.o udp.o udp_compat.o raw.o ouvr_packet.o openmax_render.o rgb_render.o openmax_audio.o ffmpeg_audio.o feedback_net.o input_send.o

.PHONY: all
all: openuvr

openuvr: main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -L/opt/vc/lib -lavcodec /usr/local/lib/libavutil.so -lopenmaxil -lbcm_host -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@rm -f openuvr main.o $(OBJS)

