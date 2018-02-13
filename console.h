#pragma once
#include "frame_input.h"

static char console_buffer[1024] = {0};
static char console_front_buffer[1024] = {0};
static size_t console_buffer_used = 0;
static bool console_visible = false;
static float console_animation_end_time = 0.0f;
static float console_animation_start_time = 0.0f;

void ConsoleHideMessage()
{
    if (console_visible)
    {
        console_visible = false;
        console_animation_start_time = frame_input.elapsed_time;
        console_animation_end_time = frame_input.elapsed_time+0.4f;
    }
}

void ConsoleShowMessage()
{
    strcpy(console_front_buffer, console_buffer);
    if (!console_visible)
    {
        console_visible = true;
        console_animation_start_time = frame_input.elapsed_time;
        console_animation_end_time = frame_input.elapsed_time+0.2f;
    }
}

void ConsoleAppendMessageV(const char *fmt, va_list args)
{
    console_buffer_used += vsnprintf(console_buffer+console_buffer_used, sizeof(console_buffer)-console_buffer_used, fmt, args);
}

void ConsoleAppendMessage(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ConsoleAppendMessageV(fmt, args);
    va_end(args);
}

void ConsoleClearMessage()
{
    console_buffer_used = 0;
}

void ConsoleMessage(const char *fmt, ...)
{
    ConsoleClearMessage();
    va_list args;
    va_start(args, fmt);
    ConsoleAppendMessageV(fmt, args);
    va_end(args);
    ConsoleShowMessage();
}

void ConsoleDraw()
{
    if (!console_visible && frame_input.elapsed_time > console_animation_end_time)
        return;

    const char *text = console_front_buffer;

    using namespace ImGui;
    float padding = 10.0f;
    float display_width = GetIO().DisplaySize.x;
    float wrap_width = display_width - 2.0f*padding;
    float text_height = CalcTextSize(text, NULL, false, wrap_width).y;

    float y = 0.0f;
    if (frame_input.elapsed_time < console_animation_end_time)
    {
        float t = frame_input.elapsed_time;
        float t0 = console_animation_start_time;
        float t1 = console_animation_end_time;
        float a = (frame_input.elapsed_time-t0)/(t1-t0);
        float y0 = (console_visible) ? -text_height-2.0f*padding : 0.0f;
        float y1 = (console_visible) ? 0.0f : -text_height-2.0f*padding;
        y = y0 + (y1-y0)*(0.5f+0.5f*sinf(3.1415926f*(a-0.5f)));
    }

    ImDrawList *draw = GetOverlayDrawList();
    draw->AddRectFilled(ImVec2(0,y), ImVec2(display_width, y + text_height + 2.0f*padding), IM_COL32(0,0,0,100), 16.0f, ImDrawCornerFlags_All);
    draw->AddText(GetFont(), GetFontSize(), ImVec2(padding,y+padding), IM_COL32(255,255,255,255), text, NULL, wrap_width);
}
