/*
    The MIT License (MIT)

    Copyright (c) 2020 OpenUVR

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Authors:
    Alec Rohloff
    Zackary Allen
    Kung-Min Lin
    Chengyi Nie
    Hung-Wei Tseng
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <unistd.h>
#include "accelerometer_read.h"


void select_device(int file, int addr)
{
    if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        printf("Failed to select I2C device.\n");
        exit(1);
    }
}

void set_bandwidth_rate(int file, int rate)
{
    if (i2c_smbus_write_byte_data(file, BW_RATE, rate) < 0)
    {
        printf("Failed to write byte to i2c\n");
        exit(1);
    }
}

void set_range(int file, int range)
{
    int cur_range = i2c_smbus_read_byte_data(file, DATA_FORMAT);
    cur_range &= ~0x0F;
    cur_range |= range;
    cur_range |= 0x08;

    if (i2c_smbus_write_byte_data(file, DATA_FORMAT, cur_range) < 0)
    {
        printf("Failed to set range\n");
        exit(1);
    }
}

void enable_measurement(int file)
{
    if (i2c_smbus_write_byte_data(file, POWER_CTL, MEASURE) < 0)
    {
        printf("Failed to enable measurement\n");
        exit(1);
    }
}

int get_axes(uint8_t bytes[], int r1, int r2)
{
    int a = bytes[r1] | (bytes[r2] << 8);
    if (a & (1 << 16 - 1))
        a -= (1 << 16);
    return a;
}

void get_coord(int file, struct coord* c)
{
    uint8_t bytes[6];
    if (i2c_smbus_read_i2c_block_data(file, AXES_DATA, 6, bytes) < 0)
    {
        printf("Failed to read axes\n");
        exit(1);
    }

    int x = get_axes(bytes, 0, 1);
    int y = get_axes(bytes, 2, 3);
    int z = get_axes(bytes, 4, 5);

    c->x = x * SCALE_MULTIPLIER;
    c->y = y * SCALE_MULTIPLIER;
    c->z = z * SCALE_MULTIPLIER;
}

int main(int argc, char* argv[])
{
    FILE* i2c_file = fopen("/dev/i2c-1", "rw");
    if (i2c_file < 0)
    {
        printf("Unable to open i2c bus!\n");
        exit(1);
    }
    int i2c_fileno = fileno(i2c_file);
    select_device(i2c_fileno, ACCEL_ADDR);
    set_bandwidth_rate(i2c_fileno, BW_RATE_100HZ);
    set_range(i2c_fileno, RANGE_2G);
    enable_measurement(i2c_fileno);

    struct coord c;
    while(1) 
    {
        get_coord(i2c_fileno, &c);
        printf("%f %f %f\n", c.x, c.y, c.z);
        usleep(100000 / 60);
    }
    fclose(i2c_file);

    return 0;
}