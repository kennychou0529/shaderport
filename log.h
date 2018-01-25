#pragma once

// todo: replace with internal console rendered to screen
void Log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
