#include "vdb.h"
#include "transform.h"
#include "frameinput.h"
// #include "3rdparty/imgui.h"

static int draw_string_id = 0;
static ImDrawList *user_draw_list = NULL;
static ImU32 vdb_current_color = IM_COL32(1.0f,1.0f,1.0f,1.0f);
static float vdb_current_line_width = 1.0f;
static float vdb_current_point_size = 1.0f;

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

    vdb_current_color = IM_COL32(1.0f,1.0f,1.0f,1.0f);
    vdb_current_line_width = 1.0f;
    vdb_current_point_size = 1.0f;
}

// todo: are x,y screen or framebuffer coordinates?
void DrawTextUnformatted(float x, float y, const char *text)
{
    ImVec2 text_size = ImGui::CalcTextSize(text);
    x -= text_size.x*0.5f;
    y -= text_size.y*0.5f;
    #if 1
    char id[32];
    sprintf(id, "##DrawString%d", draw_string_id++);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,1.0f,1.0f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f,0.0f,0.0f,0.4f));
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::Begin(id, 0, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text(text);
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    #else
    // this will render on top of everything, which I don't really want!
    ImDrawList *draw = ImGui::GetOverlayDrawList();
    draw->AddText(ImVec2(x+1,y+1), IM_COL32(0,0,0,255), text);
    draw->AddText(ImVec2(x,y), IM_COL32(255,255,255,255), text);
    #endif
}

void vdb_text(float x, float y, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (w == -1 || w >= (int)sizeof(buffer))
        w = (int)sizeof(buffer) - 1;
    buffer[w] = 0;
    DrawTextUnformatted(x, y, buffer);
    va_end(args);
}

void vdb_path_clear()
{
    user_draw_list->PathClear();
}
void vdb_path_to(float x, float y)
{
    user_draw_list->PathLineTo(ImVec2(NdcToFbX(x), NdcToFbY(y)));
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
    ImVec2 a = ImVec2(x - vdb_current_point_size, y - vdb_current_point_size);
    ImVec2 b = ImVec2(x + vdb_current_point_size, y + vdb_current_point_size);
    user_draw_list->AddRect(a, b, vdb_current_color);
}
void vdb_line(float x1, float y1, float x2, float y2)
{
    ImVec2 a = ImVec2(x1,y1);
    ImVec2 b = ImVec2(x2,y2);
    user_draw_list->AddLine(a, b, vdb_current_color, vdb_current_line_width);
}
void vdb_triangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    ImVec2 a = ImVec2(x1,y1);
    ImVec2 b = ImVec2(x2,y2);
    ImVec2 c = ImVec2(x3,y3);
    user_draw_list->AddTriangle(a, b, c, vdb_current_color, vdb_current_line_width);
}
void vdb_triangle_filled(float x1, float y1, float x2, float y2, float x3, float y3)
{
    ImVec2 a = ImVec2(x1,y1);
    ImVec2 b = ImVec2(x2,y2);
    ImVec2 c = ImVec2(x3,y3);
    user_draw_list->AddTriangleFilled(a, b, c, vdb_current_color);
}
void vdb_quad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    ImVec2 a = ImVec2(x1,y1);
    ImVec2 b = ImVec2(x2,y2);
    ImVec2 c = ImVec2(x3,y3);
    ImVec2 d = ImVec2(x4,y4);
    user_draw_list->AddQuad(a, b, c, d, vdb_current_color, vdb_current_line_width);
}
void vdb_quad_filled(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    ImVec2 a = ImVec2(x1,y1);
    ImVec2 b = ImVec2(x2,y2);
    ImVec2 c = ImVec2(x3,y3);
    ImVec2 d = ImVec2(x4,y4);
    user_draw_list->AddQuadFilled(a, b, c, d, vdb_current_color);
}
void vdb_rect(float x, float y, float w, float h)
{
    ImVec2 a = ImVec2(x,y);
    ImVec2 b = ImVec2(x+w,y+h);
    user_draw_list->AddRect(a, b, vdb_current_color, 0.0f, 0, vdb_current_line_width);
}
void vdb_rect_filled(float x, float y, float w, float h)
{
    ImVec2 a = ImVec2(x,y);
    ImVec2 b = ImVec2(x+w,y+h);
    user_draw_list->AddRectFilled(a, b, vdb_current_color);
}
void vdb_circle(float x, float y, float r)
{
    user_draw_list->AddCircle(ImVec2(x, y), r, vdb_current_color, 12, vdb_current_line_width);
}
void vdb_circle_filled(float x, float y, float r)
{
    user_draw_list->AddCircleFilled(ImVec2(x, y), r, vdb_current_color, 12);
}
