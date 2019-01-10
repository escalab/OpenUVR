# OpenUVR
Open-source Untethered Virtual Reality

## Compiling/Installing on the Host PC (sending side)

### First-Time FFmpeg Installation
FFmpeg is included in OpenUVR as a git submodule. It is set to track version 4.0, and it must be installed before compiling OpenUVR.

1. Clone the `OpenUVR` repository anywhere in your home directory on the host machine.
2. `cd OpenUVR/sending`
3. `git submodule init && git submodule update`. This will download the FFmpeg code from the repository at https://github.com/FFmpeg/FFmpeg.git and will place it into the `OpenUVR/sending/FFmpeg/` directory.
3. You'll need to apply the `bgr0_ffmpeg.patch` to add support for CUDA encoding in RGBA format, which is supported by NVIDIA but not FFMPEG by default. Enter the `FFmpeg/` directory, run `git apply ../bgr0_ffmpeg.patch` to apply the patch, then leave that directory.
3. From `OpenUVR/sending`, run `make ffmpeg`. This command will enter the `OpenUVR/sending/FFmpeg/` directory, run the `configure` script with specific parameters, then run `make` and `make install`. This will install files to the `OpenUVR/sending/ffmpeg_build/` directory. In the future, you may need to configure FFmpeg differently by changing the configure parameters listed in `OpenUVR/sending/Makefile`.

This will install static and shared libraries to the `OpenUVR/sending/ffmpeg_build/lib` directory. IOQuake3 will need to be compiled with these libraries, and they must be specified when running it. This will be described below in `Compiling IOQuake3`.

### Compiling OpenUVR
From the `OpenUVR/sending/` directory, run `make` to compile the OpenUVR shared libraries and use `sudo make install` to install them. This places the `openuvr.h` header file into `/usr/local/include/openuvr/` and it places `libopenuvr.so` into `/usr/local/lib/`.

`OpenUVR` can be compiled to output the moving average of recent timings. To compile it to output the average encoding time (i.e. the time elapsed from starting to send the RGB bytes to FFmpeg until receiving the encoded frame from FFmpeg), use `make TIME_FLAGS=-DTIME_ENCODING`. To have it output the average network time, use `make TIME_FLAGS=-DTIME_NETWORK`. To do both, do `make TIME_FLAGS="-DTIME_NETWORK -DTIME_ENCODING"`

### Compiling IOQuake3
IOQuake3 is the game we're using to to run OpenUVR. It is open source and runs on Linux, so we need to download the source code, modify it to use OpenUVR, then compile and run it.

1. The latest ioq3 is included as a submodule in `OpenUVR/quake/ioq3`. If you haven't yet updated the git submodules as instructed above, use `git submodule init && git submodule update`.
2. Edit `OpenUVR/quake/ioq3/Makefile`. Under the `SETUP AND BUILD -- LINUX` section, replace the `RENDERER_LIBS=...` line (should be around line 396) with `RENDERER_LIBS = $(SDL_LIBS) -lGL`. Also replace the `LIBS=...` line just above that with the following 3 lines:
```
OPENUVR_FFMPEG_LIB_DIR=/your/path/to/OpenUVR/sending/ffmpeg_build/lib
LIBS=-L/usr/local/cuda/lib64 -lopenuvr -lcuda -lglut $(OPENUVR_FFMPEG_LIB_DIR)/libavdevice.so $(OPENUVR_FFMPEG_LIB_DIR)/libavfilter.so $(OPENUVR_FFMPEG_LIB_DIR)/libavformat.so $(OPENUVR_FFMPEG_LIB_DIR)/libavcodec.so $(OPENUVR_FFMPEG_LIB_DIR)/libavutil.so $(OPENUVR_FFMPEG_LIB_DIR)/libswscale.so $(OPENUVR_FFMPEG_LIB_DIR)/libswresample.so -lnppicc -lnppig -lnppc -lass -lSDL2-2.0 -lsndio -lasound -lvdpau -ldl -lva -lva-drm -lXext -lxcb-shm -lxcb-xfixes -lxcb-shape -lxcb -lXv -lfreetype -lpostproc -lva-x11 -lX11 -lpthread -lm -lz
CLIENT_CFLAGS+=-I/usr/local/cuda/include
```
3. The `ioq3/code/sdl/sdl_glimp.c` file is the entry point that we will use to call into our OpenUVR library.
   1. First, include our header by inserting `#include <openuvr/openuvr.h>` near the top. Below that, declare the global struct which will hold the OpenUVR context with `struct openuvr_context *ouvr_ctx;`. Also include the OpenGL header that enables cuda functionality with `#include <GLES3/gl3.h>`.
   2. Declare four functions that take no parameters and return void, which we'll call `setup_openuvr_nocuda()`, `send_openuvr_nocuda()`, `setup_openuvr_cuda()`, and `send_openuvr_cuda()`. `setup_openuvr_nocuda()` will set up the `ouvr_ctx` in a mode which does not use CUDA, meaning that it is passed the pointer to a region in memory containing the RGB values of each frame, and `send_openuvr_nocuda()` will send that region. `setup_openuvr_cuda()` will set up the `ouvr_ctx` in a mode using CUDA, which means that it is passed a pointer to a `PixelBufferObject`, which allows it to take the RGB values directly from GPU memory.
      - `setup_openuvr_nocuda()` should contain code such as the following, after declaring `buf` as a global `unsigned char *`:
      ```
      buf = malloc(4 * glConfig.vidWidth * glConfig.vidHeight);
      ouvr_ctx = openuvr_alloc_context(OPENUVR_ENCODER_H264, OPENUVR_NETWORK_RAW, buf);
      openuvr_init_thread_continuous(ouvr_ctx);
      ```
      - `send_openuvr_nocuda()` should contain code such as:
      ```
      glReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
      ```
      - `setup_openuvr_cuda()` should contain code such as:
      ```
      GLuint pbo;
      glGenBuffers(1, &pbo);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
      glBufferData(GL_PIXEL_PACK_BUFFER, glConfig.vidWidth*glConfig.vidHeight*4, 0, GL_DYNAMIC_COPY);
      glReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      ouvr_ctx = openuvr_alloc_context(OPENUVR_ENCODER_H264_CUDA, OPENUVR_NETWORK_RAW, &pbo);
      openuvr_cuda_copy(ouvr_ctx, NULL);
      openuvr_init_thread_continuous(ouvr_ctx);
      ```
      - `send_openuvr_cuda()` should contain code such as:
      ```
      glReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      // technically should be called, but seems to be optional, and should be investigated:
      openuvr_cuda_copy(ouvr_ctx, NULL);
      ```
   3. At the end of the `GLimp_SetMode()` function, insert the call to either `setup_openuvr_nocuda()` or `setup_openuvr_cuda()`.
   4. At the beginning of the `GLimp_EndFrame()` function, insert the call to either `send_openuvr_nocuda()` or `send_openuvr_cuda()` immediately before the call to `SDL_GL_SwapWindow()`. It should go before the call to `SDL_GL_SwapWindow()` since `qglReadPixels()` reads from the back buffer, so this way it will start processing frames before they are displayed to the player on the host machine.
4. TODO run the make script. This will compile ioq3 and install it to your home directory in the `~/bin/ioquake3` directory.

Now that Quake 3 is installed, you can run it using `sudo LD_LIBRARY_PATH=/your/path/to/OpenUVR/sending/ffmpeg_build/lib ~/bin/ioquake3/ioquake3.x86_64`.

## Compiling on the Raspberry Pi (receiving side)
1. Clone the `OpenUVR` repository anywhere in your home directory on the Raspberry Pi.
2. Enter the `OpenUVR/receiving` directory.
3. Compile the standalone `openuvr` program with `make`.

## Running on the Raspberry Pi (receiving side)
Run the program with `sudo ./openuvr <encoding_type> <network_type>`.\
`<encoding type>` can be one of `h264` or `rgb`, but it will likely always be `h264` for your purposes.
`<network_type>` can be one of `raw`, `udp`, `udp_compat`, or `tcp`. Whatever is chosen, it must match the protocol used on the sending side. `tcp` should not be used except for testing purposes. `udp_compat` is used when the sending side is some program other than OpenUVR which sends frames using UDP (for example the ffmpeg executable). `raw` is the optimal choice (measured around 1% faster than UDP, but further optimizations can possibly improve this).

The program must be run with `sudo` only if the `raw` protocol is used. Otherwise, it can be run with or without `sudo`.

When running `openuvr`, you should be able to notice that every so often it gets laggy and drops frames. This is because the display manager on the raspberry pi performs periodic tasks which disrupt `openuvr`. To run it without these interruptions, go into TTY1 using `ctrl+alt+F1`. Log in (probably using the default username `pi` and password `raspberry`), then kill the display manager with `sudo systemctl stop lightdm`. This will kill any windows you had open. To return to desktop mode, use `sudo systemctl start lightdm`.
