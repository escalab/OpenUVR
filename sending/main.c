#include "openwvr.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>
#include <sys/shm.h>

void usage()
{
    printf("Usage: sudo ./openwvr [h264 | rgb] [tcp | udp | udp-compat | raw]\n");
}

int main(int argc, char **argv)
{
    struct openwvr_context *context;
    enum OPENWVR_NETWORK_TYPE net_choice = -1;
    enum OPENWVR_ENCODER_TYPE enc_choice = -1;

    __uid_t uid = getuid();
    if (uid != 0)
    {
        printf("Error: program must be run as root.\n");
        usage();
        return 1;
    }

    if (argc > 3)
    {
        usage();
        return 1;
    }

    key_t shm_key = 5678;
    int shm_id = shmget(shm_key, 1920 * 1080 * 4, 0666);
    unsigned char *src = shmat(shm_id, NULL, 0);
    if (src == NULL)
    {
        printf("couldn't get shared memory\n");
        return -1;
    }

    if (!strcmp("h264", argv[1]))
    {
        enc_choice = OPENWVR_ENCODER_H264;
    }
    else if (!strcmp("rgb", argv[1]))
    {
        enc_choice = OPENWVR_ENCODER_RGB;
    }

    if (!strcmp("tcp", argv[2]))
    {
        net_choice = OPENWVR_NETWORK_TCP;
    }
    else if (!strcmp("udp", argv[2]))
    {
        net_choice = OPENWVR_NETWORK_UDP;
    }
    else if (!strcmp("udp-compat", argv[2]))
    {
        net_choice = OPENWVR_NETWORK_UDP_COMPAT;
    }
    else if (!strcmp("raw", argv[2]))
    {
        net_choice = OPENWVR_NETWORK_RAW;
    }

    if (net_choice == -1 || enc_choice == -1)
    {
        usage();
        return 1;
    }

    context = openwvr_alloc_context(enc_choice, net_choice, src);
    if (context == NULL)
    {
        usage();
        return 1;
    }

    openwvr_init_thread_continuous(context);

    for (;;)
    {
        //do nothing
    }
    return 0;
}