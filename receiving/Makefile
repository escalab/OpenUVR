CFLAGS=-std=c11 -Wall -Wextra -D_GNU_SOURCE=1 -O3 -I/opt/vc/include $(TIME_FLAGS)
CC=gcc

OBJS=openwvr.o tcp.o udp.o udp_compat.o raw.o owvr_packet.o openmax_render.o rgb_render.o openmax_audio.o ffmpeg_audio.o feedback_net.o input_send.o

.PHONY: all
all: openwvr

openwvr: main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -L/opt/vc/lib -lavcodec /usr/local/lib/libavutil.so -lopenmaxil -lbcm_host -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@rm -f openwvr main.o $(OBJS)

