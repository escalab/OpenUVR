#include "openuvr.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

void usage()
{
    printf("Usage: sudo ./openuvr [h264 | rgb] [udp | udp-compat | raw]\n");
}

int main(int argc, char **argv) {
    struct openuvr_context *context;
    enum OPENUVR_NETWORK_TYPE net_choice = -1;
    enum OPENUVR_DECODER_TYPE enc_choice = -1;

    if(argc > 3) {
        usage();
        return 1;
    }

    if(!strcmp("h264", argv[1])) {
        enc_choice = OPENUVR_DECODER_H264;
    }
    else if(!strcmp("rgb", argv[1])) {
        enc_choice = OPENUVR_DECODER_RGB;
    }

    if(!strcmp("tcp", argv[2])) {
        net_choice = OPENUVR_NETWORK_TCP;
    }
    else if(!strcmp("udp", argv[2])) {
        net_choice = OPENUVR_NETWORK_UDP;
    }
    else if(!strcmp("udp-compat", argv[2])) {
        net_choice = OPENUVR_NETWORK_UDP_COMPAT;
    }
    else if(!strcmp("raw", argv[2])) {
        net_choice = OPENUVR_NETWORK_RAW;
    }

    if(net_choice == -1 || enc_choice == -1) {
        usage();
        return 1;
    }

    context = openuvr_alloc_context(enc_choice, net_choice);

    if(context == NULL) {
        usage();
        return 1;
    }

    int frames_recvd = 0;
    int curr_sec = 0;
    struct timeval tv;
    
    while(1){
        if(openuvr_receive_frame(context) != 0)
        {
            return 1;
        }
        gettimeofday(&tv, NULL);
        if(tv.tv_sec != curr_sec)
        {
           // printf("%d fps\n", frames_recvd);
            curr_sec = tv.tv_sec;
            frames_recvd = 0;
        }
        frames_recvd++;
    }
    return 0;
}
