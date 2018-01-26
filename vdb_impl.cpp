#include "vdb.h"
#include "transform.h"
#include "frameinput.h"
// #include "3rdparty/imgui.h"

static int draw_string_id = 0;
static ImDrawList *user_draw_list = NULL;

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
}

// todo: are x,y screen or framebuffer coordinates?
void vdb_text_unformatted(float x, float y, const char *text)
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

// todo: vdb_text_center_x(); vdb_text_left(); ...
void vdb_text(float x, float y, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (w == -1 || w >= (int)sizeof(buffer))
        w = (int)sizeof(buffer) - 1;
    buffer[w] = 0;
    vdb_text_unformatted(x, y, buffer);
    va_end(args);
}

// todo: put these in a more logical place
void vdb_path_clear()
{
    user_draw_list->PathClear();
}
void vdb_path_line_to(float x, float y)
{
    user_draw_list->PathLineTo(ImVec2(NdcToFbX(x), NdcToFbY(y)));
}
void vdb_path_fill_convex(float r, float g, float b, float a)
{
    user_draw_list->PathFillConvex(IM_COL32((int)r, (int)g, (int)b, (int)a));
}
