#pragma once
#include <stdint.h>
#include "3rdparty/tinycthread.h"

enum command_id_t
{
    id_view = 0,
    id_color,
    id_line_width,
    id_point_size,
    id_path_clear,
    id_path_to,
    id_path_fill,
    id_path_stroke,
    id_text,
    id_point,
    id_line,
    id_triangle,
    id_triangle_filled,
    id_quad,
    id_quad_filled,
    id_rect,
    id_rect_filled,
    id_circle,
    id_circle_filled,
    id_new_frame = 254,
    id_end_frame = 255,
    id_count // todo: assert(id_count <= 256); // ensure id fits into uint8
};

struct command_buffer_t
{
    uint8_t *data; // points to pre-allocated array of bytes
    uint32_t used; // number of bytes available in 'data'
    uint32_t max_size; // allocation size of 'data'

    uint32_t read; // number of bytes that have been read

    float ReadFloat32() {
        assert((read + sizeof(float) <= used) && "Read past command buffer");
        // todo: replace with "ensure_data_available" and don't do command if so
        float x = *(float*)(data + read);
        read += sizeof(float);
        return x;
    }
    uint8_t ReadUint8() {
        assert((read + sizeof(uint8_t) <= used) && "Read past command buffer");
        uint8_t x = *(uint8_t*)(data + read);
        read += sizeof(uint8_t);
        return x;
    }
    char *ReadString(uint8_t length) {
        assert((read + length <= used) && "Read past command buffer");
        char *x = (char*)(data + read);
        read += length; return x;
    }

    void Draw()
    {
        assert(data && "Data in command buffer was null");
        read = 0;
        float left,right,bottom,top;
        float r,g,b,a;
        float x1,x2,x3,x4;
        float y1,y2,y3,y4;
        float x,y,w,h;
        int n;
        char *s;
        while (read < used)
        {
            #define F ReadFloat32(); // to make this part more readable
            uint8_t id = ReadUint8();
            if      (id == id_view)             { left=F;right=F;bottom=F;top=F;vdb_view(left,right,bottom,top); }
            else if (id == id_color)            { r=F;g=F;b=F;a=F;vdb_color(r,g,b,a); }
            else if (id == id_line_width)       { x=F;vdb_line_width(x); }
            else if (id == id_point_size)       { x=F;vdb_point_size(x); }
            else if (id == id_path_clear)       { vdb_path_clear(); }
            else if (id == id_path_to)          { x=F;y=F;vdb_path_to(x,y); }
            else if (id == id_path_fill)        { vdb_path_fill(); }
            else if (id == id_path_stroke)      { vdb_path_stroke(); }
            else if (id == id_text)             { x=F;y=F;n=ReadUint8();s=ReadString(n);vdb_text(x,y,s,n); }
            else if (id == id_point)            { x=F;y=F;vdb_point(x,y); }
            else if (id == id_line)             { x1=F;y1=F;x2=F;y2=F;vdb_line(x1,y1,x2,y2); }
            else if (id == id_triangle)         { x1=F;y1=F;x2=F;y2=F;x3=F;y3=F;vdb_triangle(x1,y1,x2,y2,x3,y3); }
            else if (id == id_triangle_filled)  { x1=F;y1=F;x2=F;y2=F;x3=F;y3=F;vdb_triangle_filled(x1,y1,x2,y2,x3,y3); }
            else if (id == id_quad)             { x1=F;y1=F;x2=F;y2=F;x3=F;y3=F;x4=F;y4=F;vdb_quad(x1,y1,x2,y2,x3,y3,x4,y4); }
            else if (id == id_quad_filled)      { x1=F;y1=F;x2=F;y2=F;x3=F;y3=F;x4=F;y4=F;vdb_quad_filled(x1,y1,x2,y2,x3,y3,x4,y4); }
            else if (id == id_rect)             { x=F;y=F;w=F;h=F;vdb_rect(x,y,w,h); }
            else if (id == id_rect_filled)      { x=F;y=F;w=F;h=F;vdb_rect_filled(x,y,w,h); }
            else if (id == id_circle)           { x=F;y=F;r=F;vdb_circle(x,y,r); }
            else if (id == id_circle_filled)    { x=F;y=F;r=F;vdb_circle_filled(x,y,r); }
            #undef F
        }
    }
};

union draw_cmd_t
{
    struct cmd_vdb_view_t { float left, right, bottom, top; void Draw() { vdb_view(left, right, bottom, top); } };
    struct cmd_vdb_color_t { float r, g, b, a; void Draw() { vdb_color(r, g, b, a); } };
    struct cmd_vdb_line_width_t { float pixel_width; void Draw() { vdb_line_width(pixel_width); } };
    struct cmd_vdb_point_size_t { float pixel_size; void Draw() { vdb_point_size(pixel_size); } };
    struct cmd_vdb_text_t { float x, y, const char *text; int n; void Draw() { vdb_text(x, y, text, n); } };
    struct cmd_vdb_point_t { float x, y; void Draw() { vdb_point(x, y); } };
    struct cmd_vdb_line_t { float x1, y1, x2, y2; void Draw() { vdb_line(x1, y1, x2, y2); } };
    struct cmd_vdb_triangle_t { float x1, y1, x2, y2, x3, y3; void Draw() { vdb_triangle(x1, y1, x2, y2, x3, y3); } };
    struct cmd_vdb_triangle_filled_t { float x1, y1, x2, y2, x3, y3; void Draw() { vdb_triangle_filled(x1, y1, x2, y2, x3, y3); } };
    struct cmd_vdb_quad_t { float x1, y1, x2, y2, x3, y3, x4, y4; void Draw() { vdb_quad(x1, y1, x2, y2, x3, y3, x4, y4); } };
    struct cmd_vdb_quad_filled_t { float x1, y1, x2, y2, x3, y3, x4, y4; void Draw() { vdb_quad_filled(x1, y1, x2, y2, x3, y3, x4, y4); } };
    struct cmd_vdb_rect_t { float x, y, w, h; void Draw() { vdb_rect(x, y, w, h); } };
    struct cmd_vdb_rect_filled_t { float x, y, w, h; void Draw() { vdb_rect_filled(x, y, w, h); } };
    struct cmd_vdb_circle_t { float x, y, r; void Draw() { vdb_circle(x, y, r); } };
    struct cmd_vdb_circle_filled_t { float x, y, r; void Draw() { vdb_circle_filled(x, y, r); } };
}

static command_buffer_t command_buffer1 = {0};
static command_buffer_t command_buffer2 = {0};

static command_buffer_t *back_buffer = NULL;
static command_buffer_t *front_buffer = NULL;

command_buffer_t *GetFrontBuffer() { return front_buffer; }
command_buffer_t *GetBackBuffer() { return back_buffer; }

mtx_t front_buffer_mutex = {0};
void LockFrontBuffer() { mtx_lock(&mutex_command_buffer); }
void ReleaseFrontBuffer() { mtx_unlock(&mutex_command_buffer); thrd_yield(); }

void SwapCommandBuffers()
{
    // two threads might want to use the front buffer:
    // 1) the main thread wants to draw it at 60 fps
    // 2) the connection thread wants to swap it with
    // the back buffer once it has finished receiving
    // data.
    LockFrontBuffer();

    command_buffer_t *temp = front_buffer;
    front_buffer = back_buffer;
    back_buffer = temp;
    ReleaseFrontBuffer();
}

bool AllocateCommandBuffers(uint32_t max_size)
{
    assert(!command_buffer1.data && "Buffer already allocated");
    assert(!command_buffer2.data && "Buffer already allocated");
    assert(id_count <= 256 && "Command IDs cannot exceed the size of a uint8 (256) for now");

    command_buffer1.data = (uint8_t*)malloc(max_size);
    if (!command_buffer1.data)
        return false;
    command_buffer1.used = 0;
    command_buffer1.max_size = max_size;

    command_buffer2.data = (uint8_t*)malloc(max_size);
    if (!command_buffer2.data)
        return false;
    command_buffer2.used = 0;
    command_buffer2.max_size = max_size;

    back_buffer = &command_buffer1;
    front_buffer = &command_buffer2;

    mtx_init(&mutex_command_buffer, mtx_plain);

    return true;
}
