#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//for pollfd and poll()
#include <poll.h>
#include <linux/input.h>
#include <pthread.h>
#include <sys/time.h>

#include "ouvr_packet.h"

#define SERVER_PORT 9000
#define CLIENT_PORT 9001

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

struct SensorData
{
    float gyro_x;
    float gyro_y;
    float gyro_z;
};

int sensor_recv_fd, input_fd;

static struct pollfd sensor_poll;
static struct SensorData sensor_data;
static struct msghdr sensor_msg = {0};
static struct iovec sensor_iov;

static void input_control(int fd, int type, int code, int val)
{
	struct input_event ie;

	ie.type = type;
	ie.code = code;
	ie.value = val;
	ie.time.tv_usec = 0;
	ie.time.tv_usec = 0;
	
	if (write(fd, &ie, sizeof(ie)) < 0)
	{
		PRINT_ERR("Could not write to virtual input event\n");
	}
}

static void setup_file_descriptors()
{
    sensor_recv_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr.s_addr);
    serv_addr.sin_port = htons(SERVER_PORT);

    struct sockaddr_in cli_addr = {0};
    cli_addr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENT_IP, &cli_addr.sin_addr.s_addr);
    cli_addr.sin_port = htons(CLIENT_PORT);

    bind(sensor_recv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    connect(sensor_recv_fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

    //event, message, and poll setup
    sensor_iov.iov_len = sizeof(struct SensorData);
    sensor_iov.iov_base = &sensor_data;
    sensor_msg.msg_iov = &sensor_iov;
    sensor_msg.msg_iovlen = 1;
    sensor_poll.fd = sensor_recv_fd;
    sensor_poll.events = POLLIN;
}

static void create_virtual_input()
{
	input_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (input_fd < 0)
	{
		PRINT_ERR("Failed at opening uinput\n");
	}
	
	// Create Virtual Mouse
	// ioctl(input_fd, UI_SET_EVBIT, EV_KEY);
	// ioctl(input_fd, UI_SET_KEYBIT, BTN_LEFT);	// Left Button
	// ioctl(input_fd, UI_SET_KEYBIT, BTN_RIGHT);	// Right Button	
	ioctl(input_fd, UI_SET_EVBIT, EV_REL);
	ioctl(input_fd, UI_SET_RELBIT, REL_X);		// Horizontal Motion
	ioctl(input_fd, UI_SET_RELBIT, REL_Y);		// Vertical Motion

	// Create Virtual Keyboard
	/*
	ioctl(input_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(input_fd, UI_SET_KEYBIT, KEY_SPACE);	// Up
	ioctl(input_fd, UI_SET_KEYBIT, KEY_LEFTCTRL);	// Down
	ioctl(input_fd, UI_SET_KEYBIT, KEY_W);		// Forward
	ioctl(input_fd, UI_SET_KEYBIT, KEY_A);		// Left
	ioctl(input_fd, UI_SET_KEYBIT, KEY_S);		// Backward
	ioctl(input_fd, UI_SET_KEYBIT, KEY_D);		// Right
	*/

	// Create Virtual Thumbstick
	// ioctl(input_fd, UI_SET_EVBIT, EV_ABS);
	// ioctl(input_fd, UI_SET_ABSBIT, ABS_X);
	// ioctl(input_fd, UI_SET_ABSBIT, ABS_Y);
	// ioctl(input_fd, UI_SET_ABSBIT, ABS_RX);
	// ioctl(input_fd, UI_SET_ABSBIT, ABS_RY);
	
	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "UnrealTournamentHMDController");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor = 0x3;
	uidev.id.product = 0x3;
	uidev.id.version = 2;

	// Thumbstick Configuration
	// uidev.absmax[ABS_X] = 512;
	// uidev.absmin[ABS_X] = -512;
	// uidev.absfuzz[ABS_X] = 0;
	// uidev.absflat[ABS_X] = 15;

	// uidev.absmax[ABS_Y] = 512;
	// uidev.absmin[ABS_Y] = -512;
	// uidev.absfuzz[ABS_Y] = 0;
	// uidev.absflat[ABS_Y] = 15;

	// uidev.absmax[ABS_RX] = 512;
	// uidev.absmin[ABS_RX] = -512;
	// uidev.absfuzz[ABS_RX] = 0;
	// uidev.absflat[ABS_RX] = 16;
	
	// uidev.absmax[ABS_RY] = 512;
	// uidev.absmin[ABS_RY] = -512;
	// uidev.absfuzz[ABS_RY] = 0;
	// uidev.absflat[ABS_RY] = 16;

	if (write(input_fd, &uidev, sizeof(uidev)) < 0)
	{
		PRINT_ERR("Could not write to virtual input\n");
		return;
	}
	
	if (ioctl(input_fd, UI_DEV_CREATE) < 0)
	{
		PRINT_ERR("ui_dev_create\n");
		return;
	}
}

static void *receive_input_loop(void *arg)
{
    arg = (void *)arg;

    setup_file_descriptors();
    create_virtual_input();
    
    sleep(1);   

    do
    {
        recvmsg(sensor_recv_fd, &sensor_msg, 0);

	// Hard-coded factors (because direct manipulation of game control mechanism would make the project less adaptable to other scenarios)
	input_control(input_fd, EV_REL, REL_X, sensor_data.gyro_y*0.075);
	input_control(input_fd, EV_REL, REL_Y, sensor_data.gyro_x*-0.075);

	input_control(input_fd, EV_SYN, SYN_REPORT, 0);
    } while (1);
    return 0;
}

int receive_input_loop_start()
{
    pthread_t recv_input_thread;
    pthread_create(&recv_input_thread, NULL, receive_input_loop, NULL);
    return 0;
}
