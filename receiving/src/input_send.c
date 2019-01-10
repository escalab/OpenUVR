#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
//for pollfd and polling in general
#include <poll.h>
//for input_event for mouse
#include <linux/input.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "ouvr_packet.h"

#define SERVER_PORT_MOUSE 9000
#define CLIENT_PORT_MOUSE 9001

int mouse_fd;
int mouse_send_fd;
int keyboard_fd;

static struct pollfd mouse_poll[2];
static struct input_event event[32];
static struct msghdr mouse_msg = {0};
static struct iovec mouse_iov;

static void setup_mouse_file_descriptors()
{

    mouse_fd = open("/dev/input/event2", O_RDWR);
    if (mouse_fd < 0)
    {
        printf("Couldn't open event1, probably not sudo\n");
        exit(1);
    }

    keyboard_fd = open("/dev/input/event0", O_RDWR);
    if (keyboard_fd < 0)
    {
        printf("Couldn't open event0, probably not sudo\n");
        exit(1);
    }

    mouse_send_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (mouse_send_fd < 0)
    {
        printf("Couldn't open mouse send socket\n");
        exit(1);
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr.s_addr);
    serv_addr.sin_port = htons(SERVER_PORT_MOUSE);

    struct sockaddr_in cli_addr = {0};
    cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &cli_addr.sin_addr.s_addr);
    cli_addr.sin_port = htons(CLIENT_PORT_MOUSE);

    bind(mouse_send_fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
    connect(mouse_send_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    //event, message, and poll setup
    mouse_iov.iov_len = sizeof(struct input_event) * 32;
    mouse_iov.iov_base = event;
    mouse_msg.msg_iov = &mouse_iov;
    mouse_msg.msg_iovlen = 1;

    mouse_poll[0].fd = mouse_fd;
    mouse_poll[0].events = POLLIN;
    mouse_poll[1].fd = keyboard_fd;
    mouse_poll[1].events = POLLIN;
}

static void *send_input_loop(void *arg)
{
    setup_mouse_file_descriptors();
    do
    {
        if (poll(mouse_poll, 2, 1000) > 0)
        {
            if (mouse_poll[0].revents & POLLIN)
                event[15].code = read(mouse_fd, event, sizeof(struct input_event) * 15) / sizeof(struct input_event);
            if (mouse_poll[1].revents & POLLIN)
                event[31].code = read(keyboard_fd, event + 16, sizeof(struct input_event) * 15) / sizeof(struct input_event);
	        sendmsg(mouse_send_fd, &mouse_msg, 0);
        }
    } while (1);
    return 0;
}


int send_input_loop_start(){
    pthread_t send_input_thread;
    pthread_create(&send_input_thread, NULL, send_input_loop, NULL);
    return 0;
}
