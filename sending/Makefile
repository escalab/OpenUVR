CFLAGS=-std=c11 -fPIC -Wall -Wextra -D_GNU_SOURCE=1 -O3 -I/home/arrohlof/dev/openwvr_ffmpeg_4/build -I/home/arrohlof/dev/openwvr_ffmpeg_4/build/include -I/usr/local/cuda/include $(TIME_FLAGS)
CC=gcc

OBJS=owvr_packet.o tcp.o udp.o udp_compat.o raw.o ffmpeg_encode.o ffmpeg_cuda_encode.o rgb_encode.o ffmpeg_audio.o asoundlib_audio.o openwvr.o feedback_net.o input_recv.o

.PHONY: all
all: openwvr libopenwvr.so

openwvr: main.o libopenwvr.a
	$(CC) $(CFLAGS) -o $@ main.o -L/usr/local/cuda/lib64 libopenwvr.a  -lcuda -lglut ~/dev/openwvr_ffmpeg_4/build/lib/libavdevice.a ~/dev/openwvr_ffmpeg_4/build/lib/libavfilter.a ~/dev/openwvr_ffmpeg_4/build/lib/libavformat.a ~/dev/openwvr_ffmpeg_4/build/lib/libavcodec.a ~/dev/openwvr_ffmpeg_4/build/lib/libavutil.a ~/dev/openwvr_ffmpeg_4/build/lib/libswscale.a ~/dev/openwvr_ffmpeg_4/build/lib/libswresample.a -lnppicc -lnppig -lnppc -lass -lSDL2-2.0 -lsndio -lasound -lvdpau -ldl -lva -lva-drm -lXext -lxcb-shm -lxcb-xfixes -lxcb-shape -lxcb -lXv -lfreetype -lpostproc -lva-x11 -lX11 -lpthread -lm -lz

libopenwvr.a: $(OBJS)
	ar -rcs libopenwvr.a $(OBJS)

libopenwvr.so: $(OBJS)
	gcc -shared -o libopenwvr.so $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@rm -f openwvr libopenwvr.a libopenwvr.so main.o $(OBJS)

.PHONY: install
install: libopenwvr.a libopenwvr.so
	cp openwvr.h /usr/local/include/openwvr/
	cp libopenwvr.a /usr/local/lib/
	cp libopenwvr.so /usr/local/lib/