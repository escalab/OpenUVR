#
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
#

from ssim_plugin import ffi
import tensorflow as tf

import numpy as np

import time
import random
import io
import sys

NUM_BUFS = 150

ref_img_data = [None]*NUM_BUFS
cmp_img_data = [None]*NUM_BUFS
for i in range(NUM_BUFS):
    ref_img_data[i] = bytearray(1920*1080)
    cmp_img_data[i] = bytearray(1920*1080)


cur_idx = 0
cur_batch_num = 0


def compute_ssim():
    global ref_img_data, cmp_img_data
    print('------------------------Beginning to analyze ssim-------------------------------')
    # sess = tf.Session(config=tf.ConfigProto(log_device_placement=True))
    num_chunks = int(NUM_BUFS/10)

    with tf.Graph().as_default(), tf.Session() as sess:
        ref_ph = tf.placeholder(tf.uint8, shape=[NUM_BUFS, 1920*1080])
        ref_var = tf.Variable(ref_ph)
        ref_reshaped = tf.reshape(ref_var, [num_chunks, 10, 1080, 1920, 1])

        cmp_ph = tf.placeholder(tf.uint8, shape=[NUM_BUFS, 1920*1080])
        cmp_var = tf.Variable(cmp_ph)
        cmp_reshaped = tf.reshape(cmp_var, [num_chunks, 10, 1080, 1920, 1])

        # with tf.device('/gpu:0'):

        sess.run(ref_var.initializer, feed_dict={ref_ph: ref_img_data})
        sess.run(cmp_var.initializer, feed_dict={cmp_ph: cmp_img_data})

        total_mean = 0
        for i in range(num_chunks):
            ssim = tf.image.ssim(ref_reshaped[i], cmp_reshaped[i], 255)
            ssim_mean = tf.math.reduce_mean(ssim)

            res = sess.run(ssim)
            res_mean = sess.run(ssim_mean)
            print(res)
            print(res_mean)
            total_mean += res_mean

        total_mean /= num_chunks

        sess.close()

    print('------------------------RESULTS-------------------------------')
    print(total_mean)


@ffi.def_extern()
def py_ssim_set_ref_image_data(buf):
    global ref_img_data, cur_idx
    ffi.memmove(ref_img_data[cur_idx], buf, 1920*1080)


@ffi.def_extern()
def py_ssim_set_cmp_image_data_or_compute_ssim(buf):
    global ref_img_data, cmp_img_data, cur_idx

    ffi.memmove(cmp_img_data[cur_idx], buf, 1920*1080)

    cur_idx += 1
    if cur_idx < NUM_BUFS:
        return
    cur_idx = 0

    compute_ssim()
