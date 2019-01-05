# OpenUVR
Open-source Untethered Virtual Reality

## Compiling/Installing on the Host PC (sending side)

### First-Time FFmpeg Installation
FFmpeg is included in OpenUVR as a git submodule. It is set to track version 4.0, and it must be installed before compiling OpenUVR.

1. Clone the `OpenUVR` repository anywhere in your home directory on the host machine.
2. `cd OpenUVR/sending`
3. `git submodule init && git submodule update`. This will download the FFmpeg code from the repository at https://github.com/FFmpeg/FFmpeg.git and will place it into the `OpenUVR/sending/FFmpeg/` directory.
3. `make ffmpeg`. This command will enter the `OpenUVR/sending/FFmpeg/` directory, run the `configure` script with specific parameters, then run `make` and `make install`. This will install files to the `OpenUVR/sending/ffmpeg_build/` directory. In the future, you may need to configure FFmpeg differently by changing the configure parameters listed in `OpenUVR/sending/Makefile`.

This will install static and shared libraries to the `OpenUVR/sending/ffmpeg_build/lib` directory. IOQuake3 will need to be compiled with these libraries, and they must be specified when running it. This will be described below in `Compiling IOQuake3`.

### Compiling OpenUVR
From the `OpenUVR/sending/` directory, run `make` to compile the OpenUVR shared libraries and use `sudo make install` to install them. This places the `openuvr.h` header file into `/usr/local/include/openuvr/` and it places `libopenuvr.so` into `/usr/local/lib/`.

### Compiling IOQuake3
IOQuake3 is the game we're using to to run OpenUVR. It is open source and runs on Linux, so we need to download the source code, modify it to use OpenUVR, then compile and run it.

1. Clone the IOQuake3 repository somewhere into your home directory: `https://github.com/ioquake/ioq3.git`.
2. Edit `ioq3/Makefile`. Under the `SETUP AND BUILD -- LINUX` section, replace the `LIBS=...` line (should be around line 392) with the following 3 lines:
```
OPENUVR_FFMPEG_LIB_DIR=<replace this with the absolute path to your ffmpeg_build/lib directory>
LIBS=-L/usr/local/cuda/lib64 -lopenuvr -lcuda -lglut $(OPENUVR_FFMPEG_LIB_DIR)/libavdevice.so $(OPENUVR_FFMPEG_LIB_DIR)/libavfilter.so $(OPENUVR_FFMPEG_LIB_DIR)/libavformat.so $(OPENUVR_FFMPEG_LIB_DIR)/libavcodec.so $(OPENUVR_FFMPEG_LIB_DIR)/libavutil.so $(OPENUVR_FFMPEG_LIB_DIR)/libswscale.so $(OPENUVR_FFMPEG_LIB_DIR)/libswresample.so -lnppicc -lnppig -lnppc -lass -lSDL2-2.0 -lsndio -lasound -lvdpau -ldl -lva -lva-drm -lXext -lxcb-shm -lxcb-xfixes -lxcb-shape -lxcb -lXv -lfreetype -lpostproc -lva-x11 -lX11 -lpthread -lm -lz
CLIENT_CFLAGS+=-I/usr/local/cuda/include
```
3. The `ioq3/code/sdl/sdl_glimp.c` file is the entry point that we will use to call into our OpenUVR library.
   1. First, include our header by inserting `#include <openuvr/openuvr.h>` near the top. Below that, declare the global struct which will hold the OpenUVR context with `struct openuvr_context *ouvr_ctx;`
   2. Declare four functions that take no parameters and return void, which we'll call `setup_openuvr_nocuda()`, `setup_openuvr_cuda()`, `send_openuvr_nocuda()`, and `send_openuvr_cuda()`. `setup_openuvr_nocuda()` will set up the `ouvr_ctx` in a mode which does not use CUDA, meaning that it is passed the pointer to a region in memory containing the RGB values of each frame, and `send_openuvr_nocuda()` will send that region. `setup_openuvr_cuda()` will set up the `ouvr_ctx` in a mode using CUDA, which means that it is passed a pointer to a `PixelBufferObject`, which allows it to take the RGB values directly from GPU memory.
      - `setup_openuvr_nocuda` should contain code such as:
      ```
      unsigned char *buf = malloc(4 * glConfig.vidWidth * glConfig.vidHeight);
      ouvr_ctx = openuvr_alloc_context(<encoder_type>, <network_type>, buf);
      ```
   2. Towards the end of the `GLimp_SetMode()` function, insert the code to setup the OpenUVR context:
4. TODO run the make script. This will compile ioq3 and install it to your home directory in the `~/bin/ioquake3` directory.
