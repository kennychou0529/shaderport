#pragma once
#include "gl_error.h"

struct render_texture_t
{
    GLuint fbo;
    GLuint color;
    GLuint depth;
    int width;
    int height;
};

render_texture_t MakeRenderTexture(int width, int height, bool enable_depth=false);

//
//
//

GLuint MakeRenderBuffer(int width, int height, GLenum format)
{
    GLuint result;
    glGenRenderbuffers(1, &result);
    glBindRenderbuffer(GL_RENDERBUFFER, result);
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return result;
}

render_texture_t MakeRenderTexture(int width, int height, bool enable_depth)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    CheckGLError("Creating framebuffer object");

    // todo: what if we want float texture as internal format?
    GLuint color = TexImage2D(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);
    CheckGLError("Creating color texture for render target");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    // todo: more color attachments, if you want multi-output shaders

    GLuint depth = 0;
    if (enable_depth)
    {
        // DEPTH_COMPONENT16
        // DEPTH_COMPONENT24
        // DEPTH_COMPONENT32
        // DEPTH_COMPONENT32F
        depth = MakeRenderBuffer(width, height, GL_DEPTH_COMPONENT24);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        printf("Framebuffer not complete\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckGLError("Creating framebuffer");

    render_texture_t result = {0};
    result.fbo = fbo;
    result.color = color;
    result.depth = depth;
    result.width = width;
    result.height = height;
    return result;
}
