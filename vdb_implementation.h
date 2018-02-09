#pragma once
#include "3rdparty/imgui.h"
#include "frame_input.h"
#include "console.h"

static int draw_string_id = 0;
static ImDrawList *user_draw_list = NULL;
static ImU32 vdb_current_color = IM_COL32(255,255,255,255);
static float vdb_current_line_width = 1.0f;
static float vdb_current_point_size = 1.0f;

static ImU32 vdb_current_text_background = 0;
static bool  vdb_current_text_shadow = true;
static float vdb_current_text_x_align = -0.5f;
static float vdb_current_text_y_align = -0.5f;

// todo: optimize, store inverse width
struct vdb_view_t { float left,right,bottom,top; };
vdb_view_t vdb_current_view = { 0 };

void vdbBeforeUpdateAndDraw(frame_input_t input)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("##UserDrawWindow", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings);
    user_draw_list = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    draw_string_id = 0;

    vdb_current_color = IM_COL32(255,255,255,255);
    vdb_current_line_width = 1.0f;
    vdb_current_point_size = 1.0f;

    vdb_current_view.left = -1.0f;
    vdb_current_view.right = +1.0f;
    vdb_current_view.bottom = -1.0f;
    vdb_current_view.top = +1.0f;

    vdb_current_text_background = 0;
    vdb_current_text_shadow = true;
    vdb_current_text_x_align = -0.5f;
    vdb_current_text_y_align = -0.5f;
}

ImVec2 ConvertCoordinates(float x, float y)
{
    float x_normalized = (x-vdb_current_view.left)/(vdb_current_view.right-vdb_current_view.left);
    float y_normalized = (y-vdb_current_view.top)/(vdb_current_view.bottom-vdb_current_view.top);
    float x_display = frame_input.window_w*x_normalized;
    float y_display = frame_input.window_h*y_normalized;
    return ImVec2(x_display, y_display);
}

void vdb_view(float left, float right, float bottom, float top)
{
    vdb_current_view.left = left;
    vdb_current_view.right = right;
    vdb_current_view.bottom = bottom;
    vdb_current_view.top = top;
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

// todo: are x,y screen or framebuffer coordinates?
void vdb_text(float x, float y, const char *text, int length)
{
    ImVec2 pos = ConvertCoordinates(x,y);
    ImVec2 text_size = ImGui::CalcTextSize(text);
    pos.x += text_size.x * vdb_current_text_x_align;
    pos.y += text_size.y * vdb_current_text_y_align;
    if (vdb_current_text_background)
    {
        const float xpad = 4.0f;
        const float ypad = 2.0f;
        ImVec2 a = ImVec2(pos.x-xpad,pos.y-ypad);
        ImVec2 b = ImVec2(pos.x+2.0f*xpad+text_size.x, pos.y+2.0f*ypad+text_size.y);
        user_draw_list->AddRectFilled(a, b, vdb_current_text_background, 8.0f);
    }
    if (vdb_current_text_shadow)
        user_draw_list->AddText(ImVec2(pos.x+1,pos.y+1), IM_COL32(0,0,0,255), text, text+length);
    user_draw_list->AddText(pos, vdb_current_color, text, text+length);
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
    user_draw_list->PathLineTo(ConvertCoordinates(x, y));
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
    ImVec2 c = ConvertCoordinates(x, y);
    ImVec2 a = ImVec2(c.x - vdb_current_point_size, c.y - vdb_current_point_size);
    ImVec2 b = ImVec2(c.x + vdb_current_point_size, c.y + vdb_current_point_size);
    user_draw_list->AddRectFilled(a, b, vdb_current_color);
}
void vdb_line(float x1, float y1, float x2, float y2)
{
    ImVec2 a = ConvertCoordinates(x1,y1);
    ImVec2 b = ConvertCoordinates(x2,y2);
    user_draw_list->AddLine(a, b, vdb_current_color, vdb_current_line_width);
}
void vdb_triangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    ImVec2 a = ConvertCoordinates(x1,y1);
    ImVec2 b = ConvertCoordinates(x2,y2);
    ImVec2 c = ConvertCoordinates(x3,y3);
    user_draw_list->AddTriangle(a, b, c, vdb_current_color, vdb_current_line_width);
}
void vdb_triangle_filled(float x1, float y1, float x2, float y2, float x3, float y3)
{
    ImVec2 a = ConvertCoordinates(x1,y1);
    ImVec2 b = ConvertCoordinates(x2,y2);
    ImVec2 c = ConvertCoordinates(x3,y3);
    user_draw_list->AddTriangleFilled(a, b, c, vdb_current_color);
}
void vdb_quad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    ImVec2 a = ConvertCoordinates(x1,y1);
    ImVec2 b = ConvertCoordinates(x2,y2);
    ImVec2 c = ConvertCoordinates(x3,y3);
    ImVec2 d = ConvertCoordinates(x4,y4);
    user_draw_list->AddQuad(a, b, c, d, vdb_current_color, vdb_current_line_width);
}
void vdb_quad_filled(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    ImVec2 a = ConvertCoordinates(x1,y1);
    ImVec2 b = ConvertCoordinates(x2,y2);
    ImVec2 c = ConvertCoordinates(x3,y3);
    ImVec2 d = ConvertCoordinates(x4,y4);
    user_draw_list->AddQuadFilled(a, b, c, d, vdb_current_color);
}
void vdb_rect(float x, float y, float w, float h)
{
    ImVec2 a = ConvertCoordinates(x,y);
    ImVec2 b = ConvertCoordinates(x+w,y+h);
    user_draw_list->AddRect(a, b, vdb_current_color, 0.0f, 0, vdb_current_line_width);
}
void vdb_rect_filled(float x, float y, float w, float h)
{
    ImVec2 a = ConvertCoordinates(x,y);
    ImVec2 b = ConvertCoordinates(x+w,y+h);
    user_draw_list->AddRectFilled(a, b, vdb_current_color);
}
void vdb_circle(float x, float y, float r)
{
    float r_display = r/(vdb_current_view.right-vdb_current_view.left); // could also do y-axis... ideally we do an ellipse?
    user_draw_list->AddCircle(ConvertCoordinates(x, y), r_display, vdb_current_color, 12, vdb_current_line_width);
}
void vdb_circle_filled(float x, float y, float r)
{
    float r_display = r/(vdb_current_view.right-vdb_current_view.left); // could also do y-axis... ideally we do an ellipse?
    user_draw_list->AddCircleFilled(ConvertCoordinates(x, y), r_display, vdb_current_color, 12);
}

//
// These functions are currently only used in DLL scripts
//

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

// Additional draw functions

#include "shader.h"
#include "colormap_inferno.h"
#include "texture_shader.h"
void DrawTextureFancy(GLuint texture, bool mono=false, float *selector=NULL, float range_min=0.0f, float range_max=1.0f, float *gain=NULL, float *bias=NULL)
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
    if (!loaded)
    {
        colormap = TexImage2D(colormap_inferno, colormap_inferno_length, 1, GL_RGB, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);
        program = LoadShaderFromMemory(texture_shader_vs, texture_shader_fs);
        attrib_in_position = glGetAttribLocation(program, "in_position");
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

    glVertexAttrib2f(attrib_in_position, -1.0f, -1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, -1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, +1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, -1.0f, +1.0f);
    glVertexAttrib2f(attrib_in_position, -1.0f, -1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glUseProgram(0);
}
int vdb_load_image_u08(const void *data, int width, int height, int components)
{
    if (components == 1)      return TexImage2D(data, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else if (components == 2) return TexImage2D(data, width, height, GL_RG, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else if (components == 3) return TexImage2D(data, width, height, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else if (components == 4) return TexImage2D(data, width, height, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    else { ConsoleMessage("'components' must be 1,2,3 or 4"); return 0; }
}
int vdb_load_image_f32(const void *data, int width, int height, int components)
{
    if (components == 1)      return TexImage2D(data, width, height, GL_LUMINANCE, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else if (components == 2) return TexImage2D(data, width, height, GL_RG, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else if (components == 3) return TexImage2D(data, width, height, GL_RGB, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else if (components == 4) return TexImage2D(data, width, height, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST);
    else { ConsoleMessage("'components' must be 1,2,3 or 4"); return 0; }
}
int vdb_load_image_file(const char *filename, int *width, int *height, int *components)
{
    unsigned char *data = stbi_load(filename, width, height, components, 3);
    if (!data)
    {
        ConsoleMessage("Failed to load image %s", filename);
        return 0;
    }
    int handle = TexImage2D(data, *width, *height, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    free(data);
    return handle;
}
void vdb_draw_image(int handle) { DrawTextureFancy(handle); }
void vdb_draw_image_mono(int handle, float r, float g, float b, float a, float range_min, float range_max)
{
    float selector[4] = { r, g, b, a };
    DrawTextureFancy(handle, true, selector, range_min, range_max);
}
