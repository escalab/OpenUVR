#!/bin/bash
cd "$(dirname "$0")"
sudo LD_LIBRARY_PATH=../sending/src/:../sending/ffmpeg_build/lib/ ./thesea.x86_64