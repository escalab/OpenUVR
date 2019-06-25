import cffi
ffibuilder = cffi.FFI()

with open('ssim_plugin.h') as f:
    data = ''.join([line for line in f])
    ffibuilder.embedding_api(data)

ffibuilder.set_source("ssim_plugin", r'''
    #include "ssim_plugin.h"
''')

with open('ssim_plugin_src.py') as f:
    data = ''.join([line for line in f])
    ffibuilder.embedding_init_code(data)

ffibuilder.emit_c_code("ssim_plugin.c")