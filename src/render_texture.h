// Usage example:
// render_texture_t rt = MakeRenderTexture(...);
// ...
// rt.Enable();
// ... render content
// rt.Disable();
// ...
// rt.Bind();
// ... use texture2D
// rt.Unbind();

#pragma once
#include "gl_error.h"
#include "frame_input.h"

struct render_texture_t
{
    GLuint fbo;
    GLuint color;
    GLuint depth;
    int width;
    int height;

    void Enable()  { glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, width, height); }
    void Disable() { glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(0, 0, frame_input.framebuffer_w, frame_input.framebuffer_h); }
    void Bind()    { glBindTexture(GL_TEXTURE_2D, color); }
    void Unbind()  { glBindTexture(GL_TEXTURE_2D, 0); }
};

render_texture_t MakeRenderTexture(int width, int height, GLenum mag_filter=GL_LINEAR, GLenum min_filter=GL_LINEAR, bool enable_depth=false)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    CheckGLError("Creating framebuffer object");

    // todo: what if we want float texture as internal format?
    GLuint color = TexImage2D(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE, mag_filter, min_filter, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);
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
        glGenRenderbuffers(1, &depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
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

void FreeRenderTexture(render_texture_t *rt)
{
    if (rt->fbo)
        glDeleteFramebuffers(1, &rt->fbo);
    if (rt->color)
        glDeleteTextures(1, &rt->color);
    if (rt->depth)
        glDeleteRenderbuffers(1, &rt->depth);
    rt->width = 0;
    rt->height = 0;
    rt->fbo = 0;
    rt->color = 0;
    rt->depth = 0;
}
