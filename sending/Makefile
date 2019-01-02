CFLAGS=-std=c11 -fPIC -Wall -Wextra -D_GNU_SOURCE=1 -O3 -I/home/arrohlof/dev/openwvr_ffmpeg_4/build -I/home/arrohlof/dev/openwvr_ffmpeg_4/build/include -I/usr/local/cuda/include $(TIME_FLAGS)

OBJS=ouvr_packet.o tcp.o udp.o udp_compat.o raw.o ffmpeg_encode.o ffmpeg_cuda_encode.o rgb_encode.o ffmpeg_audio.o asoundlib_audio.o openuvr.o feedback_net.o input_recv.o

.PHONY: all
all: libopenuvr.so

libopenuvr.so: $(OBJS)
	gcc -shared -o libopenuvr.so $(OBJS)
	chmod -x libopenuvr.so

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

# Optional program to send content from shared memory buffer:
openuvr: main.o libopenuvr.so
	$(CC) $(CFLAGS) -o $@ main.o -L/usr/local/cuda/lib64 libopenuvr.so  -lcuda -lglut ~/dev/openwvr_ffmpeg_4/build/lib/libavdevice.a ~/dev/openwvr_ffmpeg_4/build/lib/libavfilter.a ~/dev/openwvr_ffmpeg_4/build/lib/libavformat.a ~/dev/openwvr_ffmpeg_4/build/lib/libavcodec.a ~/dev/openwvr_ffmpeg_4/build/lib/libavutil.a ~/dev/openwvr_ffmpeg_4/build/lib/libswscale.a ~/dev/openwvr_ffmpeg_4/build/lib/libswresample.a -lnppicc -lnppig -lnppc -lass -lSDL2-2.0 -lsndio -lasound -lvdpau -ldl -lva -lva-drm -lXext -lxcb-shm -lxcb-xfixes -lxcb-shape -lxcb -lXv -lfreetype -lpostproc -lva-x11 -lX11 -lpthread -lm -lz

ffmpeg:
	cd FFmpeg/ &&\
	./configure --prefix=$(shell pwd)/ffmpeg_build/ --enable-static --enable-pic --pkg-config-flags=--static --extra-cflags='--static -fPIC -I/usr/local/cuda/include' --extra-ldflags='-L/usr/local/cuda/lib64' --extra-libs='-lpthread -lm' --bindir=$(shell pwd)/ffmpeg_build/bin --enable-gpl --enable-libass --enable-libfdk-aac --enable-libfreetype --enable-libmp3lame --enable-cuda --enable-cuvid --enable-libnpp --enable-libopus --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libx264 --enable-libx265 --enable-nonfree --enable-nvenc --disable-muxers --enable-muxer=h264 --enable-muxer=mpegts --enable-muxer=tee --enable-muxer=null --disable-demuxers --disable-encoders --enable-encoder=h264_nvenc --enable-encoder=nvenc_h264 --enable-encoder=nvenc --disable-decoders --enable-decoder=rawvideo --enable-muxer=rtsp --enable-protocol=sctp --disable-doc &&\
	make && make install


.PHONY: install
install: libopenuvr.so
	rsync openuvr.h /usr/local/include/openuvr/
	rsync libopenuvr.so /usr/local/lib/

.PHONY: clean
clean:
	@rm -f openuvr libopenuvr.a libopenuvr.so main.o $(OBJS)