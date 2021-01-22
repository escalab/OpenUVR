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

.PHONY: all
all:
	@$(MAKE) -C src/ all

.PHONY: install
install:
	@$(MAKE) -C src/ install

ffmpeg:
	cd FFmpeg/ &&\
	./configure --prefix=$(shell pwd)/ffmpeg_build/ --enable-static --enable-pic --enable-shared --pkg-config-flags=--static --extra-cflags='--static -fPIC -I/usr/local/cuda/include' --extra-ldflags='-L/usr/local/cuda/lib64' --extra-libs='-lpthread -lm' --bindir=$(shell pwd)/ffmpeg_build/bin --enable-gpl --enable-libass --enable-libfdk-aac --enable-libfreetype --enable-libmp3lame --enable-cuda --enable-cuvid --enable-libnpp --enable-libopus --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libx264 --enable-nonfree --enable-nvenc --disable-muxers --enable-muxer=h264 --enable-muxer=mpegts --enable-muxer=tee --enable-muxer=null --disable-demuxers --disable-encoders --enable-encoder=h264_nvenc --enable-encoder=nvenc_h264 --enable-encoder=nvenc --disable-decoders --enable-decoder=rawvideo --enable-decoder=h264_cuvid --enable-decoder=h264_crystalhd --enable-decoder=h264_mediacodec --enable-decoder=h264_mmal --enable-decoder=h264_qsv --enable-muxer=rtsp --enable-protocol=sctp --disable-doc &&\
	make && make install

.PHONY: clean
clean:
	@$(MAKE) -C src/ clean

