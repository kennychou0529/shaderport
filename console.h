#pragma once
#include "frame_input.h"

float display_time_start = 0.0f;
float display_time_end = 0.0f;
const char *console_message = NULL;
void ConsoleSetMessage(float duration, const char *msg)
{
    display_time_start = frame_input.elapsed_time;
    display_time_end = display_time_start + duration;
    console_message = msg;
}

void ConsoleDrawMessage()
{
    if (frame_input.elapsed_time <= display_time_end)
    {
        using namespace ImGui;
        float padding = 10.0f;
        float display_width = GetIO().DisplaySize.x;
        float wrap_width = display_width - 2.0f*padding;
        ImDrawList *draw = GetOverlayDrawList();
        float text_height = CalcTextSize(console_message, NULL, false, wrap_width).y;
        draw->AddRectFilled(ImVec2(0,0), ImVec2(display_width, text_height + 2.0f*padding), IM_COL32(0,0,0,100), 16.0f, ImDrawCornerFlags_All);
        draw->AddText(GetFont(), GetFontSize(), ImVec2(padding,padding), IM_COL32(255,255,255,255), console_message, NULL, wrap_width);
    }
}
