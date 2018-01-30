#pragma once
#include <stdint.h>
#include "script_input.h"
#include "3rdparty/tinycthread.h"
#include "command_buffer.h"

typedef void (*script_loop_t)(script_input_t);

static uint8_t script_data_buffer[1024*1024*10] = {0}; // todo: allocate at start of thread?
volatile uint32_t script_data_used = 0;

void script_push_bytes(uint8_t *data, uint32_t count)
{
    while (count > 0) // in case we couldn't write the entire data into the buffer at once
    {
        while (script_data_used > 0)
        {
            // wait until connection reader thread has processed existing data
        }
        uint32_t bytes_to_copy = count;
        if (bytes_to_copy > sizeof(script_data_buffer))
            bytes_to_copy = sizeof(script_data_buffer);
        memcpy(script_data_buffer, data, bytes_to_copy);
        count -= bytes_to_copy;
        data += bytes_to_copy;
        script_data_used = bytes_to_copy;
    }
}

// To be somewhat robust against non-deliberate communication
// over the TCP or serial channel, the StartOfFrame signal is
// not just a single 1 byte ID; instead it's the magic byte
// repeated 8 times.
// todo: document this decision for people who want to write
// an implementation of the transmission protocol for their
// platform.
int ScriptDataWelcomerThread(void *arg)
{
    bool frame_begun = false;
    while (true)
    {
        while (script_data_used == 0)
        {
            // wait until there is data to process
            thrd_yield();
        }

        uint8_t *copy_from = script_data_buffer;
        uint32_t copy_count = script_data_used;
        if (!frame_begun)
        {
            static uint32_t consecutive_new_frame_markers = 0;
            for (uint32_t i = 0; i < script_data_used; i++)
            {
                if (script_data_buffer[i] == (uint8_t)id_new_frame)
                {
                    consecutive_new_frame_markers++;
                    if (consecutive_new_frame_markers == required_consecutive_new_frame_markers)
                    {
                        copy_from = script_data_buffer + i;
                        copy_count = script_data_used - i;
                        frame_begun = true;
                        consecutive_new_frame_markers = 0;
                        LockCommandBuffer();
                        command_buffer.used = 0; // clear command buffer
                        ReleaseCommandBuffer();
                        break;
                    }
                }
                else
                {
                    consecutive_new_frame_markers = 0;
                }
            }
        }

        // just copy all data into the command buffer immediately
        // don't bother locking it
        if (frame_begun)
        {
            command_buffer_t *b = &command_buffer;
            if (b->used + copy_count > b->max_size)
            {
                copy_count = b->max_size - b->used;
                printf("Backbuffer filled up, truncating data. Some graphical glitches may occur.\n");
                // todo: log this error to on-screen console, warn user
            }
            memcpy(b->data + b->used, copy_from, copy_count);
            b->used += copy_count;
        }

        script_data_used = 0;
    }
    return 0;
}

void ScriptConnectionStart()
{
    thrd_t thread = {0};
    thrd_create(&thread, ScriptDataWelcomerThread, NULL);
}

void ScriptUpdateAndDraw(frame_input_t input, bool reload)
{

}


#if 0
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
void PushCommandString(const char *x, int n) {
    command_buffer_t *b = GetBackBuffer();
    assert(b && b->data && "Back buffer not allocated");
    if (n > 256)
        n = 256;
    assert(b->used + n + 1 <= b->max_size);
    PushCommandUint8(n);
    for (int i = 0; i < n; i++)
        *(char*)(b->data + b->used++) = x[i];
}
#define f(x) PushCommandFloat32(x)
#define u(id) PushCommandUint8(id)
void PushCommand_vdb_view(float left, float right, float bottom, float top) { u(id_view); f(left); f(right); f(bottom); f(top); }
void PushCommand_vdb_color(float r, float g, float b, float a) { u(id_color); f(r); f(g); f(b); f(a); }
void PushCommand_vdb_line_width(float pixel_width) { u(id_line_width); f(pixel_width); }
void PushCommand_vdb_point_size(float pixel_size) { u(id_point_size); f(pixel_size); }
void PushCommand_vdb_path_clear() { u(id_path_clear); }
void PushCommand_vdb_path_to(float x, float y) { u(id_path_to); f(x); f(y); }
void PushCommand_vdb_path_fill() { u(id_path_fill); }
void PushCommand_vdb_path_stroke() { u(id_path_stroke); }
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
void PushCommand_vdb_text(float x, float y, const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (w == -1 || w >= (int)sizeof(buffer))
        w = (int)sizeof(buffer) - 1;
    buffer[w] = 0;
    va_end(args);

    u(id_text); f(x); f(y); PushCommandString(buffer,w);
}
#undef f
#undef u

void vdb_new_frame() { GetBackBuffer()->used = 0; }
void vdb_render() { SwapCommandBuffers(); }

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
        tcc_add_symbol(s, "vdb_new_frame", vdb_new_frame);
        tcc_add_symbol(s, "vdb_render", vdb_render);

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
#endif
