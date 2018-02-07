#pragma once

void DisplayError(const char *msg)
{
    using namespace ImGui;
    float padding = 10.0f;
    float display_width = GetIO().DisplaySize.x;
    float wrap_width = display_width - 2.0f*padding;
    ImDrawList *draw = GetOverlayDrawList();
    float text_height = CalcTextSize(msg, NULL, false, wrap_width).y;
    draw->AddRectFilled(ImVec2(0,0), ImVec2(display_width, text_height + 2.0f*padding), IM_COL32(0,0,0,100), 16.0f, ImDrawCornerFlags_All);
    draw->AddText(GetFont(), GetFontSize(), ImVec2(padding,padding), IM_COL32(255,255,255,255), msg, NULL, wrap_width);
}
