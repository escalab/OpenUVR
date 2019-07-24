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

struct sensor_data
{
    float accel_x;
    float accel_y;
    float accel_z;

    float gyro_x;
    float gyro_y;
    float gyro_z;

    float mag_x;
    float mag_y;
    float mag_z;
};

int sensor_recv_fd, input_fd;

static struct pollfd sensor_poll;
static struct sensor_data data;
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
    sensor_iov.iov_len = sizeof(struct sensor_data);
    sensor_iov.iov_base = &data;
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
	ioctl(input_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(input_fd, UI_SET_KEYBIT, KEY_SPACE);	// Up
	ioctl(input_fd, UI_SET_KEYBIT, KEY_LEFTCTRL);	// Down
	ioctl(input_fd, UI_SET_KEYBIT, KEY_W);		// Forward
	ioctl(input_fd, UI_SET_KEYBIT, KEY_A);		// Left
	ioctl(input_fd, UI_SET_KEYBIT, KEY_S);		// Backward
	ioctl(input_fd, UI_SET_KEYBIT, KEY_D);		// Right

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

    // time_t start_t, end_t;
    // double diff_t;

    setup_file_descriptors();
    create_virtual_input();
    
    sleep(1);   
 
    // time(&start_t);

    do
    {
        recvmsg(sensor_recv_fd, &sensor_msg, 0);
	// time(&end_t);
	// diff_t = difftime(end_t, start_t);

	// if (diff_t > 0.2)
	// {
	//     PRINT_ERR("%.5f\n", diff_t);
	// }

	// time(&start_t);

	
	

	input_control(input_fd, EV_REL, REL_X, data.gyro_y*0.05);
	input_control(input_fd, EV_REL, REL_Y, data.gyro_x*-0.05);
	// input_control(input_fd, EV_ABS, ABS_X, 15);

	/* Movement Controls
	input_control(input_fd, EV_KEY, KEY_SPACE, 0);
	input_control(input_fd, EV_KEY, KEY_LEFTCTRL, 0);
	input_control(input_fd, EV_KEY, KEY_W, 0);
	input_control(input_fd, EV_KEY, KEY_A, 0);
	input_control(input_fd, EV_KEY, KEY_S, 0);
	input_control(input_fd, EV_KEY, KEY_D, 0);

	if (data.accel_x > 1)
		input_control(input_fd, EV_KEY, KEY_A, 1);
	if (data.accel_x < -1)
		input_control(input_fd, EV_KEY, KEY_D, 1);
	if (data.accel_y > 1)
		input_control(input_fd, EV_KEY, KEY_W, 1);
	if (data.accel_y < -1)
		input_control(input_fd, EV_KEY, KEY_S, 1);
	if (data.accel_z < 8)
		input_control(input_fd, EV_KEY, KEY_SPACE, 1);
	if (data.accel_z > 10)
		input_control(input_fd, EV_KEY, KEY_LEFTCTRL, 1);
	*/	

	// PRINT_ERR("%.3f, %.3f, %.3f\n", data.accel_x, data.accel_y, data.accel_z);

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
