# The MIT License (MIT)

# Copyright (c) 2020 OpenUVR

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Authors:
# Alec Rohloff
# Zackary Allen
# Kung-Min Lin
# Chengyi Nie
# Hung-Wei Tseng

import math

# Data Logging
import time
import board
import busio
import adafruit_lsm9ds1

# Socket
import json
import socket
import os
from _thread import *
import threading
import struct

print_lock = threading.Lock()

ADDR = "/tmp/openuvr_hmd_input_socket"
if os.path.exists(ADDR):
    os.remove(ADDR)

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(ADDR)
server.listen(5)

i2c = busio.I2C(board.SCL, board.SDA)		# Connect sensors via I2C
sensor = adafruit_lsm9ds1.LSM9DS1_I2C(i2c)	# Identify sensor as Adafruit LSM9DS1

def euler_to_quaternion(yaw, pitch, roll):
    cy = math.cos(yaw * 0.5)
    sy = math.sin(yaw * 0.5)
    cp = math.cos(pitch * 0.5)
    sp = math.sin(pitch * 0.5)
    cr = math.cos(roll * 0.5)
    sr = math.sin(roll * 0.5)

    q = {'w': 0.0, 'x': 0.0, 'y': 0.0, 'z': 0.0}
    q['w'] = cy * cp * cr + sy * sp * sr
    q['x'] = cy * cp * sr - sy * sp * cr
    q['y'] = sy * cp * sr + cy * sp * cr
    q['z'] = sy * cp * cr - cy * sp * sr

    return q

def data_thread(conn):
    while True:
        global sensor
        try:
            sensor_instance = sensor
            accel_x, accel_y, accel_z = sensor.acceleration
            mag_x, mag_y, mag_z = sensor.magnetic
            gyro_x, gyro_y, gyro_z = sensor.gyro
            temp = sensor.temperature
        except OSError as e:
            print("server disconnected")
            reconnected = False
            while not reconnected:
                try:
                    i2c = busio.I2C(board.SCL, board.SDA)
                    sensor = adafruit_lsm9ds1.LSM9DS1_I2C(i2c)
                    reconnected = True
                    print("server reconnected")
                    break
                except ValueError as e:
                    reconnected = False
                    print("server reconnecting...")
                    time.sleep(1)

        # Orientation Calculation (can be later used if a player's orientation can be set, instead of being controlled by a joystick or mouse of which orientation cannot be set to specific values.)
        
        # pitch = math.atan2(accel_y, accel_z) * 180/math.pi
        # roll = math.atan2(accel_x, accel_z) * 180/math.pi

        # x_flat = mag_x*math.cos(pitch) + mag_y*math.sin(pitch)*math.sin(roll) + mag_z*math.sin(pitch)*math.cos(roll)
        # y_flat = mag_y*math.cos(roll) + mag_z*math.sin(roll)
        # yaw = math.atan2(-y_flat, x_flat) * 180/math.pi
        
        conn.send(struct.pack('<3f', gyro_x, gyro_y, gyro_z)) # Little Endian Byte Order ("<")

        # time.sleep(0.01)

def main():
    # pygame.init()
    # display = (800, 600)
    # pygame.display.set_mode(display, DOUBLEBUF|OPENGL)
    # pygame.display.set_caption("Orientation Simulation")
    # gluPerspective(45, (display[0]/display[1]), 0.1, 50.0)
    # glTranslatef(0.0, 0.0, -5)

    while True:
        conn, _ = server.accept()
        print_lock.acquire()
        start_new_thread(data_thread, (conn,))
    server.close()

if __name__ == '__main__':
    main()
