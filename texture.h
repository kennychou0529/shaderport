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
    void *data,
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

void DrawTextureFancy(int slot)
{
    static bool loaded = false;
    static GLuint program = 0;
    static GLint attrib_in_position = 0;
    static GLint uniform_mode = 0;
    static GLint uniform_gain = 0;
    static GLint uniform_bias = 0;
    static GLint uniform_channel0 = 0;
    if (!loaded)
    {
        #define SHADER(S) "#version 150\n" #S

        const char *vs = SHADER(
        in vec2 in_position;
        out vec2 texel;
        void main()
        {
            texel = vec2(0.5)+0.5*in_position;
            gl_Position = vec4(in_position, 0.0, 1.0);
        }
        );

        const char *fs = SHADER(
        in vec2 texel;
        uniform int mode;
        uniform vec4 gain;
        uniform vec4 bias;
        uniform sampler2D channel0;
        out vec4 color;
        void main()
        {
            const vec3 colormap[3] = vec3[](
                vec3(1.46159096e-03,   4.66127766e-04,   1.38655200e-02),
                vec3(2.25764007e-03,   1.29495431e-03,   1.83311461e-02),
                vec3(3.27943222e-03,   2.30452991e-03,   2.37083291e-02));

            vec4 sample = texture(channel0, texel);
            sample = gain*(sample + bias);

            // mode == single,float
            const vec4 selector = vec4(1.0, 0.0, 0.0, 0.0);
            const float range_min = 0.0;
            const float range_max = 1.0;
            float f = (dot(sample,selector) - range_min) / (range_max - range_min);
            f = clamp(f, 0.0, 1.0);
            f = f*float(colormap.length());
            int n = clamp(int(f), 0, colormap.length()-1);
            color.rgb = colormap[n];
            color.a = 1.0;

            // mode == gray
            // color = vec4(sample.r*2.0 + sample.b + sample.g*3.0)/6.0;

            // mode == rgb
            // color = sample;
        });

        #undef SHADER

        program = LoadShaderFromMemory(vs, fs);
        attrib_in_position = glGetAttribLocation(program, "in_position");
        uniform_channel0 = glGetUniformLocation(program, "channel0");
        uniform_gain = glGetUniformLocation(program, "gain");
        uniform_bias = glGetUniformLocation(program, "bias");
        // uniform_mode = glGetUniformLocation(program, "mode");

        loaded = true;
    }

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


    static float gain[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static float bias[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // rt.Enable();
    glUseProgram(program);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    BindTexture(slot);
    glUniform1i(uniform_channel0, 0);
    glUniform4fv(uniform_gain, 1, gain);
    glUniform4fv(uniform_bias, 1, bias);
    glVertexAttrib2f(attrib_in_position, -1.0f, -1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, -1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, -1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, -1.0f, -1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glUseProgram(0);
    // rt.Disable();

    // {
    //     using namespace ImGui;
    //     PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
    //     Begin("##texture");
    //     Image((ImTextureID)rt.color, ImVec2((float)rt.width, (float)rt.height));
    //     End();
    //     PopStyleVar();
    // }

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
}
