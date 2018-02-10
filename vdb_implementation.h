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
static float vdb_current_text_font_size = -1.0f;
static int vdb_current_text_font = 0;

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

    vdb_current_text_font_size = -1.0f;
    vdb_current_text_font = 0;
}

// Converts user's coordinates into OpenGL normalized device coordinates
ImVec2 UserToOpenGLCoordinates(float x, float y)
{
    float x_ndc = -1.0f + 2.0f*(x-vdb_current_view.left)/(vdb_current_view.right-vdb_current_view.left);
    float y_ndc = -1.0f + 2.0f*(y-vdb_current_view.bottom)/(vdb_current_view.top-vdb_current_view.bottom);
    return ImVec2(x_ndc, y_ndc);
}

// ImGui uses display coordinates for drawing, which is different from framebuffer
// coordinates, if DPI scaling is active: i.e. framebuffer resolution might be half
// of the size of the window on the display.
ImVec2 UserToDisplayCoordinates(float x, float y)
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
void vdb_text_font_size(float size) { vdb_current_text_font_size = size; }

void vdb_text(float x, float y, const char *text, int length)
{
    ImFont *font = GetFontImGuiHandleOrDefault(vdb_current_text_font);

    float font_size = (vdb_current_text_font_size < 0.0f) ? font->FontSize : vdb_current_text_font_size;
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, text, text+length);

    ImVec2 pos = UserToDisplayCoordinates(x,y);
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
    float r_display = r/(vdb_current_view.right-vdb_current_view.left); // could also do y-axis... ideally we do an ellipse?
    user_draw_list->AddCircle(UserToDisplayCoordinates(x, y), r_display, vdb_current_color, 12, vdb_current_line_width);
}
void vdb_circle_filled(float x, float y, float r)
{
    float r_display = r/(vdb_current_view.right-vdb_current_view.left); // could also do y-axis... ideally we do an ellipse?
    user_draw_list->AddCircleFilled(UserToDisplayCoordinates(x, y), r_display, vdb_current_color, 12);
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
    static GLint attrib_in_texel = 0;
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
        attrib_in_texel = glGetAttribLocation(program, "in_texel");
        uniform_projection = glGetUniformLocation(program, "projection");
        uniform_channel0 = glGetUniformLocation(program, "channel0");
        uniform_channel1 = glGetUniformLocation(program, "channel1");
        uniform_gain = glGetUniformLocation(program, "gain");
        uniform_bias = glGetUniformLocation(program, "bias");
        uniform_selector = glGetUniformLocation(program, "selector");
        uniform_blend = glGetUniformLocation(program, "blend");
        uniform_range_min = glGetUniformLocation(program, "range_min");
        uniform_range_max = glGetUniformLocation(program, "range_max");

        // todo: buggy vertexattrib, do we need to create a standard VAO?
        #if 0
        static GLfloat quad_data[6*4] = {
            -1,-1,0,0,
            +1,-1,1,0,
            +1,+1,1,1,
            +1,+1,1,1,
            -1,+1,0,1,
            -1,-1,0,0
        };
        glGenBuffers(1, &quad_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, quad_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        #endif
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
        float r = vdb_current_view.right;
        float l = vdb_current_view.left;
        float t = vdb_current_view.top;
        float b = vdb_current_view.bottom;
        GLfloat projection[4*4] = {
            2.0f/(r-l), 0.0f, 0.0f, -1.0f-2.0f*l/(r-l),
            0.0f, 2.0f/(t-b), 0.0f, -1.0f-2.0f*b/(t-b),
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

    {
        static float position[] = { -1,-1, +1,-1, +1,+1, +1,+1, -1,+1, -1,-1 };
        static float texel[] = { 0,0, 1,0, 1,1, 1,1, 0,1, 0,0 };
        glVertexAttribPointer(attrib_in_position, 2, GL_FLOAT, GL_FALSE, 0, position);
        glEnableVertexAttribArray(attrib_in_position);
        glVertexAttribPointer(attrib_in_texel, 2, GL_FLOAT, GL_FALSE, 0, texel);
        glEnableVertexAttribArray(attrib_in_texel);
    }
    glDrawArrays(GL_TRIANGLES, 0, 6);
    {
        glDisableVertexAttribArray(attrib_in_position);
        glDisableVertexAttribArray(attrib_in_texel);
    }

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
    unsigned char *data = stbi_load(filename, width, height, components, 3);
    if (data)
    {
        SetTexture(slot, data, *width, *height, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
        free(data);
    }
    else
    {
        ConsoleMessage("Failed to load image %s", filename);
    }
}
void vdb_draw_image(int slot) { DrawTextureFancy(GetTextureSlotHandle(slot)); }
void vdb_draw_image_mono(int slot, float r, float g, float b, float a, float range_min, float range_max)
{
    float selector[4] = { r, g, b, a };
    DrawTextureFancy(GetTextureSlotHandle(slot), true, selector, range_min, range_max);
}

//
// OpenGL wrapper
// todo: GLES compatibility to run on tablets
// todo: fixed pipeline deprecation
//

void vdb_gl_clear_color(float r, float g, float b, float a) { glClearColor(r,g,b,a); glClear(GL_COLOR_BUFFER_BIT); }
void vdb_gl_clear_depth(float depth) { glClearDepth(depth); glClear(GL_DEPTH_BUFFER_BIT); }
void vdb_gl_point_size(float size) { glPointSize(size); }
void vdb_gl_line_width(float width) { glLineWidth(width); }

// This is reset to zero before running the user's draw commands
// and checked if still zero afterwards. If it's not zero, it means
// the user did not push and pop equal amounts, and we display an error.
static int push_pop_transform_number = 0;

void vdb_gl_projection(float *v)
{
    // todo: gl deprecation
    glMatrixMode(GL_PROJECTION);
    if (v) glLoadMatrixf(v);
    else glLoadIdentity();
}

void vdb_gl_push_transform(float *v)
{
    push_pop_transform_number++;
    // todo: gl deprecation
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if (v) glLoadMatrixf(v);
    else glLoadIdentity();
}

void vdb_gl_pop_transform()
{
    push_pop_transform_number--;
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
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
