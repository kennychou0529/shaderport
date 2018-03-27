#pragma once
#include "render_texture.h"
#include "shader.h"
#include "frame_input.h"

namespace TemporalBlend
{
    static render_texture_t rt_frame = {0};
    static render_texture_t rt_accumulator[2] = {0};
    static int turn = 0;
    // static render_texture_t rt2 = {0};
    // static int read_index = 0;
    // static int draw_index = 0;

    void Begin()
    {
        int w = frame_input.framebuffer_w;
        int h = frame_input.framebuffer_h;
        if (rt_frame.width != w || rt_frame.height != h)
        {
            FreeRenderTexture(&rt_frame);
            FreeRenderTexture(&rt_accumulator[0]);
            FreeRenderTexture(&rt_accumulator[1]);
            rt_frame = MakeRenderTexture(w, h, GL_NEAREST, GL_NEAREST, true);
            rt_accumulator[0] = MakeRenderTexture(w, h, GL_NEAREST, GL_NEAREST, false);
            rt_accumulator[1] = MakeRenderTexture(w, h, GL_NEAREST, GL_NEAREST, false);
        }
        rt_frame.Enable();
    }
    void End()
    {
        rt_frame.Disable();
    }
    void Draw(float factor)
    {
        static GLuint program = 0;
        static GLint attrib_position = 0;
        static GLint uniform_factor = 0;
        static GLint uniform_sampler0 = 0;
        static GLint uniform_sampler1 = 0;
        if (!program)
        {
            #define SHADER(S) "#version 150\n" #S
            const char *vs = SHADER(
            in vec2 position;
            out vec2 texel;
            void main()
            {
                texel = vec2(0.5) + 0.5*position;
                gl_Position = vec4(position, 0.0, 1.0);
            }
            );

            const char *fs = SHADER(
            in vec2 texel;
            uniform sampler2D sampler0; // current frame
            uniform sampler2D sampler1; // last accumulator
            uniform float factor;
            out vec4 out_color; // current accumulator
            void main()
            {
                // out_color = texture(sampler0,texel);
                vec4 current_frame = texture(sampler0,texel);
                vec4 last_accumulator = texture(sampler1,texel);
                vec4 current_accumulator = factor*last_accumulator + (1.0-factor)*current_frame;
                out_color = current_accumulator;

                // todo: alphablending, alpha buffers are kinda broken with this?
            }
            );
            #undef SHADER

            program = LoadShaderFromMemory(vs,fs);
            attrib_position = glGetAttribLocation(program, "position");
            uniform_sampler0 = glGetUniformLocation(program, "sampler0");
            uniform_sampler1 = glGetUniformLocation(program, "sampler1");
            uniform_factor = glGetUniformLocation(program, "factor");
            assert(attrib_position >= 0 && "Unused or nonexistent attribute");
            assert(uniform_sampler0 >= 0 && "Unused or nonexistent uniform");
            assert(uniform_sampler1 >= 0 && "Unused or nonexistent uniform");
            assert(uniform_factor >= 0 && "Unused or nonexistent uniform");
            assert(program && "Failed to compile TemporalBlend shader");
        }

        int next_turn = (turn+1)%2;
        int back = turn;
        int front = next_turn;

        rt_accumulator[front].Enable();

        glUseProgram(program);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(uniform_sampler0, 0);
        rt_frame.Bind();
        glActiveTexture(GL_TEXTURE1);
        glUniform1i(uniform_sampler1, 1);
        rt_accumulator[back].Bind();
        glUniform1f(uniform_factor, factor);

        static const float position[] = { -1,-1, 1,-1, 1,1, 1,1, -1,1, -1,-1 };
        glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, 0, position);
        glEnableVertexAttribArray(attrib_position);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(attrib_position);
        glActiveTexture(GL_TEXTURE0);
        rt_frame.Unbind();
        rt_accumulator[back].Unbind();
        glUseProgram(0);

        rt_accumulator[front].Disable();

        rt_accumulator[front].Bind();
        glBegin(GL_TRIANGLES);
        glColor4f(1,1,1,1);
        glTexCoord2f(0,0);glVertex2f(-1,-1);
        glTexCoord2f(1,0);glVertex2f(+1,-1);
        glTexCoord2f(1,1);glVertex2f(+1,+1);
        glTexCoord2f(1,1);glVertex2f(+1,+1);
        glTexCoord2f(0,1);glVertex2f(-1,+1);
        glTexCoord2f(0,0);glVertex2f(-1,-1);
        glEnd();
        rt_accumulator[front].Unbind();
        glDisable(GL_TEXTURE_2D);

        turn = next_turn;
    }
};

