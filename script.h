#ifndef SCRIPT_H
#define SCRIPT_H

void vdb_path_clear();
void vdb_path_line_to(float x, float y);
void vdb_path_fill_convex(float r, float g, float b, float a);
// void PushView();
// void PopView();
// void vdbColor(float r, float g, float b, float a);
// void vdbRectFilled();
// void vdbRect();
// void vdbCircle();
// void vdbCircleFilled();
// void vdbText();
// void vdbPoint(float x, float y);
// void vdbLine(float x1, float y1, float x2, float y2);
// void vdbImageU08(void *data, int width, int height, int channels);
// void vdbImageF32(void *data, int width, int height, int channels);
// void vdbNewFrame();
// void vdbEndFrame();


typedef struct
{
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
} script_input_t;

#endif
