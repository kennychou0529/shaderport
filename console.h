#pragma once

static ExampleAppLog app_log;
void LogToScreenConsole(const char *line)
{
    // input.elapsed_time = (float)glfwGetTime();
    using namespace ImGui;

    app_log.AddLog(line);


    // float wrap_width = GetIO().DisplaySize.x;
    // ImDrawList *draw = GetOverlayDrawList();
    // draw->AddText(GetFont(), GetFontSize()*0.8f, ImVec2(1,1), IM_COL32(0,0,0,255), line, NULL, wrap_width);
    // draw->AddText(GetFont(), GetFontSize()*0.8f, ImVec2(0,0), IM_COL32(255,255,255,255), line, NULL, wrap_width);
    // printf("[INFO] %s", line);
}
