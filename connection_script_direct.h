#pragma once
#include "script_input.h"

void vdb_render() { }
void vdb_new_frame() { }

typedef void (*script_loop_t)(script_input_t);
script_loop_t LoadScript()
{
    script_loop_t ScriptLoop = NULL;

    Log("reloading script\n");
    TCCState *s;
    s = tcc_new();
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    tcc_add_library_path(s, ".");
    if (tcc_add_file(s, "../script/test.c", TCC_FILETYPE_C) == -1)
    {
        Log("failed to compile script\n");
        return NULL;
    }

    // add API functions
    {
        tcc_add_symbol(s, "vdb_render", vdb_render);
        tcc_add_symbol(s, "vdb_new_frame", vdb_new_frame);

        tcc_add_symbol(s, "vdb_view", vdb_view);
        tcc_add_symbol(s, "vdb_color", vdb_color);
        tcc_add_symbol(s, "vdb_line_width", vdb_line_width);
        tcc_add_symbol(s, "vdb_point_size", vdb_point_size);
        tcc_add_symbol(s, "vdb_path_clear", vdb_path_clear);
        tcc_add_symbol(s, "vdb_path_to", vdb_path_to);
        tcc_add_symbol(s, "vdb_path_fill", vdb_path_fill);
        tcc_add_symbol(s, "vdb_path_stroke", vdb_path_stroke);
        tcc_add_symbol(s, "vdb_text", vdb_text_formatted);
        tcc_add_symbol(s, "vdb_point", vdb_point);
        tcc_add_symbol(s, "vdb_line", vdb_line);
        tcc_add_symbol(s, "vdb_triangle", vdb_triangle);
        tcc_add_symbol(s, "vdb_triangle_filled", vdb_triangle_filled);
        tcc_add_symbol(s, "vdb_quad", vdb_quad);
        tcc_add_symbol(s, "vdb_quad_filled", vdb_quad_filled);
        tcc_add_symbol(s, "vdb_rect", vdb_rect);
        tcc_add_symbol(s, "vdb_rect_filled", vdb_rect_filled);
        tcc_add_symbol(s, "vdb_circle", vdb_circle);
        tcc_add_symbol(s, "vdb_circle_filled", vdb_circle_filled);
    }

    static unsigned char *code = NULL;
    int n = tcc_relocate(s, NULL);
    if (code)
    {
        free(code);
    }
    code = (unsigned char*)malloc(n);
    if (tcc_relocate(s, code) == -1)
    {
        Log("failed to compile script\n");
        return NULL;
    }
    ScriptLoop = (script_loop_t)tcc_get_symbol(s, "loop");
    if (!ScriptLoop)
    {
        Log("failed to compile script: where is the loop function?\n");
        return NULL;
    }
    tcc_delete(s);
    return ScriptLoop;
}

#include "render_texture.h"
#include "shader.h"


void ScriptUpdateAndDraw(frame_input_t input, bool reload)
{
    static script_loop_t ScriptLoop = NULL;
    if (reload)
    {
        ScriptLoop = LoadScript();
    }

    // testing render textures
    #if 1
    static bool first = true;
    static render_texture_t rt = {0};
    static GLuint program = 0;
    if (first)
    {
        first = false;

        const char *vs = "#version 150\n"
        "in vec2 in_position;\n"
        "out vec2 position;\n"
        "void main()\n"
        "{\n"
        "    position = in_position;\n"
        "    gl_Position = vec4(in_position, 0.0, 1.0);\n"
        "}\n";

        const char *fs = "#version 150\n"
        "in vec2 position;\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "    color = vec4(0.5*position.x+0.5, 0.5*position.y+0.5, 0.5, 1.0);\n"
        "}\n";

        program = LoadShaderFromMemory(vs, fs);
        rt = MakeRenderTexture(32, 32, GL_NEAREST, GL_NEAREST);
    }

    // todo: getVertexattribLocation

    rt.Enable();
    glUseProgram(program);
    GLint slot = glGetAttribLocation(program, "in_position");
    glVertexAttrib2f(slot, -1.0f, -1.0f);
    glVertexAttrib2f(slot, +1.0f, -1.0f);
    glVertexAttrib2f(slot, +1.0f, +1.0f);
    glVertexAttrib2f(slot, +1.0f, +1.0f);
    glVertexAttrib2f(slot, -1.0f, +1.0f);
    glVertexAttrib2f(slot, -1.0f, -1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUseProgram(0);
    rt.Disable();

    glEnable(GL_TEXTURE_2D);
    rt.Bind();
    glBegin(GL_TRIANGLES);
    glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-0.5f,-0.5f);
    glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+0.5f,-0.5f);
    glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+0.5f,+0.5f);
    glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+0.5f,+0.5f);
    glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-0.5f,+0.5f);
    glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-0.5f,-0.5f);
    glEnd();
    rt.Unbind();
    glDisable(GL_TEXTURE_2D);
    #endif

    if (ScriptLoop)
    {
        script_input_t s = {0};
        s.window_x = input.window_x;
        s.window_y = input.window_y;
        s.window_w = input.window_w;
        s.window_h = input.window_h;
        s.framebuffer_w = input.framebuffer_w;
        s.framebuffer_h = input.framebuffer_h;
        s.mouse_x = input.mouse_x;
        s.mouse_y = input.mouse_y;
        s.mouse_u = input.mouse_u;
        s.mouse_v = input.mouse_v;
        s.elapsed_time = input.elapsed_time;
        s.frame_time = input.frame_time;
        s.recording_video = input.recording_video;
        s.lost_focus = input.lost_focus;
        s.regained_focus = input.regained_focus;
        ScriptLoop(s);
    }
}
