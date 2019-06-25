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

# saves all NUM_BUFS image for each iteration to a file, to be read later by ssim_standalone.py


def save_image_batch():
    global cur_batch_num
    print('------------------------Saving image batch %d-------------------------------' % cur_batch_num)
    ref_out = io.open('/home/arrohlof/Pictures/ssim/ref%d' %
                      cur_batch_num, 'wb')
    cmp_out = io.open('/home/arrohlof/Pictures/ssim/cmp%d' %
                      cur_batch_num, 'wb')
    for i in range(NUM_BUFS):
        ref_out.write(ref_img_data[i])
        cmp_out.write(cmp_img_data[i])
    ref_out.close()
    cmp_out.close()

    cur_batch_num += 1


def save_yuv_values_as_text(index):
    ref_out = io.open('/home/arrohlof/Pictures/ssim/ref_yuv.txt', 'w')
    cmp_out = io.open('/home/arrohlof/Pictures/ssim/cmp_yuv.txt', 'w')

    ref_to_be_written = ref_img_data[index]
    cmp_to_be_written = cmp_img_data[index]

    for i in range(len(ref_to_be_written)):
        ref_out.write(str(ref_to_be_written[i])+' ')
        cmp_out.write(str(cmp_to_be_written[i])+' ')
    ref_out.close()
    cmp_out.close()


def compute_ssim():
    global ref_img_data, cmp_img_data
    print('------------------------Beginning to analyze ssim-------------------------------')
    # sess = tf.Session(config=tf.ConfigProto(log_device_placement=True))
    num_chunks = int(NUM_BUFS/10)

    save_yuv_values_as_text(0)

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
    # save_image_batch()
