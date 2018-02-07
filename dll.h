#pragma once

struct io_t
{
    // todo: void (*screenshot)();
    // todo: void (*framegrab)();

    // todo: key presses

    // Note: for retina displays screen coordinates != framebuffer coordinates
    int window_x,window_y; // This is the position of the window's client area in screen coordinates
    int window_w,window_h; // This is the size of the window's client area in screen coordinates
    int framebuffer_w,framebuffer_h; // This is the size in pixels of the framebuffer in the window

    float mouse_x,mouse_y; // The position of the mouse in the client area in screen coordinates where (0,0):top-left
    float mouse_u,mouse_v; // -||- in normalized mouse coordinates where (-1,-1):bottom-left (+1,+1):top-right

    float elapsed_time;
    float frame_time; // Note: When recording video you probably want to use your own animation timer
                      // that increments at a fixed time step per loop. It is also possible that camera
                      // movement based on frame_time will explode...
    int recording_video;
    int lost_focus;
    int regained_focus;
};

struct draw_t
{
    void (*path_clear)();
    void (*path_to)(float x, float y);
    void (*path_fill)();
    void (*path_stroke)();
    void (*view)(float left, float right, float bottom, float top);
    void (*color)(float r, float g, float b, float a);
    void (*line_width)(float pixel_width);
    void (*point_size)(float pixel_size);
    void (*text)(float x, float y, const char *fmt, ...);
    void (*point)(float x, float y);
    void (*line)(float x1, float y1, float x2, float y2);
    void (*triangle)(float x1, float y1, float x2, float y2, float x3, float y3);
    void (*triangle_filled)(float x1, float y1, float x2, float y2, float x3, float y3);
    void (*quad)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    void (*quad_filled)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    void (*rect)(float x, float y, float w, float h);
    void (*rect_filled)(float x, float y, float w, float h);
    void (*circle)(float x, float y, float r);
    void (*circle_filled)(float x, float y, float r);
};
