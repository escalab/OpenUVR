#!/bin/bash
cd "$(dirname "$0")"
sudo LD_LIBRARY_PATH=$(pwd)/../sending/ffmpeg_build/lib ~/bin/ioquake3/ioquake3.x86_64
