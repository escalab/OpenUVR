#include "openuvr.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>
#include <sys/shm.h>

#include <dlfcn.h>

void usage()
{
    printf("Usage: sudo ./openuvr [h264 | rgb] [tcp | udp | udp-compat | raw]\n");
}

int main(int argc, char **argv)
{
    struct openuvr_context *context;
    enum OPENUVR_NETWORK_TYPE net_choice = -1;
    enum OPENUVR_ENCODER_TYPE enc_choice = -1;

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

    // key_t shm_key = 5678;
    // int shm_id = shmget(shm_key, 1920 * 1080 * 4, 0666);
    // uint8_t *src = shmat(shm_id, NULL, 0);
    // if (src == NULL)
    // {
    //     printf("couldn't get shared memory\n");
    //     return -1;
    // }

    uint8_t *src = malloc(1920 * 1080 * 4);
    memset(src, 100, 1920 * 1080 * 4);

    if (!strcmp("h264", argv[1]))
    {
        enc_choice = OPENUVR_ENCODER_H264;
    }
    else if (!strcmp("rgb", argv[1]))
    {
        enc_choice = OPENUVR_ENCODER_RGB;
    }

    if (!strcmp("tcp", argv[2]))
    {
        net_choice = OPENUVR_NETWORK_TCP;
    }
    else if (!strcmp("udp", argv[2]))
    {
        net_choice = OPENUVR_NETWORK_UDP;
    }
    else if (!strcmp("udp-compat", argv[2]))
    {
        net_choice = OPENUVR_NETWORK_UDP_COMPAT;
    }
    else if (!strcmp("raw", argv[2]))
    {
        net_choice = OPENUVR_NETWORK_RAW;
    }

    if (net_choice == -1 || enc_choice == -1)
    {
        usage();
        return 1;
    }

    void *handle = dlopen("libopenuvr.so", RTLD_LAZY);
    if (handle == NULL)
    {
        printf("null\n");
        exit(1);
    }
    struct openuvr_context *(*openuvr_alloc)(enum OPENUVR_ENCODER_TYPE, enum OPENUVR_NETWORK_TYPE, uint8_t *, unsigned int);
    openuvr_alloc = dlsym(handle, "openuvr_alloc_context");
    int (*openuvr_init)(struct openuvr_context *);
    openuvr_init = dlsym(handle, "openuvr_init_thread_continuous");

    context = openuvr_alloc(enc_choice, net_choice, src, 0);
    if (context == NULL)
    {
        usage();
        return 1;
    }

    openuvr_init(context);

    // openuvr_init() spawned a new thread, and this thread doesn't have anything else to do so suspend it to save CPU rather than using an empty infinite loop.
    pause();

    return 0;
}