#pragma once

struct frame_input_t
{
    bool key_down[256];
    bool key_press[256];

    // Note: for retina displays screen coordinates != framebuffer coordinates
    int window_x,window_y; // This is the position of the window's client area in screen coordinates
    int window_w,window_h; // This is the size of the window's client area in screen coordinates
    int framebuffer_w,framebuffer_h; // This is the size in pixels of the framebuffer in the window

    float mouse_x,mouse_y; // The position of the mouse in the client area in screen coordinates where (0,0):top-left
    float mouse_u,mouse_v; // -||- in normalized mouse coordinates where (-1,-1):bottom-left (+1,+1):top-right
    float mouse_wheel;

    float elapsed_time;
    float frame_time; // Note: When recording video you probably want to use your own animation timer
                      // that increments at a fixed time step per loop. It is also possible that camera
                      // movement based on frame_time will explode...
    bool is_focused;
    bool lost_focus;
    bool regained_focus;
    bool framebuffer_size_changed;
};

// Global variable for convenience. It gets updated inside BeforeUpdateAndDraw.
extern frame_input_t frame_input;
