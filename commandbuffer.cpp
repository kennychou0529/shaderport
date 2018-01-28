#include <stdint.h>

enum command_id_t
{
    id_view,
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
    id_count // todo: assert(id_count <= 256); // ensure id fits into uint8
};

struct command_buffer_t
{
    uint8_t *data;
    uint32_t size;
    uint32_t max_size;
};

struct command_buffer_parser_t
{
    uint8_t *data; // points to pre-allocated array of bytes
    uint32_t size; // number of bytes available in data
    uint32_t read; // number of bytes read in data

    float ReadFloat32() {
        assert((read + sizeof(float) <= size) && "Read past command buffer");
        float x = *(float*)(data + read);
        read += sizeof(float);
        return x;
    }
    uint8_t ReadUint8() {
        assert((read + sizeof(uint8_t) <= size) && "Read past command buffer");
        uint8_t x = *(uint8_t*)(data + read);
        read += sizeof(uint8_t);
        return x;
    }
    char *ReadString(uint8_t length) {
        assert((read + length <= size) && "Read past command buffer");
        char *x = (char*)(data + read);
        read += length; return x;
    }

    void Draw()
    {
        assert(data && "Data in command buffer was null");
        read = 0;
        while (read < size)
        {
            #define f ReadFloat32() // to make this part more readable
            uint8_t id = ReadUint8();
            if      (id == id_view)             vdb_view(f,f,f,f);
            else if (id == id_color)            vdb_color(f,f,f,f);
            else if (id == id_line_width)       vdb_line_width(f);
            else if (id == id_point_size)       vdb_point_size(f);
            else if (id == id_path_clear)       vdb_path_clear();
            else if (id == id_path_to)          vdb_path_to(f,f);
            else if (id == id_path_fill)        vdb_path_fill();
            else if (id == id_path_stroke)      vdb_path_stroke();
            else if (id == id_text)             vdb_text(f,f, ReadString(ReadUint8()));
            else if (id == id_point)            vdb_point(f,f);
            else if (id == id_line)             vdb_line(f,f,f,f);
            else if (id == id_triangle)         vdb_triangle(f,f,f,f,f,f);
            else if (id == id_triangle_filled)  vdb_triangle_filled(f,f,f,f,f,f);
            else if (id == id_quad)             vdb_quad(f,f,f,f,f,f,f,f);
            else if (id == id_quad_filled)      vdb_quad_filled(f,f,f,f,f,f,f,f);
            else if (id == id_rect)             vdb_rect(f,f,f,f);
            else if (id == id_rect_filled)      vdb_rect_filled(f,f,f,f);
            else if (id == id_circle)           vdb_circle(f,f,f);
            else if (id == id_circle_filled)    vdb_circle_filled(f,f,f);
            #undef f
        }
    }
};

static command_buffer_t command_buffer1 = {0};
static command_buffer_t command_buffer2 = {0};

static command_buffer_t *back_buffer = NULL;
static command_buffer_t *front_buffer = NULL;

command_buffer_t *GetFrontBuffer() { return front_buffer; }
command_buffer_t *GetBackBuffer() { return back_buffer; }

void SwapCommandBuffers()
{
    command_buffer_t *temp = front_buffer;
    front_buffer = back_buffer;
    back_buffer = temp;
}

bool AllocateCommandBuffers(uint32_t max_size)
{
    assert(!command_buffer1.data && "Buffer already allocated");
    assert(!command_buffer2.data && "Buffer already allocated");

    command_buffer1.data = (uint8_t*)malloc(max_size);
    if (!command_buffer1.data)
        return false;
    command_buffer1.size = 0;
    command_buffer1.max_size = max_size;

    command_buffer2.data = (uint8_t*)malloc(max_size);
    if (!command_buffer2.data)
        return false;
    command_buffer2.size = 0;
    command_buffer2.max_size = max_size;

    back_buffer = &command_buffer1;
    front_buffer = &command_buffer2;

    return true;
}


//
// Dummy stuff
// todo: remove
//

void PushCommandUint8(uint8_t x)
{
    assert(back_buffer && back_buffer->data && "Back buffer not allocated");
    assert(back_buffer->size + sizeof(uint8_t) <= back_buffer->max_size);
    *(uint8_t*)back_buffer->data[back_buffer->size] = x;
    back_buffer->size += sizeof(uint8_t);
}
void PushCommandFloat32(float x) {
    assert(back_buffer && back_buffer->data && "Back buffer not allocated");
    assert(back_buffer->size + sizeof(float) <= back_buffer->max_size);
    *(float*)back_buffer->data[back_buffer->size] = x;
    back_buffer->size += sizeof(float);
}
void PushCommandString(const char *x) {
    assert(back_buffer && back_buffer->data && "Back buffer not allocated");
    while (*x) {
        assert(back_buffer->size + 1 <= back_buffer->max_size);
        *(char*)back_buffer->data[back_buffer->size++] = *x;
        x++;
    }
}
#define f(x) PushCommandFloat32(x)
#define u(id) PushCommandUint8(id)
void PushCommand_vdb_new_frame() { PushCommandString("new frame"); }
void PushCommand_vdb_render() { PushCommandString("end frame"); }
void PushCommand_vdb_view(float left, float right, float bottom, float top) { u(id_view); f(left); f(right); f(bottom); f(top); }
void PushCommand_vdb_color(float r, float g, float b, float a) { u(id_color); f(r); f(g); f(b); f(a); }
void PushCommand_vdb_line_width(float pixel_width) { u(id_line_width); f(pixel_width); }
void PushCommand_vdb_point_size(float pixel_size) { u(id_point_size); f(pixel_size); }
void PushCommand_vdb_path_clear() { u(id_path_clear); }
void PushCommand_vdb_path_to(float x, float y) { u(id_path_to); f(x); f(y); }
void PushCommand_vdb_path_fill() { u(id_path_fill); }
void PushCommand_vdb_path_stroke() { u(id_path_stroke); }
void PushCommand_vdb_text(float x, float y, const char *text) { u(id_text); f(x); f(y); PushCommandString(text); }
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
