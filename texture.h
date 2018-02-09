#pragma once
#include "3rdparty/glad.h"

// TEXTURES
//   Can be assigned to a 'slot' which can then be bound to render it on meshes.
//   OpenGL supports many texture formats aside from unsigned 8 bit values, for example,
//   32bit float or 32bit int, as well as anywhere from one to four components (RGBA).
// EXAMPLE
//   unsigned char data[128*128*3];
//   SetTexture(0, data, 128, 128, GL_RGB, GL_UNSIGNED_BYTE);
//   DrawTexture(0);
// EXAMPLE
//   Drawing a mipmap'd texture
//   SetTexture(0, data, 128, 128, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
// EXAMPLE
//                           data_format   data_type
//   Grayscale 32 bit float: GL_LUMINANCE  GL_FLOAT
//   RGB       32 bit float: GL_RGB        GL_FLOAT
//   RGB        8 bit  char: GL_RGB        GL_UNSIGNED_BYTE
void SetTexture(
    int slot,
    void *data,
    int width,
    int height,
    GLenum data_format = GL_RGB,
    GLenum data_type = GL_UNSIGNED_BYTE,
    GLenum mag_filter = GL_LINEAR,
    GLenum min_filter = GL_LINEAR,
    GLenum wrap_s = GL_CLAMP_TO_EDGE,
    GLenum wrap_t = GL_CLAMP_TO_EDGE,
    GLenum internal_format = GL_RGBA);
void DrawTexture(int slot); // Draws the texture to the entire viewport
void BindTexture(int slot); // Enable a texture for custom drawing

//
// Implementation
//

GLuint TexImage2D(
    const void *data,
    int width,
    int height,
    GLenum data_format,
    GLenum data_type = GL_UNSIGNED_BYTE,
    GLenum mag_filter = GL_LINEAR,
    GLenum min_filter = GL_LINEAR,
    GLenum wrap_s = GL_CLAMP_TO_EDGE,
    GLenum wrap_t = GL_CLAMP_TO_EDGE,
    GLenum internal_format = GL_RGBA)
{
    GLuint result = 0;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    if (min_filter == GL_LINEAR_MIPMAP_LINEAR)
    {
        glGenerateMipmap(GL_TEXTURE_2D); // todo
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }
    glTexImage2D(GL_TEXTURE_2D, 0,
                 internal_format,
                 width,
                 height,
                 0,
                 data_format,
                 data_type,
                 data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return result;
}

GLuint GetTextureSlotHandle(int slot)
{
    return 4040 + slot;
}

void SetTexture(
    int slot,
    void *data,
    int width,
    int height,
    GLenum data_format,
    GLenum data_type,
    GLenum mag_filter,
    GLenum min_filter,
    GLenum wrap_s,
    GLenum wrap_t,
    GLenum internal_format)
{
    GLuint tex = GetTextureSlotHandle(slot); // Hopefully no one else uses this texture range!
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    // glGenerateMipmap(GL_TEXTURE_2D); // todo
    if (min_filter == GL_LINEAR_MIPMAP_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 internal_format,
                 width,
                 height,
                 0,
                 data_format,
                 data_type,
                 data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BindTexture(int slot)
{
    glBindTexture(GL_TEXTURE_2D, GetTextureSlotHandle(slot));
}

void DrawTexture(int slot)
{
    glEnable(GL_TEXTURE_2D);
    BindTexture(slot);
    glBegin(GL_TRIANGLES);
    glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-1,-1);
    glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,-1);
    glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+1,+1);
    glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+1,+1);
    glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,+1);
    glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-1,-1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

#include "shader.h"
#include "render_texture.h"
#include "colormap_inferno.h"
#include "texture_shader.h"

enum texture_colormap_t
{
    texture_colormap_rgb,
    texture_colormap_gray,
    texture_colormap_inferno
};

void DrawTextureFancy(GLuint texture, texture_colormap_t mode=texture_colormap_rgb, float *selector=NULL, float range_min=0.0f, float range_max=1.0f, float *gain=NULL, float *bias=NULL)
{
    static bool loaded = false;
    static GLuint program = 0;
    static GLuint colormap = 0;
    static GLint attrib_in_position = 0;
    static GLint uniform_gain = 0;
    static GLint uniform_bias = 0;
    static GLint uniform_selector = 0;
    static GLint uniform_blend = 0;
    static GLint uniform_range_min = 0;
    static GLint uniform_range_max = 0;
    static GLint uniform_channel0 = 0;
    static GLint uniform_channel1 = 0;
    if (!loaded)
    {
        colormap = TexImage2D(colormap_inferno, colormap_inferno_length, 1, GL_RGB, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);
        program = LoadShaderFromMemory(texture_shader_vs, texture_shader_fs);
        attrib_in_position = glGetAttribLocation(program, "in_position");
        uniform_channel0 = glGetUniformLocation(program, "channel0");
        uniform_channel1 = glGetUniformLocation(program, "channel1");
        uniform_gain = glGetUniformLocation(program, "gain");
        uniform_bias = glGetUniformLocation(program, "bias");
        uniform_selector = glGetUniformLocation(program, "selector");
        uniform_blend = glGetUniformLocation(program, "blend");
        uniform_range_min = glGetUniformLocation(program, "range_min");
        uniform_range_max = glGetUniformLocation(program, "range_max");
        loaded = true;
    }

    #if 0
    static render_texture_t rt = {0};
    {
        int width,height;
        BindTexture(slot);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (rt.width != width || rt.height != height)
        {
            rt = MakeRenderTexture(width, height, GL_NEAREST, GL_NEAREST);
        }
    }
    #endif


    // rt.Enable();
    glUseProgram(program);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colormap);
    glUniform1i(uniform_channel0, 0);
    glUniform1i(uniform_channel1, 1);

    if (mode == texture_colormap_rgb) glUniform1f(uniform_blend, 0.0f);
    else glUniform1f(uniform_blend, 1.0f);

    if (mode == texture_colormap_gray) glUniform4f(uniform_selector, 2.0f/6.0f, 1.0f/6.0f, 3.0f/6.0f, 0.0f);
    else if (selector) glUniform4fv(uniform_selector, 1, selector);
    else glUniform4f(uniform_selector, 1.0f, 0.0f, 0.0f, 0.0f);

    if (gain) glUniform4fv(uniform_gain, 1, gain);
    else glUniform4f(uniform_gain, 1.0f, 1.0f, 1.0f, 1.0f);

    if (bias) glUniform4fv(uniform_bias, 1, bias);
    else glUniform4f(uniform_bias, 0.0f, 0.0f, 0.0f, 0.0f);

    glUniform1f(uniform_range_min, range_min);
    glUniform1f(uniform_range_max, range_max);

    glVertexAttrib2f(attrib_in_position, -1.0f, -1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, -1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, -1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, -1.0f, -1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glUseProgram(0);
    // rt.Disable();

    #if 0
    // {
    //     using namespace ImGui;
    //     PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
    //     Begin("##texture");
    //     Image((ImTextureID)rt.color, ImVec2((float)rt.width, (float)rt.height));
    //     End();
    //     PopStyleVar();
    // }
    #endif

    #if 0
    {
        using namespace ImGui;
        static bool rgb_gain = false;
        static bool rgb_bias = false;
        Text("Contrast");
        if (rgb_gain)
        {
            DragFloat3("##Contrast", gain, 0.1f);
        }
        else
        {
            DragFloat("##Contrast", gain, 0.1f);
            if (gain[0] < 0.0f)
                gain[0] = 0.0f;
            gain[1] = gain[0];
            gain[2] = gain[0];
        }
        SameLine();
        Checkbox("RGB##rgb_gain", &rgb_gain);
        Text("Brightness");
        if (rgb_bias)
        {
            SliderFloat3("##Brightness", bias, -1.0f, 1.0f);
        }
        else
        {
            SliderFloat("##Brightness", bias, -1.0f, 1.0f);
            bias[1] = bias[0];
            bias[2] = bias[0];
        }
        SameLine();
        Checkbox("RGB##rgb_bias", &rgb_bias);
    }
    #endif
}
