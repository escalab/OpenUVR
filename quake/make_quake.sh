#!/bin/bash
export COPYDIR=/home/$(whoami)/bin/ioquake3
cd "$(dirname "$0")/ioq3"
touch code/sdl/sdl_glimp.c && make copyfiles
