#pragma once

// The temporal super sampler renders a high-resolution framebuffer
// with dimensions (W<<N,H<<N) where N is the upsampling factor.
// This framebuffer is linearly downsampled and blitted to the window.
// The rendering is done over several frames where each frame is assigned
// to a regular sampling of the high-resolution buffer. Here for example
// is a 3x2 framebuffer supersampled to 6x4.
//                          -----------------
//                         | 1 2 | 1 2 | 1 2 |
//                         | 3 4 | 3 4 | 3 4 |
//                         |-----+-----+-----|
//                         | 1 2 | 1 2 | 1 2 |
//                         | 3 4 | 3 4 | 3 4 |
//                          -----------------
// Each number refers to the frame at which that pixel gets rendered.
// For example, in frame 1, only the upper-left pixels are rendered
// to a low-resolution framebuffer (here 3x2), and then distributed
// to the high-res framebuffer:
//                                   _________________
//                 ___________      | 1 # | 1 # | 1 # |
//                | 1 | 1 | 1 |     | # # | # # | # # |
//                | --+---+---| --> |-----+-----+-----|
//                | 1 | 1 | 1 |     | 1 # | 1 # | 1 # |
//                 -----------      | # # | # # | # # |
//                                   -----------------
// In this example, it would take 4 frames before the output is stable,
// assuming a static scene.
namespace TemporalSuperSample
{
    static render_texture_t output;
    static render_texture_t lowres;
    static int subpixel;
    static int upsample;
    // dx,dy = pixel offset to apply to projection matrix element (0,2) and (1,2)
    // . . dx 0
    // . . dy 0
    // . .  . .
    // . .  . 1
    //
    void GetSubpixelOffset(int w, int h, int n, float *dx, float *dy)
    {
        int idx = subpixel % (1<<n);
        int idy = subpixel / (1<<n);
        float pixel_width = 2.0f/(w<<n); // width in NDC [-1,+1] space
        float pixel_height = 2.0f/(h<<n); // width in NDC [-1,+1] space
        *dx = idx*pixel_width;
        *dy = idy*pixel_height;
    }
    void Begin(int w, int h, int n)
    {
        if (lowres.width != w || lowres.height != h)
        {
            FreeRenderTexture(&lowres);
            lowres = MakeRenderTexture(w, h, GL_NEAREST, GL_NEAREST, true);
            subpixel = 0;
        }
        if (output.width != w<<n || output.height != h<<n)
        {
            FreeRenderTexture(&output);
            output = MakeRenderTexture(w<<n, h<<n, GL_LINEAR, GL_LINEAR, false);
            subpixel = 0;
        }
        upsample = n;
        lowres.Enable();
    }
    void End()
    {
        lowres.Disable();
    }
    void Draw()
    {
        static GLuint program = 0;
        static GLint attrib_position = 0;
        static GLint uniform_sampler0 = 0;
        static GLint uniform_dx = 0;
        static GLint uniform_dy = 0;
        static GLint uniform_tile_dim = 0;
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
            uniform sampler2D sampler0;
            uniform float dx;
            uniform float dy;
            uniform float tile_dim;
            out vec4 out_color;
            void main()
            {
                out_color = texture(sampler0, texel);
                float ix = trunc(mod(gl_FragCoord.x,tile_dim));
                float iy = trunc(mod(gl_FragCoord.y,tile_dim));
                if (ix == dx && iy == dy)
                {
                    out_color = texture(sampler0,texel);
                }
                else
                {
                    out_color = vec4(0.0);
                }
            }
            );
            #undef SHADER

            program = LoadShaderFromMemory(vs,fs);
            attrib_position = glGetAttribLocation(program, "position");
            uniform_sampler0 = glGetUniformLocation(program, "sampler0");
            uniform_dx = glGetUniformLocation(program, "dx");
            uniform_dy = glGetUniformLocation(program, "dy");
            uniform_tile_dim = glGetUniformLocation(program, "tile_dim");
            assert(attrib_position >= 0 && "Unused or nonexistent attribute");
            assert(uniform_sampler0 >= 0 && "Unused or nonexistent uniform");
            assert(uniform_dx >= 0 && "Unused or nonexistent uniform");
            assert(uniform_dy >= 0 && "Unused or nonexistent uniform");
            assert(uniform_tile_dim >= 0 && "Unused or nonexistent uniform");
            assert(program && "Failed to compile TemporalBlend shader");
        }

        output.Enable();

        glUseProgram(program);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(uniform_sampler0, 0);
        glBindTexture(GL_TEXTURE_2D, lowres.color);
        glActiveTexture(GL_TEXTURE1);
        glUniform1f(uniform_dx, (float)(subpixel % (1<<upsample)));
        glUniform1f(uniform_dy, (float)(subpixel / (1<<upsample)));
        glUniform1f(uniform_tile_dim, (float)(1<<upsample));

        static const float position[] = { -1,-1, 1,-1, 1,1, 1,1, -1,1, -1,-1 };
        glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, 0, position);
        glEnableVertexAttribArray(attrib_position);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(attrib_position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,0);
        glUseProgram(0);

        output.Disable();

        glBindTexture(GL_TEXTURE_2D, output.color);
        glBegin(GL_TRIANGLES);
        glColor4f(1,1,1,1);
        glTexCoord2f(0,0);glVertex2f(-1,-1);
        glTexCoord2f(1,0);glVertex2f(+1,-1);
        glTexCoord2f(1,1);glVertex2f(+1,+1);
        glTexCoord2f(1,1);glVertex2f(+1,+1);
        glTexCoord2f(0,1);glVertex2f(-1,+1);
        glTexCoord2f(0,0);glVertex2f(-1,-1);
        glEnd();
        glBindTexture(GL_TEXTURE_2D,0);
        glDisable(GL_TEXTURE_2D);

        // just a linear sampling order. want something nicer in the future
        int num_subpixels = (1<<upsample)*(1<<upsample);
        subpixel = (subpixel+1)%num_subpixels;
    }
};
