#pragma once
#include "gl_error.h"

struct render_texture_t
{
    GLuint fbo;
    GLuint color[SO_FBO_MAX_COLOR_ATTACHMENTS];
    GLenum target[SO_FBO_MAX_COLOR_ATTACHMENTS];
    GLuint depth;
    int color_count;
    int width;
    int height;
};

void MakeRenderTexture();

GLuint MakeRenderBuffer(int width, int height, GLenum format)
{
    GLuint result;
    glGenRenderbuffers(1, &result);
    glBindRenderbuffer(GL_RENDERBUFFER, result);
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    CheckGLError();
    return result;
}
