// todo: This is temporary. Just for drafting out the API
#include "script.h"

typedef void (*script_loop_t)(script_input_t);

void PushCommandUint8(uint8_t x) {
    command_buffer_t *b = GetBackBuffer();
    assert(b && b->data && "Back buffer not allocated");
    assert(b->used + sizeof(uint8_t) <= b->max_size);
    *(uint8_t*)(b->data + b->used) = x;
    b->used += sizeof(uint8_t);
}
void PushCommandFloat32(float x) {
    command_buffer_t *b = GetBackBuffer();
    assert(b && b->data && "Back buffer not allocated");
    assert(b->used + sizeof(float) <= b->max_size);
    *(float*)(b->data + b->used) = x;
    b->used += sizeof(float);
}
void PushCommandString(const char *x) {
    command_buffer_t *b = GetBackBuffer();
    assert(b && b->data && "Back buffer not allocated");
    while (*x) {
        assert(b->used + 1 <= b->max_size);
        *(char*)(b->data + b->used++) = *x;
        x++;
    }
}
#define f(x) PushCommandFloat32(x)
#define u(id) PushCommandUint8(id)
void PushCommand_vdb_new_frame() { GetBackBuffer()->used = 0; }
void PushCommand_vdb_render() { SwapCommandBuffers(); }
void PushCommand_vdb_view(float left, float right, float bottom, float top) { u(id_view); f(left); f(right); f(bottom); f(top); }
void PushCommand_vdb_color(float r, float g, float b, float a) { u(id_color); f(r); f(g); f(b); f(a); }
void PushCommand_vdb_line_width(float pixel_width) { u(id_line_width); f(pixel_width); }
void PushCommand_vdb_point_size(float pixel_size) { u(id_point_size); f(pixel_size); }
void PushCommand_vdb_path_clear() { u(id_path_clear); }
void PushCommand_vdb_path_to(float x, float y) { u(id_path_to); f(x); f(y); }
void PushCommand_vdb_path_fill() { u(id_path_fill); }
void PushCommand_vdb_path_stroke() { u(id_path_stroke); }
void PushCommand_vdb_text(float x, float y, const char *fmt, ...) { u(id_text); f(x); f(y); PushCommandString(fmt); }
void PushCommand_vdb_point(float x, float y) { u(id_point); f(x); f(y); }
void PushCommand_vdb_line(float x1, float y1, float x2, float y2) { u(id_line); f(x1); f(y1); f(x2); f(y2); }
void PushCommand_vdb_triangle(float x1, float y1, float x2, float y2, float x3, float y3) { u(id_triangle); f(x1); f(y1); f(x2); f(y2); f(x3); f(y3); }
void PushCommand_vdb_triangle_filled(float x1, float y1, float x2, float y2, float x3, float y3) { u(id_triangle_filled); f(x1); f(y1); f(x2); f(y2); f(x3); f(y3); }
void PushCommand_vdb_quad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) { u(id_quad); f(x1); f(y1); f(x2); f(y2); f(x3); f(y3); f(x4); f(y4); }
void PushCommand_vdb_quad_filled(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) { u(id_quad_filled); f(x1); f(y1); f(x2); f(y2); f(x3); f(y3); f(x4); f(y4); }
void PushCommand_vdb_rect(float x, float y, float w, float h) { u(id_rect); f(x); f(y); f(w); f(h); }
void PushCommand_vdb_rect_filled(float x, float y, float w, float h) { u(id_rect_filled); f(x); f(y); f(w); f(h); }
void PushCommand_vdb_circle(float x, float y, float r) { u(id_circle); f(x); f(y); f(r); }
void PushCommand_vdb_circle_filled(float x, float y, float r) { u(id_circle_filled); f(x); f(y); f(r); }
#undef f
#undef u


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
    #if 1
    {
        tcc_add_symbol(s, "vdb_new_frame", PushCommand_vdb_new_frame);
        tcc_add_symbol(s, "vdb_render", PushCommand_vdb_render);
        tcc_add_symbol(s, "vdb_view", PushCommand_vdb_view);
        tcc_add_symbol(s, "vdb_color", PushCommand_vdb_color);
        tcc_add_symbol(s, "vdb_line_width", PushCommand_vdb_line_width);
        tcc_add_symbol(s, "vdb_point_size", PushCommand_vdb_point_size);
        tcc_add_symbol(s, "vdb_path_clear", PushCommand_vdb_path_clear);
        tcc_add_symbol(s, "vdb_path_to", PushCommand_vdb_path_to);
        tcc_add_symbol(s, "vdb_path_fill", PushCommand_vdb_path_fill);
        tcc_add_symbol(s, "vdb_path_stroke", PushCommand_vdb_path_stroke);
        tcc_add_symbol(s, "vdb_text", PushCommand_vdb_text);
        tcc_add_symbol(s, "vdb_point", PushCommand_vdb_point);
        tcc_add_symbol(s, "vdb_line", PushCommand_vdb_line);
        tcc_add_symbol(s, "vdb_triangle", PushCommand_vdb_triangle);
        tcc_add_symbol(s, "vdb_triangle_filled", PushCommand_vdb_triangle_filled);
        tcc_add_symbol(s, "vdb_quad", PushCommand_vdb_quad);
        tcc_add_symbol(s, "vdb_quad_filled", PushCommand_vdb_quad_filled);
        tcc_add_symbol(s, "vdb_rect", PushCommand_vdb_rect);
        tcc_add_symbol(s, "vdb_rect_filled", PushCommand_vdb_rect_filled);
        tcc_add_symbol(s, "vdb_circle", PushCommand_vdb_circle);
        tcc_add_symbol(s, "vdb_circle_filled", PushCommand_vdb_circle_filled);
    }
    #else
    {
        tcc_add_symbol(s, "vdb_view", vdb_view);
        tcc_add_symbol(s, "vdb_color", vdb_color);
        tcc_add_symbol(s, "vdb_line_width", vdb_line_width);
        tcc_add_symbol(s, "vdb_point_size", vdb_point_size);
        tcc_add_symbol(s, "vdb_path_clear", vdb_path_clear);
        tcc_add_symbol(s, "vdb_path_to", vdb_path_to);
        tcc_add_symbol(s, "vdb_path_fill", vdb_path_fill);
        tcc_add_symbol(s, "vdb_path_stroke", vdb_path_stroke);
        tcc_add_symbol(s, "vdb_text", vdb_text);
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
    #endif

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

void ScriptUpdateAndDraw(frame_input_t input, bool reload)
{
    static script_loop_t ScriptLoop = NULL;
    if (reload)
    {
        ScriptLoop = LoadScript();
    }
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

    if (front_buffer)
    {
        front_buffer->Draw();
    }
}
