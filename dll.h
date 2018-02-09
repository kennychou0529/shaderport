#pragma once

struct io_t
{
    // todo: void (*screenshot)();
    // todo: void (*framegrab)();

    bool (*key_down)(char key);
    bool (*key_press)(char key);
    bool (*mouse_down)(int button);
    bool (*mouse_click)(int button);

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
    int lost_focus;
    int regained_focus;

    int recording_video;
    // todo: int video_frame_index;
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

    void (*text_background)(int r, int g, int b, int a);
    void (*text_no_background)();
    void (*text_shadow)(bool enabled);
    void (*text_x_left)();
    void (*text_x_center)();
    void (*text_x_right)();
    void (*text_y_top)();
    void (*text_y_center)();
    void (*text_y_bottom)();

    int (*load_image_file)(const char *filename, int *width, int *height, int *components);
    int (*load_image_u08)(const void *data, int width, int height, int components);
    int (*load_image_f32)(const void *data, int width, int height, int components);
    void (*image)(int handle);

    // enum draw_image_mode_t
    // {
    //     mode_rgb = 0,
    //     mode_gray = 1,
    //     mode_inferno = 2
    // };
    // void (*draw_image)(int handle, draw_image_mode_t mode, float *selector=NULL, float range_min=0.0f, float range_max=0.0f, float *gain=NULL, float *bias=NULL);
};

struct gui_t
{
    void (*begin)(const char *label);
    void (*begin_no_title)(const char *label);
    void (*end)();
    bool (*slider1f)(const char* label, float* v, float v_min, float v_max);
    bool (*slider1i)(const char* label, int* v, int v_min, int v_max);
    bool (*button)(const char *label);
    bool (*checkbox)(const char *label, bool *v);
    bool (*radio)(const char *label, int *v, int v_button);
};
