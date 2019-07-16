CFLAGS=-std=c11 -fPIC -Wall -Wextra -D_GNU_SOURCE=1 -O3 -I$(shell pwd)/../ffmpeg_build -I$(shell pwd)/../ffmpeg_build/include -I/usr/local/cuda/include -I/usr/include/python3.5m $(TIME_FLAGS)

OBJS=ouvr_packet.o tcp.o udp.o udp_compat.o raw.o raw_ring.o inject.o rtt_measure_net.o ffmpeg_encode.o ffmpeg_cuda_encode.o rgb_encode.o openuvr.o openuvr_managed.o feedback_net.o input_recv.o

# required for pulse audio, but doesn't work with unity. TODO find a nice way to fix this so that we can uncomment it
#OBJS+= pulse_audio.o

.PHONY: all
all: libopenuvr.so libopenuvr.a


ifdef MODE_MEASURE_SSIM
OBJS+= ssim_dummy_net.o ssim_plugin.o
CFLAGS+= -DMEASURE_SSIM
	
# auto-generated c source code to run python
ssim_plugin.c: ssim_plugin_builder.py ssim_plugin_src.py ssim_plugin.h
	python3 ssim_plugin_builder.py
endif


libopenuvr.so: $(OBJS)
	gcc -shared -L../ffmpeg_build/lib/ -Wl,--no-as-needed -lavcodec -lavfilter -lavformat -lavutil -lswresample -lswscale -lavdevice -o libopenuvr.so $(OBJS)
	chmod -x libopenuvr.so

FFMPEG_LIB_DIR=../ffmpeg_build/lib

libopenuvr.a: $(OBJS)
	ar rcs libopenuvr.a $(OBJS) $(FFMPEG_LIB_DIR)/libavdevice.a $(FFMPEG_LIB_DIR)/libavfilter.a $(FFMPEG_LIB_DIR)/libavformat.a $(FFMPEG_LIB_DIR)/libavcodec.a $(FFMPEG_LIB_DIR)/libavutil.a $(FFMPEG_LIB_DIR)/libswscale.a $(FFMPEG_LIB_DIR)/libswresample.a

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

# Optional program to send static content:
openuvr: main.o libopenuvr.so
	$(CC) $(CFLAGS) -o $@ main.o -lcuda -lglut -lass -lSDL2-2.0 -lsndio -lasound -lvdpau -ldl -lva -lva-drm -lXext -lxcb-shm -lxcb-xfixes -lxcb-shape -lxcb -lXv -lfreetype -lpostproc -lva-x11 -lX11 -lpthread -lm -lz

.PHONY: install
install: libopenuvr.so
	rsync openuvr.h /usr/local/include/openuvr/
	rsync libopenuvr.so /usr/local/lib/
	rsync libopenuvr.a /usr/local/lib/

.PHONY: clean
clean:
	@rm -f openuvr libopenuvr.a libopenuvr.so main.o ssim_plugin.c ssim_plugin.o ssim_dummy_net.o $(OBJS)
