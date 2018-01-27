#pragma once

static char log_buffer[1024*1024];

// todo: replace with internal console rendered to screen
void Log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
