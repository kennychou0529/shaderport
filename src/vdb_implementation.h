#pragma once
#include "3rdparty/imgui.h"
#include "frame_input.h"
#include "console.h"
#include "video.h"
#include "shader.h"
#include "colormap_inferno.h"
#include "texture_shader.h"

static int draw_string_id = 0;
static ImDrawList *user_draw_list = NULL;
static ImU32 vdb_current_color = IM_COL32(255,255,255,255);
static float vdb_current_line_width = 1.0f;
static float vdb_current_point_size = 1.0f;

static ImU32 vdb_current_text_background = 0;
static bool  vdb_current_text_shadow = true;
static float vdb_current_text_x_align = -0.5f;
static float vdb_current_text_y_align = -0.5f;
static float vdb_current_text_size = -1.0f;
static int vdb_current_text_font = 0;

// todo: optimize, store inverse width
struct vdb_viewport_t { float x,y,w,h; };
struct vdb_transform_t { float left,right,bottom,top; };
static vdb_viewport_t vdb_current_viewport = { 0 };
static vdb_transform_t vdb_current_transform = { 0 };

static int vdb_gl_transform_stack_index = 0;
static bool vdb_pop_clip_rect = false;

void vdbBeforeUpdateAndDraw(frame_input_t input)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoInputs|
        ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("##UserDrawWindow", NULL, flags);
    user_draw_list = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    draw_string_id = 0;

    vdb_current_color = IM_COL32(255,255,255,255);
    vdb_current_line_width = 1.0f;
    vdb_current_point_size = 1.0f;

    vdb_gl_transform_stack_index = 0;

    vdb_current_transform.left = -1.0f;
    vdb_current_transform.right = +1.0f;
    vdb_current_transform.bottom = -1.0f;
    vdb_current_transform.top = +1.0f;

    vdb_current_viewport.x = 0.0f;
    vdb_current_viewport.y = 0.0f;
    vdb_current_viewport.w = 1.0f;
    vdb_current_viewport.h = 1.0f;

    vdb_current_text_background = 0;
    vdb_current_text_shadow = true;
    vdb_current_text_x_align = -0.5f;
    vdb_current_text_y_align = -0.5f;

    vdb_current_text_size = -1.0f;
    vdb_current_text_font = 0;

    vdb_pop_clip_rect = false;
}

bool vdbAfterUpdateAndDraw(frame_input_t input)
{
    if (vdb_pop_clip_rect)
        user_draw_list->PopClipRect();
    if (vdb_gl_transform_stack_index != 0)
    {
        ConsoleMessage("Push/Pop transform pair not matched");
        return false;
    }
    return true;
}

// ImGui uses display coordinates for drawing, which is different from framebuffer
// coordinates, if DPI scaling is active: i.e. framebuffer resolution might be half
// of the size of the window on the display.
ImVec2 UserToDisplayCoordinates(float x, float y)
{
    // todo: optimize, store inverses
    float x_normalized = (x-vdb_current_transform.left)/(vdb_current_transform.right-vdb_current_transform.left);
    float y_normalized = (y-vdb_current_transform.top)/(vdb_current_transform.bottom-vdb_current_transform.top);

    float vx = vdb_current_viewport.x;
    float vy = vdb_current_viewport.y;
    float vw = vdb_current_viewport.w;
    float vh = vdb_current_viewport.h;

    int x_display = (int)(frame_input.window_w*(vx + x_normalized*vw));
    int y_display = (int)(frame_input.window_h*(vy + y_normalized*vh));
    return ImVec2((float)x_display, (float)y_display);
}

bool vdb_viewport(float x, float y, float w, float h)
{
    if (w < 0.0f)
    {
        x += w;
        w = -w;
    }
    if (h < 0.0f)
    {
        y += h;
        h = -h;
    }
    vdb_current_viewport.x = x;
    vdb_current_viewport.y = y;
    vdb_current_viewport.w = w;
    vdb_current_viewport.h = h;

    // add clipping rectangles for ImGui primitives
    {
        int min_x = (int)(x*frame_input.window_w - 1);
        int min_y = (int)(y*frame_input.window_h - 1);
        int max_x = (int)((x+w)*frame_input.window_w + 2);
        int max_y = (int)((y+h)*frame_input.window_h + 2);
        ImVec2 clip_rect_min = ImVec2((float)min_x,(float)min_y);
        ImVec2 clip_rect_max = ImVec2((float)max_x,(float)max_y);
        if (vdb_pop_clip_rect)
            user_draw_list->PopClipRect();
        user_draw_list->PushClipRect(clip_rect_min, clip_rect_max, true);
        vdb_pop_clip_rect = true;
    }

    // for opengl rendering
    {
        int fbw = frame_input.framebuffer_w;
        int fbh = frame_input.framebuffer_h;
        int gl_w = (int)(w*fbw);
        int gl_h = (int)(h*fbh);
        int gl_x = (int)(x*fbw);
        int gl_y = fbh - (int)(y*fbh) - gl_h;
        // todo: clamp?
        // if (gl_x < 0) gl_x = 0; if (gl_x > fbw) gl_x = fbw;
        // if (gl_y < 0) gl_y = 0; if (gl_y > fbh) gl_y = fbh;
        // if (gl_w > fbw) gl_w = fbw;
        // todo: use glScissor instead?
        glViewport(gl_x, gl_y, gl_w, gl_h);
    }

    if (x+w < 0.0f || x > 1.0f || y+h < 0.0f || y > 1.0f)
        return false;

    return true;
}

void vdb_transform(float left, float right, float bottom, float top)
{
    vdb_current_transform.left = left;
    vdb_current_transform.right = right;
    vdb_current_transform.bottom = bottom;
    vdb_current_transform.top = top;
}

void vdb_text_background(int r, int g, int b, int a) { vdb_current_text_background = IM_COL32(r,g,b,a); }
void vdb_text_no_background() { vdb_current_text_background = 0; }
void vdb_text_shadow(bool enabled) { vdb_current_text_shadow = enabled; }
void vdb_text_x_left()   { vdb_current_text_x_align =  0.0f; }
void vdb_text_x_center() { vdb_current_text_x_align = -0.5f; }
void vdb_text_x_right()  { vdb_current_text_x_align = -1.0f; }
void vdb_text_y_top()    { vdb_current_text_y_align =  0.0f; }
void vdb_text_y_center() { vdb_current_text_y_align = -0.5f; }
void vdb_text_y_bottom() { vdb_current_text_y_align = -1.0f; }
void vdb_text_size_absolute(float height_in_pixels) { vdb_current_text_size = height_in_pixels; }

void vdb_text_size(float ratio_of_viewport_height)
{
    vdb_current_text_size = ratio_of_viewport_height*vdb_current_viewport.h*frame_input.framebuffer_h;
}

void vdb_text(float x, float y, const char *text, int length)
{
    ImFont *font = GetFontImGuiHandleOrDefault(vdb_current_text_font);

    float font_size = (vdb_current_text_size < 0.0f) ? font->FontSize : vdb_current_text_size;
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, text, text+length);

    ImVec2 pos = UserToDisplayCoordinates(x,y);
    pos.x += text_size.x * vdb_current_text_x_align;
    pos.y += text_size.y * vdb_current_text_y_align;

    pos.x = (float)(int)(pos.x+0.5f);
    pos.y = (float)(int)(pos.y+0.5f);

    if (vdb_current_text_background)
    {
        const float xpad = 4.0f;
        const float ypad = 2.0f;
        ImVec2 a = ImVec2(pos.x-xpad,pos.y-ypad);
        ImVec2 b = ImVec2(pos.x+2.0f*xpad+text_size.x, pos.y+2.0f*ypad+text_size.y);
        user_draw_list->AddRectFilled(a, b, vdb_current_text_background, 8.0f);
    }

    // todo: reuse alpha from current color
    if (vdb_current_text_shadow)
        user_draw_list->AddText(font, font_size, ImVec2(pos.x+1,pos.y+1), IM_COL32(0,0,0,255), text, text+length);

    user_draw_list->AddText(font, font_size, pos, vdb_current_color, text, text+length);
}

void vdb_text_formatted(float x, float y, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (w == -1 || w >= (int)sizeof(buffer))
        w = (int)sizeof(buffer) - 1;
    buffer[w] = 0;
    vdb_text(x, y, buffer, w);
    va_end(args);
}

void vdb_path_clear()
{
    user_draw_list->PathClear();
}
void vdb_path_to(float x, float y)
{
    user_draw_list->PathLineTo(UserToDisplayCoordinates(x, y));
}
void vdb_path_fill()
{
    user_draw_list->PathFillConvex(vdb_current_color);
}
void vdb_path_stroke()
{
    user_draw_list->PathStroke(vdb_current_color, false, vdb_current_line_width);
}
void vdb_color(float r, float g, float b, float a)
{
    vdb_current_color = IM_COL32(r,g,b,a);
}
void vdb_line_width(float px)
{
    vdb_current_line_width = px;
}
void vdb_point_size(float px)
{
    vdb_current_point_size = px;
}
void vdb_point(float x, float y)
{
    ImVec2 c = UserToDisplayCoordinates(x, y);
    ImVec2 a = ImVec2(c.x - vdb_current_point_size, c.y - vdb_current_point_size);
    ImVec2 b = ImVec2(c.x + vdb_current_point_size, c.y + vdb_current_point_size);
    user_draw_list->AddRectFilled(a, b, vdb_current_color);
}
void vdb_line(float x1, float y1, float x2, float y2)
{
    ImVec2 a = UserToDisplayCoordinates(x1,y1);
    ImVec2 b = UserToDisplayCoordinates(x2,y2);
    user_draw_list->AddLine(a, b, vdb_current_color, vdb_current_line_width);
}
void vdb_triangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    ImVec2 a = UserToDisplayCoordinates(x1,y1);
    ImVec2 b = UserToDisplayCoordinates(x2,y2);
    ImVec2 c = UserToDisplayCoordinates(x3,y3);
    user_draw_list->AddTriangle(a, b, c, vdb_current_color, vdb_current_line_width);
}
void vdb_triangle_filled(float x1, float y1, float x2, float y2, float x3, float y3)
{
    ImVec2 a = UserToDisplayCoordinates(x1,y1);
    ImVec2 b = UserToDisplayCoordinates(x2,y2);
    ImVec2 c = UserToDisplayCoordinates(x3,y3);
    user_draw_list->AddTriangleFilled(a, b, c, vdb_current_color);
}
void vdb_quad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    ImVec2 a = UserToDisplayCoordinates(x1,y1);
    ImVec2 b = UserToDisplayCoordinates(x2,y2);
    ImVec2 c = UserToDisplayCoordinates(x3,y3);
    ImVec2 d = UserToDisplayCoordinates(x4,y4);
    user_draw_list->AddQuad(a, b, c, d, vdb_current_color, vdb_current_line_width);
}
void vdb_quad_filled(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    ImVec2 a = UserToDisplayCoordinates(x1,y1);
    ImVec2 b = UserToDisplayCoordinates(x2,y2);
    ImVec2 c = UserToDisplayCoordinates(x3,y3);
    ImVec2 d = UserToDisplayCoordinates(x4,y4);
    user_draw_list->AddQuadFilled(a, b, c, d, vdb_current_color);
}
void vdb_rect(float x, float y, float w, float h)
{
    ImVec2 a = UserToDisplayCoordinates(x,y);
    ImVec2 b = UserToDisplayCoordinates(x+w,y+h);
    user_draw_list->AddRect(a, b, vdb_current_color, 0.0f, 0, vdb_current_line_width);
}
void vdb_rect_filled(float x, float y, float w, float h)
{
    ImVec2 a = UserToDisplayCoordinates(x,y);
    ImVec2 b = UserToDisplayCoordinates(x+w,y+h);
    user_draw_list->AddRectFilled(a, b, vdb_current_color);
}
void vdb_circle(float x, float y, float r)
{
    ImVec2 pos = UserToDisplayCoordinates(x, y);
    float r_display = UserToDisplayCoordinates(x, y+r).y - pos.y;
    user_draw_list->AddCircle(pos, r_display, vdb_current_color, 12, vdb_current_line_width);
}
void vdb_circle_filled(float x, float y, float r)
{
    ImVec2 pos = UserToDisplayCoordinates(x, y);
    float r_display = UserToDisplayCoordinates(x, y+r).y - pos.y;
    user_draw_list->AddCircleFilled(pos, r_display, vdb_current_color, 12);
}

//
// These functions are currently only used in DLL scripts
//

int vdb_load_font(const char *filename, float size) { return GetFont(filename, size); }
void vdb_text_font(int font) { vdb_current_text_font = font; }

// ImGui wrappers
void vdb_gui_begin(const char *label) { ImGui::Begin(label); }
void vdb_gui_begin_no_title(const char *label) { ImGui::Begin(label, NULL, ImGuiWindowFlags_NoTitleBar); }
void vdb_gui_end() { ImGui::End(); }
bool vdb_gui_slider1f(const char* label, float* v, float v_min, float v_max) { return ImGui::SliderFloat(label, v, v_min, v_max); }
bool vdb_gui_slider1i(const char* label, int* v, int v_min, int v_max) { return ImGui::SliderInt(label, v, v_min, v_max); }
bool vdb_gui_button(const char *label) { return ImGui::Button(label); }
bool vdb_gui_checkbox(const char *label, bool *v) { return ImGui::Checkbox(label, v); }
bool vdb_gui_radio(const char *label, int *v, int v_button) { return ImGui::RadioButton(label, v, v_button); }
// Keyboard, mouse, screenshots, ...
bool vdb_io_key_down(char key) { return frame_input.key_down[toupper(key)]; }
bool vdb_io_key_press(char key) { return frame_input.key_press[toupper(key)]; }
bool vdb_io_mouse_down(int button)  { return ImGui::IsMouseDown(button); }
bool vdb_io_mouse_click(int button) { return ImGui::IsMouseClicked(button); }

void DrawTextureFancy(GLuint texture, float user_x, float user_y, float user_w, float user_h, bool mono=false, float *selector=NULL, float range_min=0.0f, float range_max=1.0f, float *gain=NULL, float *bias=NULL)
{
    static bool loaded = false;
    static GLuint program = 0;
    static GLuint colormap = 0;
    static GLint attrib_in_position = 0;
    static GLint uniform_gain = 0;
    static GLint uniform_bias = 0;
    static GLint uniform_selector = 0;
    static GLint uniform_blend = 0;
    static GLint uniform_range_min = 0;
    static GLint uniform_range_max = 0;
    static GLint uniform_channel0 = 0;
    static GLint uniform_channel1 = 0;
    static GLint uniform_projection = 0;
    static GLuint quad_buffer = 0;
    if (!loaded)
    {
        colormap = TexImage2D(colormap_inferno, colormap_inferno_length, 1, GL_RGB, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);
        program = LoadShaderFromMemory(texture_shader_vs, texture_shader_fs);
        attrib_in_position = glGetAttribLocation(program, "in_position");
        uniform_projection = glGetUniformLocation(program, "projection");
        uniform_channel0 = glGetUniformLocation(program, "channel0");
        uniform_channel1 = glGetUniformLocation(program, "channel1");
        uniform_gain = glGetUniformLocation(program, "gain");
        uniform_bias = glGetUniformLocation(program, "bias");
        uniform_selector = glGetUniformLocation(program, "selector");
        uniform_blend = glGetUniformLocation(program, "blend");
        uniform_range_min = glGetUniformLocation(program, "range_min");
        uniform_range_max = glGetUniformLocation(program, "range_max");
        loaded = true;
    }

    glUseProgram(program);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colormap);
    glUniform1i(uniform_channel0, 0);
    glUniform1i(uniform_channel1, 1);

    {
        float r = vdb_current_transform.right;
        float l = vdb_current_transform.left;
        float t = vdb_current_transform.top;
        float b = vdb_current_transform.bottom;
        GLfloat projection[4*4] = {
            2.0f*user_w/(r-l), 0.0f, 0.0f, -1.0f+2.0f*(user_x-l)/(r-l),
            0.0f, 2.0f*user_h/(t-b), 0.0f, -1.0f+2.0f*(user_y-b)/(t-b),
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        glUniformMatrix4fv(uniform_projection, 1, GL_TRUE, projection);
    }

    if (mono) glUniform1f(uniform_blend, 1.0f);
    else glUniform1f(uniform_blend, 0.0f);

    if (mono && selector) glUniform4fv(uniform_selector, 1, selector);
    else if (mono) glUniform4f(uniform_selector, 2.0f/6.0f, 1.0f/6.0f, 3.0f/6.0f, 0.0f);
    else glUniform4f(uniform_selector, 0.0f, 0.0f, 0.0f, 0.0f);

    if (gain) glUniform4fv(uniform_gain, 1, gain);
    else glUniform4f(uniform_gain, 1.0f, 1.0f, 1.0f, 1.0f);

    if (bias) glUniform4fv(uniform_bias, 1, bias);
    else glUniform4f(uniform_bias, 0.0f, 0.0f, 0.0f, 0.0f);

    glUniform1f(uniform_range_min, range_min);
    glUniform1f(uniform_range_max, range_max);

    static const float position[] = { 0,0, 1,0, 1,1, 1,1, 0,1, 0,0 };
    glVertexAttribPointer(attrib_in_position, 2, GL_FLOAT, GL_FALSE, 0, position);
    glEnableVertexAttribArray(attrib_in_position);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(attrib_in_position);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glUseProgram(0);
}
void vdb_load_image_u08(int slot, const void *data, int width, int height, int components)
{
    if (components == 1)      SetTexture(slot, data, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else if (components == 2) SetTexture(slot, data, width, height, GL_RG, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else if (components == 3) SetTexture(slot, data, width, height, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else if (components == 4) SetTexture(slot, data, width, height, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else                      ConsoleMessage("'components' must be 1,2,3 or 4");
}
void vdb_load_image_f32(int slot, const void *data, int width, int height, int components)
{
    if (components == 1)      SetTexture(slot, data, width, height, GL_LUMINANCE, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else if (components == 2) SetTexture(slot, data, width, height, GL_RG, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else if (components == 3) SetTexture(slot, data, width, height, GL_RGB, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else if (components == 4) SetTexture(slot, data, width, height, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else                      ConsoleMessage("'components' must be 1,2,3 or 4");
}
void vdb_load_image_file(int slot, const char *filename, int *width, int *height, int *components)
{
    unsigned char *data = stbi_load(filename, width, height, components, 4);
    if (data)
    {
        SetTexture(slot, data, *width, *height, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
        free(data);
    }
    else
    {
        ConsoleMessage("Failed to load image %s", filename);
    }
}
void vdb_draw_image(int slot, float x, float y, float w, float h) { DrawTextureFancy(GetTextureSlotHandle(slot), x, y, w, h); }
void vdb_draw_image_mono(int slot, float x, float y, float w, float h,
                         float r, float g, float b, float a, float range_min, float range_max)
{
    float selector[4] = { r, g, b, a };
    DrawTextureFancy(GetTextureSlotHandle(slot), x, y, w, h, true, selector, range_min, range_max);
}

int vdb_load_video(const char *filename, int width, int height) { return LoadVideo(filename, width, height); }
void vdb_draw_video(int video, int frame, float x, float y, float w, float h) { DrawTextureFancy(GetAndBindVideoFrameTexture(video, frame), x, y, w, h); }

//
// OpenGL wrapper
// todo: GLES compatibility to run on tablets
// todo: fixed pipeline deprecation
//

void vdb_gl_clear_color(float r, float g, float b, float a) { glClearColor(r,g,b,a); glClear(GL_COLOR_BUFFER_BIT); }
void vdb_gl_clear_depth(float depth) { glClearDepth(depth); glClear(GL_DEPTH_BUFFER_BIT); }
void vdb_gl_point_size(float size) { glPointSize(size); }
void vdb_gl_line_width(float width) { glLineWidth(width); }

void vdb_gl_projection(float *v)
{
    // todo: gl deprecation
    glMatrixMode(GL_PROJECTION);
    if (v) glLoadMatrixf(v);
    else glLoadIdentity();
}

void vdb_gl_push_transform(float *v)
{
    vdb_gl_transform_stack_index++;
    if (v)
    {
        // todo: gl deprecation
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(v);
    }
}

void vdb_gl_pop_transform()
{
    vdb_gl_transform_stack_index--;
    if (vdb_gl_transform_stack_index >= 0)
    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
    else
    {
        // error is reported at the end of frame
    }
}

void vdb_gl_blend_alpha()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}

void vdb_gl_blend_additive()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
}

void vdb_gl_blend_disable()
{
    glDisable(GL_BLEND);
}

void vdb_gl_depth_test(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}
void vdb_gl_color(float r, float g, float b, float a) { glColor4f(r,g,b,a); }
void vdb_gl_vertex(float x, float y, float z) { glVertex3f(x,y,z); }
void vdb_gl_texel(float u, float v) { glTexCoord2f(u,v); }
void vdb_gl_texture(bool enabled)
{
    if (enabled) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, 0); }
    else { glDisable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, 0); }
}
void vdb_gl_bind_texture(int slot)
{
    glBindTexture(GL_TEXTURE_2D, GetTextureSlotHandle(slot));
}
void vdb_gl_begin_lines() { glBegin(GL_LINES); }
void vdb_gl_begin_triangles() { glBegin(GL_TRIANGLES); }
void vdb_gl_begin_points() { glBegin(GL_POINTS); }
void vdb_gl_end() { glEnd(); }

struct gl_point_buffer_t
{
    GLuint position;
    GLuint color;
    int num_points;
    bool allocated;
};
#define max_point_buffers 1024
static gl_point_buffer_t point_buffers[max_point_buffers];
void vdb_gl_load_points(int slot, void *position, void *color, int num_points)
{
    if (slot < 0 || slot >= max_point_buffers)
    {
        ConsoleMessage("You are trying to set point buffer slot %d. You can only assign data from slot 0 up to %d.", slot, max_point_buffers-1);
        return;
    }

    gl_point_buffer_t *buffer = &point_buffers[slot];
    buffer->num_points = num_points;
    if (!buffer->allocated)
    {
        buffer->allocated = true;
        glGenBuffers(1, &buffer->position);
        glGenBuffers(1, &buffer->color);
    }
    // todo: buffersubdata if already allocated?
    glBindBuffer(GL_ARRAY_BUFFER, buffer->position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*num_points, position, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->color);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*num_points, color, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
#define POINT_SHADER_QUAD 0
#if POINT_SHADER_QUAD==1
#include "point_shader_quad.h"
#else
#include "point_shader.h"
#endif
void vdb_gl_draw_points(int slot, float point_size, int circle_segments)
{
    typedef void (*vertex_attrib_divisor_t)(GLuint, GLuint);
    static vertex_attrib_divisor_t VertexAttribDivisor = (vertex_attrib_divisor_t)glfwGetProcAddress("glVertexAttribDivisor");
    if (!VertexAttribDivisor)
        return; // todo: error

    // todo: gl deprecation, replace with storing own matrix stack?
    float projection[4*4];
    float model_to_view[4*4];
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, model_to_view);

    static bool shader_loaded = false;
    static GLuint program = 0;
    static GLint attrib_in_position = 0;
    static GLint attrib_in_color = 0;
    static GLint attrib_instance_position = 0;
    static GLint attrib_instance_color = 0;
    // static GLint uniform_reflection = 0;
    static GLint uniform_point_size = 0;
    static GLint uniform_projection = 0;
    static GLint uniform_model_to_view = 0;
    if (!shader_loaded)
    {
        program = LoadShaderFromMemory(point_shader_vs, point_shader_fs);
        attrib_in_position = glGetAttribLocation(program, "in_position");
        attrib_in_color = glGetAttribLocation(program, "in_color");
        attrib_instance_position = glGetAttribLocation(program, "instance_position");
        attrib_instance_color = glGetAttribLocation(program, "instance_color");
        // uniform_reflection = glGetUniformLocation(program, "reflection");
        uniform_projection = glGetUniformLocation(program, "projection");
        uniform_model_to_view = glGetUniformLocation(program, "model_to_view");
        uniform_point_size = glGetUniformLocation(program, "point_size");
        shader_loaded = true;
    }

    // todo: do we need a vertex array object...?

    glUseProgram(program);
    glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, projection);
    glUniformMatrix4fv(uniform_model_to_view, 1, GL_FALSE, model_to_view);
    glUniform1f(uniform_point_size, point_size);

    #if POINT_SHADER_QUAD==1
    static const float quad_position[] = { -1,-1, 1,-1, 1,1, 1,1, -1,1, -1,-1 };
    static const float quad_color[] = { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 };
    glVertexAttribPointer(attrib_in_position, 2, GL_FLOAT, GL_FALSE, 0, quad_position);
    glEnableVertexAttribArray(attrib_in_position);
    glVertexAttribPointer(attrib_in_color, 4, GL_FLOAT, GL_FALSE, 0, quad_color);
    glEnableVertexAttribArray(attrib_in_color);
    #else
    const int max_circle_segments = 128;
    if (circle_segments > max_circle_segments)
        circle_segments = max_circle_segments;
    static float circle[max_circle_segments*3*2];
    for (int i = 0; i < circle_segments; i++)
    {
        float t1 = 2.0f*3.1415926f*(0.125f + i/(float)(circle_segments));
        float t2 = 2.0f*3.1415926f*(0.125f + (i+1)/(float)(circle_segments));
        circle[6*i+0] = 0.0f;
        circle[6*i+1] = 0.0f;
        circle[6*i+2] = cosf(t1);
        circle[6*i+3] = sinf(t1);
        circle[6*i+4] = cosf(t2);
        circle[6*i+5] = sinf(t2);
    }
    glVertexAttribPointer(attrib_in_position, 2, GL_FLOAT, GL_FALSE, 0, circle);
    glEnableVertexAttribArray(attrib_in_position);
    #endif

    gl_point_buffer_t buffer = point_buffers[slot];
    glBindBuffer(GL_ARRAY_BUFFER, buffer.position);
    glEnableVertexAttribArray(attrib_instance_position);
    glVertexAttribPointer(attrib_instance_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    VertexAttribDivisor(attrib_instance_position, 1);

    glBindBuffer(GL_ARRAY_BUFFER, buffer.color);
    glEnableVertexAttribArray(attrib_instance_color);
    glVertexAttribPointer(attrib_instance_color, 4, GL_FLOAT, GL_FALSE, 0, 0);
    VertexAttribDivisor(attrib_instance_color, 1);

    glDrawArraysInstanced(GL_TRIANGLES, 0, circle_segments*3, buffer.num_points);
    glDisableVertexAttribArray(attrib_in_position);
    glDisableVertexAttribArray(attrib_instance_position);
    glDisableVertexAttribArray(attrib_instance_color);
    VertexAttribDivisor(attrib_instance_position, 0);
    VertexAttribDivisor(attrib_instance_color, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
