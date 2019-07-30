#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

// Socket
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include "ouvr_packet.h"

#define SENSOR_ADDR "/tmp/openuvr_hmd_input_socket"
#define SERVER_PORT 9000
#define CLIENT_PORT 9001

int sensor_fd;
int sensor_send_fd;

struct SensorData
{
	float gyro_x;
	float gyro_y;
	float gyro_z;
};

static struct SensorData sensor;
static struct pollfd sensor_poll;
static struct msghdr sensor_msg = {0};
static struct iovec sensor_iov;

static struct msghdr input_msg = {0};
static struct iovec input_iov;

static void setup_sensor_file_descriptors()
{
	struct sockaddr_un sensor_addr;
	sensor_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	sensor_addr.sun_family = AF_UNIX;
	strcpy(sensor_addr.sun_path, SENSOR_ADDR);

	if ((sensor_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		printf("Couldn't create sensor socket\n");
		exit(1);
	}

	connect(sensor_fd, (struct sockaddr *) &sensor_addr, sizeof(sensor_addr));

	sensor_iov.iov_len = sizeof(struct SensorData);
	sensor_iov.iov_base = &sensor;
	sensor_msg.msg_iov = &sensor_iov;
	sensor_msg.msg_iovlen = 1;
	sensor_poll.fd = sensor_fd;
	sensor_poll.events = POLLIN;

	sensor_send_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sensor_send_fd < 0)
	{
		printf("Couldn't open sensor send socket\n");
		exit(1);
	}

	struct sockaddr_in serv_addr = {0};
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr.s_addr);
	serv_addr.sin_port = htons(SERVER_PORT);

	struct sockaddr_in cli_addr = {0};
	cli_addr.sin_family = AF_INET;
	inet_pton(AF_INET, CLIENT_IP, &cli_addr.sin_addr.s_addr);
	cli_addr.sin_port = htons(CLIENT_PORT);

	bind(sensor_send_fd, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
	connect(sensor_send_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
}

static void *send_input_loop(void *arg)
{
	setup_sensor_file_descriptors();

	do
	{
		if (poll(&sensor_poll, 1, 1000) == 1)
		{
			recvmsg(sensor_fd, &sensor_msg, 0);
			sendmsg(sensor_send_fd, &sensor_msg, 0);
		}
	} while(1);
}

static void *sensor_start(void *arg)
{
	system("python3 lsm9ds1_input.py");
}

int send_input_loop_start()
{
	pthread_t sensor_thread;
	pthread_create(&sensor_thread, NULL, sensor_start, NULL);

	sleep(1);

	pthread_t send_input_thread;
	pthread_create(&send_input_thread, NULL, send_input_loop, NULL);
	return 0;
}
