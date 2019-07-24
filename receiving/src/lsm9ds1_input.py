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
            reconnected = False
            while not reconnected:
                try:
                    i2c = busio.I2C(board.SCL, board.SDA)
                    sensor = adafruit_lsm9ds1.LSM9DS1_I2C(i2c)
                    reconnected = True
                    print("reconnected")
                    break
                except ValueError as e:
                    reconnected = False
                    print("reconnecting...")
                    time.sleep(1)

        pitch = math.atan2(accel_y, accel_z) * 180/math.pi
        roll = math.atan2(accel_x, accel_z) * 180/math.pi

        x_flat = mag_x*math.cos(pitch) + mag_y*math.sin(pitch)*math.sin(roll) + mag_z*math.sin(pitch)*math.cos(roll)
        y_flat = mag_y*math.cos(roll) + mag_z*math.sin(roll)
        yaw = math.atan2(-y_flat, x_flat) * 180/math.pi

        conn.send(struct.pack('<3f', gyro_x, gyro_y, gyro_z)) # Little Endian Byte Order ("<")

def main():
    while True:
        conn, _ = server.accept()
        print_lock.acquire()
        start_new_thread(data_thread, (conn,))
    server.close()

if __name__ == '__main__':
    main()
