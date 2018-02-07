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
    if (!console_message)
        return;
    if (frame_input.elapsed_time > display_time_end)
        return;

    using namespace ImGui;
    float padding = 10.0f;
    float display_width = GetIO().DisplaySize.x;
    float wrap_width = display_width - 2.0f*padding;
    float text_height = CalcTextSize(console_message, NULL, false, wrap_width).y;

    float y = 0.0f;
    // blend in
    {
        float t0 = display_time_start;
        float t1 = display_time_start+0.2f;
        float t = (frame_input.elapsed_time-t0)/(t1-t0);
        float y0 = -text_height - 2.0f*padding;
        float y1 = 0.0f;
        if (t >= 0.0f && t <= 1.0f)
            y = y0 + (y1-y0)*(0.5f+0.5f*sinf(3.1415926f*(t-0.5f)));
    }
    // blend out
    {
        float t0 = display_time_end-0.5f;
        float t1 = display_time_end;
        float t = (frame_input.elapsed_time-t0)/(t1-t0);
        float y0 = 0.0f;
        float y1 = -text_height - 2.0f*padding;
        if (t >= 0.0f && t <= 1.0f)
            y = y0 + (y1-y0)*(0.5f+0.5f*sinf(3.1415926f*(t-0.5f)));
    }

    ImDrawList *draw = GetOverlayDrawList();
    draw->AddRectFilled(ImVec2(0,y), ImVec2(display_width, y + text_height + 2.0f*padding), IM_COL32(0,0,0,100), 16.0f, ImDrawCornerFlags_All);
    draw->AddText(GetFont(), GetFontSize(), ImVec2(padding,y+padding), IM_COL32(255,255,255,255), console_message, NULL, wrap_width);
}
