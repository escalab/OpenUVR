#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//for pollfd and poll()
#include <poll.h>
//for mouse input_event
#include <linux/input.h>
#include <pthread.h>

#include "ouvr_packet.h"

#define SERVER_PORT_MOUSE 9000
#define CLIENT_PORT_MOUSE 9001

struct my_event
{
    __u32 sec;
    __u32 usec;
    __u16 type;
    __u16 code;
    __s32 value;
};

int mouse_fd;
int keyboard_fd;
int mouse_recv_fd;

static struct pollfd mouse_poll;
static struct my_event event[32];
static struct msghdr mouse_msg = {0};
static struct iovec mouse_iov;

static void setup_mouse_file_descriptors()
{
    mouse_fd = open("/dev/input/event16", O_WRONLY);
    if (mouse_fd < 0)
    {
        printf("Couldn't open event3\n");
    }

    keyboard_fd = open("/dev/input/event5", O_WRONLY);
    if (keyboard_fd < 0)
    {
        printf("Couldn't open event4\n");
    }

    mouse_recv_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr.s_addr);
    serv_addr.sin_port = htons(SERVER_PORT_MOUSE);

    struct sockaddr_in cli_addr = {0};
    cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &cli_addr.sin_addr.s_addr);
    cli_addr.sin_port = htons(CLIENT_PORT_MOUSE);

    bind(mouse_recv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    connect(mouse_recv_fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

    //event, message, and poll setup
    mouse_iov.iov_len = sizeof(struct input_event) * 32;
    mouse_iov.iov_base = &event;
    mouse_msg.msg_iov = &mouse_iov;
    mouse_msg.msg_iovlen = 1;
    mouse_poll.fd = mouse_recv_fd;
    mouse_poll.events = POLLIN;
}

static void *receive_input_loop(void *arg)
{
            // printf('asdf\n');
    setup_mouse_file_descriptors();
    struct input_event event_temp[32];
    do
    {
        if (poll(&mouse_poll, 1, 1000) == 1)
        {
            // printf('fdsa\n');
            recvmsg(mouse_recv_fd, &mouse_msg, 0);
            for (int i = 0; i < event[15].code; i++)
            {
                event_temp[i].code = event[i].code;
                event_temp[i].time.tv_sec = event[i].sec;
                event_temp[i].time.tv_usec = event[i].usec;
                event_temp[i].type = event[i].type;
                event_temp[i].value = event[i].value;
            }
            write(mouse_fd, event_temp, event[15].code * sizeof(struct input_event));
            for (int i = 16; i < 16 + event[31].code; i++)
            {
                event_temp[i].code = event[i].code;
                event_temp[i].time.tv_sec = event[i].sec;
                event_temp[i].time.tv_usec = event[i].usec;
                event_temp[i].type = event[i].type;
                event_temp[i].value = event[i].value;
            }
            write(keyboard_fd, event_temp + 16, event[31].code * sizeof(struct input_event));
        }
    } while (1);
    return 0;
}

int receive_input_loop_start()
{
    pthread_t recv_input_thread;
    pthread_create(&recv_input_thread, NULL, receive_input_loop, NULL);
    return 0;
}