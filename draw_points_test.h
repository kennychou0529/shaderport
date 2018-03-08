#include "shader.h"
#include "point_shader.h"
#define _CRT_SECURE_NO_WARNINGS
#pragma warning (disable: 4201) // nonstandard extension used: nameless struct/union
#include "C:/Programming/lib-scv/so_math.h"
void DrawPointsTest(frame_input_t input)
{
    typedef void (*vertex_attrib_divisor_t)(GLuint, GLuint);
    static vertex_attrib_divisor_t VertexAttribDivisor = (vertex_attrib_divisor_t)glfwGetProcAddress("glVertexAttribDivisor");
    if (!VertexAttribDivisor)
        return; // todo: error
    static bool loaded = false;
    static GLuint program = 0;
    static GLint attrib_in_position = 0;
    static GLint attrib_instance_position = 0;
    static GLint attrib_instance_color = 0;
    static GLint uniform_reflection = 0;
    static GLint uniform_point_size = 0;
    static GLint uniform_projection = 0;
    static GLint uniform_model_to_view = 0;
    static GLuint vbo_position = 0;
    static GLuint vbo_color = 0;
    const int N = 16;
    const int num_points = N*N*N;
    static vec3 instance_position[num_points];
    static vec4 instance_color[num_points];
    if (!loaded)
    {
        program = LoadShaderFromMemory(point_shader_vs, point_shader_fs);
        attrib_in_position = glGetAttribLocation(program, "in_position");
        attrib_instance_position = glGetAttribLocation(program, "instance_position");
        attrib_instance_color = glGetAttribLocation(program, "instance_color");

        uniform_reflection = glGetUniformLocation(program, "reflection");
        uniform_projection = glGetUniformLocation(program, "projection");
        uniform_model_to_view = glGetUniformLocation(program, "model_to_view");
        uniform_point_size = glGetUniformLocation(program, "point_size");

        for (int i = 0; i < num_points; i++)
        {
            float r = (i % N)/(float)N;
            float g = ((i / N) % N)/(float)N;
            float b = ((i / N) / N)/(float)N;
            float x = -1.0f+2.0f*r;
            float y = -1.0f+2.0f*g;
            float z = -1.0f+2.0f*b;
            instance_position[i] = m_vec3(x,y,z);
            instance_color[i] = m_vec4(r,g,b,1);
        }

        glGenBuffers(1, &vbo_position);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*num_points, instance_position, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &vbo_color);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*num_points, instance_color, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        loaded = true;
    }

    // todo: do we need a vertex array object...?

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    mat4 P = mat_perspective(3.14f/4.0f, (float)input.window_w, (float)input.window_h, 0.1f, 20.0f);
    mat4 V = mat_translate(0,0,-3.5f)*mat_rotate_x(0.3f*input.elapsed_time)*mat_rotate_y(0.2f*input.elapsed_time);
    glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, P.data);
    glUniformMatrix4fv(uniform_model_to_view, 1, GL_FALSE, V.data);
    glUniform1f(uniform_point_size, 0.03f);

    {
        static int frame = 0;
        glUniform1f(uniform_reflection, frame == 0 ? -1.0f : +1.0f);
        frame = (frame + 1) % 2;
    }

    const int num_segments = 3;
    static float position[num_segments*3*2];
    for (int i = 0; i < num_segments; i++)
    {
        float t1 = 2.0f*3.1415926f*(i/(float)(num_segments));
        float t2 = 2.0f*3.1415926f*((i+1)/(float)(num_segments));
        position[6*i+0] = 0.0f;
        position[6*i+1] = 0.0f;
        position[6*i+2] = cosf(t1);
        position[6*i+3] = sinf(t1);
        position[6*i+4] = cosf(t2);
        position[6*i+5] = sinf(t2);
    }

    glVertexAttribPointer(attrib_in_position, 2, GL_FLOAT, GL_FALSE, 0, position);
    glEnableVertexAttribArray(attrib_in_position);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glEnableVertexAttribArray(attrib_instance_position);
    glVertexAttribPointer(attrib_instance_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    VertexAttribDivisor(attrib_instance_position, 1);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
    glEnableVertexAttribArray(attrib_instance_color);
    glVertexAttribPointer(attrib_instance_color, 4, GL_FLOAT, GL_FALSE, 0, 0);
    VertexAttribDivisor(attrib_instance_color, 1);

    glDrawArraysInstanced(GL_TRIANGLES, 0, num_segments*3, num_points);
    glDisableVertexAttribArray(attrib_in_position);
    glDisableVertexAttribArray(attrib_instance_position);
    glDisableVertexAttribArray(attrib_instance_color);
    VertexAttribDivisor(attrib_instance_position, 0);
    VertexAttribDivisor(attrib_instance_color, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
