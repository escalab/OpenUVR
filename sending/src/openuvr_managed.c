#include "openuvr.h"
#include "ouvr_packet.h"
#include <stdlib.h>
#include <stdio.h>
#include <GLES3/gl3.h>

static GLuint pbo = 0;
static unsigned char *cpu_encoding_buf = NULL;

int openuvr_managed_init(enum OPENUVR_ENCODER_TYPE enc_type, enum OPENUVR_NETWORK_TYPE net_type)
{
    GLint dims[4] = {0};
    glGetIntegerv(GL_VIEWPORT, dims);
    GLint w = dims[2];
    GLint h = dims[3];
    if (w != 1920 || h != 1080)
    {
        PRINT_ERR("Viewport dimensions are %dx%d. You must set the dimensions to 1920x1080.\n", w, h);
        return -1;
    }

    if (enc_type == OPENUVR_ENCODER_H264_CUDA)
    {

        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, 1920 * 1080 * 4, 0, GL_DYNAMIC_COPY);
    }
    else
    {

        cpu_encoding_buf = (unsigned char *)malloc(1920 * 1080 * 4);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cpu_encoding_buf);
    }

    GLint size = 0;
    glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE, &size);
    struct openuvr_context *ctx = openuvr_alloc_context(enc_type, net_type, cpu_encoding_buf, pbo);
    if (ctx == NULL)
    {
        PRINT_ERR("Couldn't allocate openuvr context\n");
        return -1;
    }
    openuvr_cuda_copy(ctx);
    openuvr_init_thread_continuous(ctx);

    return 0;
}

void openuvr_managed_copy_framebuffer()
{
    GLuint bound_pbo;
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, (GLint *)&bound_pbo);
    if (pbo != 0)
    {
        if (bound_pbo != pbo)
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
            glBufferData(GL_PIXEL_PACK_BUFFER, 1920 * 1080 * 4, 0, GL_DYNAMIC_COPY);
        }

        glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
    else if (cpu_encoding_buf != 0)
    {
        glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, cpu_encoding_buf);
    }
}