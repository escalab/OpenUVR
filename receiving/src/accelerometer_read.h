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
#define EARTH_GRAVITY_MS2	9.80665
#define SCALE_MULTIPLIER	0.004
#define DATA_FORMAT		0x31
#define BW_RATE			0x2C
#define POWER_CTL		0X2D
#define BW_RATE_1600HZ		0X0F
#define BW_RATE_800HZ		0X0E
#define BW_RATE_400HZ		0X0D
#define BW_RATE_200HZ		0X0C
#define BW_RATE_100HZ		0X0B
#define BW_RATE_50HZ		0X0A
#define BW_RATE_25HZ		0X09
#define RANGE_2G		0X00
#define RANGE_4G		0X01
#define RANGE_8G		0X02
#define RANGE_16G		0X03
#define MEASURE			0X08
#define AXES_DATA		0X32
#define ACCEL_ADDR		0x53

struct coord {
   double x;
   double y;
   double z;
} coord; 
