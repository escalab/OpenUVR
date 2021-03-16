# OpenUVR
Open-source Untethered Virtual Reality

## Compiling/Installing on the Host PC (sending side)

### First-Time FFmpeg Installation
FFmpeg is included in OpenUVR as a git submodule. It is set to track version 4.0, and it must be installed before compiling OpenUVR.

1. Clone the `OpenUVR` repository anywhere in your home directory on the host machine.
2. `cd OpenUVR/sending`
3. `git submodule init && git submodule update`. This will download the FFmpeg code from the repository at https://github.com/FFmpeg/FFmpeg.git and will place it into the `OpenUVR/sending/FFmpeg/` directory.
4. `sudo apt-get install ffmpeg`. FFmpeg PC-wide installation
5. `sudo apt-get install libass-dev libfdk-aac-dev libmp3lame-dev libx264-dev libx265-dev libpulse-dev libgles2-mesa-dev libvorbis-dev`. This will install all the required packages in OpenUVR.
6. You will need to apply the `bgr0_ffmpeg.patch` to add support for CUDA encoding in RGBA format, which is supported by NVIDIA but not FFMPEG by default. Enter the `FFmpeg/` directory, run `git apply ../bgr0_ffmpeg.patch` to apply the patch, then leave that directory.
7. CUDA encoding relies on the dependency `ffnvcodec`, which can be installed with these steps:
   1. `git clone https://git.videolan.org/git/ffmpeg/nv-codec-headers.git`
   2. `cd nv-codec-headers`
   3. `make`
   4. `sudo make install`
   5. `export PKG_CONFIG_PATH='path/to/lib/pkgconfig'` This is not required, but will fix the issue if step 8 returns the error `ERROR: cuda requested, but not all dependencies are satisfied: ffnvcodec`
8. From `OpenUVR/sending`, run `make ffmpeg`. This command will enter the `OpenUVR/sending/FFmpeg/` directory, run the `configure` script with specific parameters, then run `make` and `make install`. This will install files to the `OpenUVR/sending/ffmpeg_build/` directory. In the future, you may need to configure FFmpeg differently by changing the configure parameters listed in `OpenUVR/sending/Makefile`.

This will install static and shared libraries to the `OpenUVR/sending/ffmpeg_build/lib` directory. IOQuake3 will need to be compiled with these libraries, and they must be specified when running it. This will be described below in `Compiling IOQuake3`.

### Compiling OpenUVR
From the `OpenUVR/sending/` directory, run `make` to compile the OpenUVR shared libraries and use `sudo make install` to install them. This places the `openuvr.h` header file into `/usr/local/include/openuvr/` and it places `libopenuvr.so` into `/usr/local/lib/`.

`OpenUVR` can be compiled to output the moving average of recent timings. To compile it to output the average encoding time (i.e. the time elapsed from starting to send the RGB bytes to FFmpeg until receiving the encoded frame from FFmpeg), use `make TIME_FLAGS=-DTIME_ENCODING`. To have it output the average network time, use `make TIME_FLAGS=-DTIME_NETWORK`. To do both, do `make TIME_FLAGS="-DTIME_NETWORK -DTIME_ENCODING"`

### Compiling Unreal Tournament
1. The source code for Unreal Tournament is accessed on Github by requesting permission from Epic Games. Instructions are at https://github.com/EpicGames/Signup and setup requires an Epic Games account.
2. Once access is granted to the Epic Games repositories, download the repository at https://github.com/EpicGames/UnrealTournament then checkout the latest commit and follow the instructions given at https://wiki.unrealengine.com/Building_On_Linux to compile it for Linux.
3. To verify that it was compiled correctly, from the base directory of the Unreal Tournament repository run `./Engine/Binaries/Linux/UE4Editor ./UnrealTournament/UnrealTournament.uproject`. If a window pops up warning that some modules are missing or built incorrectly, then click 'Yes' to build those modules. After it compiles those modules, it should start building game objects for a long time, then the editor window should open.
4. Now it's time to modify the Engine code to call OpenUVR, and then recompile the UE4Editor. (Note: I have so far been unable to compile a standalone executable of Unreal Tournament, so UE4Editor will be the executable used whenever launching the game). There are potentially several places to add our OpenUVR code, but I have chosen to modify `Engine/Source/Runtime/OpenGLDrv/Private/Linux/OpenGLLinux.cpp` so as to guarantee that OpenUVR will have access to the OpenGL context.
   * First you need to add a directory with a C# build script to specify directories and external libraries required for OpenUVR. The changes I've made to accomplish this are listed in `OpenUVR/sending/ue4gitdiff`, so you can make those changes yourself or try using `git apply YOUR_PATH_TO_UE4GITDIFF_DIRECTORY/ue4gitdiff`. Then copy the openuvr.h file into `Engine/Source/ThirdParty/OpenUVR/`.
   * In `Engine/Source/Runtime/OpenGLDrv/Private/Linux/OpenGLLinux.cpp` add `#include "openuvr.h"` (within a `extern "C" {}` closure) to access the "managed" OpenUVR functions. The managed OpenUVR functions handle setting up and keeping track of OpenGL framebuffers and it works with Unreal Tournament. However, there might be some games where you'd want to implement the OpenGL management code within the game's source code. For example, the managed functions run in multi-threaded mode, so if you want single-threaded mode then you'll have to call the other OpenUVR functions and handle OpenGL yourself.
   * New code is to be inserted immediately before the `SDL_GL_SwapWindow` function call at the end of the `PlatformBlitToViewport` function (it should be the only instance of `SDL_GL_SwapWindow` in the file). The code should run `openuvr_managed_init(OPENUVR_ENCODER_H264_CUDA, OPENUVR_NETWORK_UDP);` the first time it is called, and it should run `openuvr_managed_copy_framebuffer();` for every frame afterwards. I accomplished this using the following code, although it can probably be improved:
   ```
   // near the top of the file:
   extern "C"{
   #include "openuvr.h"
	void setup_openuvr(){openuvr_managed_init(OPENUVR_ENCODER_H264_CUDA, OPENUVR_NETWORK_UDP);}
	void send_openuvr(){openuvr_managed_copy_framebuffer();}
   }
   ...
   // right before SDL_GL_SwapWindow(): 
   static bool is_inited = false;
	if(!is_inited){
		setup_openuvr();
		is_inited=true;
	}else{
		send_openuvr();
	}
   ```
5. Now to copy the compiled OpenUVR and FFmpeg libraries so they can be used by Unreal Tournament. Create the directory `Engine/Binaries/ThirdParty/OpenUVR/` and create the `libs/` directory within it. Place within `libs/` the compiled shared libraries `libopenuvr.so` and everything in the `ffmpeg_build/lib/` directory. I prefer to accomplish this by using a script which copies the files to the directory every time OpenUVR is built, but you can also accomplish this using symbolic links.
6. Recompile UE4Editor (`make UE4Editor`) then run the game using the command `./Engine/Binaries/Linux/UE4Editor ./UnrealTournament/UnrealTournament.uproject -game`
### Setup Host PC P2P Network
OpenUVR works the best when the host PC and the MUD communicate through P2P network. In 'OpenUVR/sending' directory, we have a run_hostapd.sh and a sample hostapd.conf that you can leverage and setup the host side network.

### Running Unreal Tournament with OpenUVR in SSIM Mode
Measuring the SSIM values of encoded frames requires a great deal of overhead, so conditional compilation of OpenUVR is required. To compile OpenUVR in SSIM Mode, use `make MODE_MEASURE_SSIM=1`. When OpenUVR is compiled in this way, it is hard-coded to run a SSIM-measuring module (`ssim_dummy_net.c`) instead of whichever network module the user selected. When the game is executed, it will do nothing for a set number of frames in order to give the user time to navigate the menus and start a game. Then, it will record 150 successive frames, encode and decode them, then measure and output to the console the average SSIM score for each batch of 10 frames, and then it will output the SSIM average for all 150 frames. It will subsequently allow a set number of frames to pass before recording and analyzing frames again.

The SSIM code relies on Python 3, so after compiling OpenUVR in SSIM mode you must make sure that the python3.5 library is linked to Unreal Tournament. My recommended way of using python is to use virtualenv. To do so, install virtualenv with `pip install virtualenv`. Then run `virtualenv --system-site-packages -p python3 ./ANY_DESTINATION_FOLDER_YOU_CHOOSE`. To enter the virtual environment, run `source ./ANY_DESTINATION_FOLDER_YOU_CHOOSE/bin/activate`, then use `pip install tensorflow tensorflow-gpu`.

Then, you should just be able to run the game as normal. Use `deactivate` when you wish to exit the virtual environment.

### Compiling IOQuake3
IOQuake3 is the first game we modified to run OpenUVR, but it is less preferable to Unreal Tournament.

1. The latest ioq3 is included as a submodule in `OpenUVR/quake/ioq3`. If you haven't yet updated the git submodules as instructed above, use `git submodule init && git submodule update`.
You may also need to install
sudo apt-get install libsdl2-dev  libvdpau-dev libva-dev libxcb-shm0-dev libpostproc-dev libgl1-mesa-dev libsdl2-dev freeglut3 freeglut3-dev

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
2. Enter the `OpenUVR/receiving/src` directory.
3. `sudo apt-get install libavcodec-dev`
4. Compile the standalone `openuvr` program with `make`.

## Running on the Raspberry Pi (receiving side)
Before running the openuvr program, you should assign an IP address to the RPi. https://github.com/escalab/OpenUVR/blob/master/receiving/assign_ip.sh provides a sample regarding how to set up the RPI with a default IP of 192.168.1.3.

Then you can run the program with `sudo ./openuvr <encoding_type> <network_type>` within the `OpenUVR/receiving/src` directory.\
`<encoding type>` can be one of `h264` or `rgb`, but it will likely always be `h264` for your purposes.
`<network_type>` can be one of `raw`, `udp`, `udp_compat`, or `tcp`. Whatever is chosen, it must match the protocol used on the sending side. `tcp` should not be used except for testing purposes. `udp_compat` is used when the sending side is some program other than OpenUVR which sends frames using UDP (for example the ffmpeg executable). `raw` is the optimal choice (measured around 1% faster than UDP, but further optimizations can possibly improve this).

The program must be run with `sudo` only if the `raw` protocol is used. Otherwise, it can be run with or without `sudo`.

When running `openuvr`, you should be able to notice that every so often it gets laggy and drops frames. This is because the display manager on the raspberry pi performs periodic tasks which disrupt `openuvr`. To run it without these interruptions, go into TTY1 using `ctrl+alt+F1`. Log in (probably using the default username `pi` and password `raspberry`), then kill the display manager with `sudo systemctl stop lightdm`. This will kill any windows you had open. To return to desktop mode, use `sudo systemctl start lightdm`.

## Connecting HMD Sensors
OpenUVR utilizes an Adafruit LSM9DS1 triple-axis sensor specifically for the purpose of detecting orientation. Make sure that the sensor is connected to the right GPIO pins on the raspberry pi. If the sensors are not configured correctly or disconnect unexpectedly, the client would try to reconnect every second. Data from the sensor is polled via a Python script, because the library provided by the manufacturer is written in Python. Follow [the official Python guide](https://learn.adafruit.com/adafruit-lsm9ds1-accelerometer-plus-gyro-plus-magnetometer-9-dof-breakout/python-circuitpython) for wiring layout and setting up required Python modules.

The entire orientation system works as such: Inside `input_send.c`, two threads are created, with one running the Python script `lsm9ds1_input.py` and the other running a UNIX socket to receive data from the Python script. The data is then entirely sent via UDP to the host PC. On the host PC, a virtual mouse input is created, and is controlled by the orientation data sent from the OpenUVR client. This input can be adapted to be a virtual joystick to avoid conflict with user mouse input, although the purpose was to allow the user to control the application interface, not just the in-game orientation, entirely through the HMD. 

## Reference Paper

Alec Rohloff, Zackary Allen, Kung-Min Lin, Joshua Okrend, Chengyi Nie, Yu-Chia Liu, and Hung-Wei Tseng. OpenUVR: an Open-Source System Framework for Untethered Virtual Reality Applications. In 27th IEEE Real-Time and Embedded Technology and Applications Symposium, RTAS 2021, 2021. 

## Deploying with Docker

Please request the RSA key from Kung-Min Lin (km.lin@berkeley.edu), which provides access to the private UnrealTournament repository that is required to test this software. You may also generate your own RSA key pairs as long as you have access to the UnrealEngine organization.
