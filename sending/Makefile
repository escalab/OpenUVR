.PHONY: all
all:
	@$(MAKE) -C src/ all

.PHONY: install
install:
	@$(MAKE) -C src/ install

ffmpeg:
	cd FFmpeg/ &&\
	./configure --prefix=$(shell pwd)/ffmpeg_build/ --enable-static --enable-pic --enable-shared --pkg-config-flags=--static --extra-cflags='--static -fPIC -I/usr/local/cuda/include' --extra-ldflags='-L/usr/local/cuda/lib64' --extra-libs='-lpthread -lm' --bindir=$(shell pwd)/ffmpeg_build/bin --enable-gpl --enable-libass --enable-libfdk-aac --enable-libfreetype --enable-libmp3lame --enable-cuda --enable-cuvid --enable-libnpp --enable-libopus --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libx264 --enable-libx265 --enable-nonfree --enable-nvenc --disable-muxers --enable-muxer=h264 --enable-muxer=mpegts --enable-muxer=tee --enable-muxer=null --disable-demuxers --disable-encoders --enable-encoder=h264_nvenc --enable-encoder=nvenc_h264 --enable-encoder=nvenc --disable-decoders --enable-decoder=rawvideo --enable-decoder=h264_cuvid --enable-muxer=rtsp --enable-protocol=sctp --disable-doc &&\
	make && make install

.PHONY: clean
clean:
	@$(MAKE) -C src/ clean
