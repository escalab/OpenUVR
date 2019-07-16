all: libSimple.so

libSimple.so: simple.c
	gcc -shared -o libSimple.so -fPIC -I../../sending/src/ simple.c -static -L../../sending/src -L../../sending/ffmpeg_build/lib -lopenuvr -lavcodec -lavformat -lavdevice -lswscale -lavutil -lcuda
	cp libSimple.so ../thesea_Data/Plugins/x86_64/