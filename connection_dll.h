// todo: create st2 build script that calls shaderport.exe ${FILEPATH}
// existing shaderport instance then reads from stdin the filename to
// compile and compiles it. if new instance, it runs vcvarsall x86_amd64
// and compiles it.

#pragma once
#include "dll.h"
#include "3rdparty/tinycthread.h"
#include "console.h"

typedef void script_loop_t(io_t, draw_t draw);
static script_loop_t *ScriptLoop = NULL;

bool FileExists(const char *filename)
{
    WIN32_FIND_DATA data;
    HANDLE handle = FindFirstFileA(filename, &data);
    if (handle != INVALID_HANDLE_VALUE)
    {
        FindClose(handle);
        return true;
    }
    else
    {
        return false;
    }
}

FILETIME FileLastWriteTime(const char *filename)
{
    FILETIME t = {};
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFileA(filename, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        t = fd.ftLastWriteTime;
        FindClose(h);
    }
    return t;
}

struct expanding_string_t
{
    char buffer[1024*1024];
    size_t used;
    void clear() { used = 0; buffer[0] = 0; }
    void append(const char *s)
    {
        size_t len = strlen(s);
        if (used + len <= sizeof(buffer))
        {
            strcpy(buffer+used, s);
            used += len;
        }
    }
};

static expanding_string_t compiler_messages = {0};

void CompileScript(const char *cpp_filename, const char *dll_filename)
{
    // todo: this doesn't work if shaderport is run from outside a batch file?
    // it works if you invoke vcvarsall and then run shaderport though.
    static bool has_vcvarsall = false;
    if (!has_vcvarsall)
    {
        system("vcvarsall x86_amd64");
        has_vcvarsall = true;
    }

    char cmd[1024];
    sprintf(cmd,
        "cd C:/Temp/build &&" // change directory so we don't clutter up the script directory
        "del vc110.pdb > NUL 2> NUL &&" // delete old generated files...
        "del script.obj > NUL 2> NUL &&"
        "del script.lib > NUL 2> NUL &&"
        "del script.exp > NUL 2> NUL &&"
        "del script.dll > NUL 2> NUL &&"
        "del script_in_use.dll > NUL 2> NUL &&"
        "del script*.pdb > NUL 2> NUL &&"
        "echo WAITING FOR PDB > lock.tmp &&"
        // "vcvarsall x86_amd64 && " // todo: can we somehow invoke vcvarsall once before popen? system(...) does not persist...
        "cl -Zi -nologo -Oi -Od -WX -W4 -wd4505 -wd4189 -wd4100 -fp:fast "
        "%s " // cpp_filename
        "/link -debug -DLL -opt:ref -PDB:script_%%random%%.pdb -export:loop "
        "-out:%s",
        cpp_filename,
        dll_filename);

    FILE *p = _popen(cmd, "rt");
    if (!p)
    {
        printf("Failed to run command (could not open pipe):\n");
        printf(cmd);
        return;
    }

    compiler_messages.clear();
    int lines = 0;
    char buffer[1024];
    while (lines < 5 && fgets(buffer, sizeof(buffer), p))
    {
        compiler_messages.append(buffer);
        lines++;
    }
    _pclose(p);
}

volatile bool compile_done = false;
int StartCompileScript(void *arg)
{
    compile_done = false;
    CompileScript("C:/Programming/shaderport/script/script.cpp", "script.dll");
    compile_done = true;
    thrd_exit(0);
    return 0;
}

void ReloadScriptDLL(const char *dll_filename)
{
    if (!FileExists(dll_filename))
    {
        printf("Failed to reload script: could not find dll\n");
        return;
    }

    // We store a handle to the DLL in order to free it later
    static HMODULE handle = NULL;

    // We need to unload the library in order to copy the user's
    // dll and overwrite the dll that we're using/going to use.
    if (handle)
    {
        FreeLibrary(handle);
        handle = NULL;
        ScriptLoop = NULL;
    }

    // First, we must rename the DLL file that our
    // game code resides in so that we can overwrite
    // it in a later compilation. If we do not rename
    // it, the vc compiler would not be able to overwrite
    // the DLL, because hey, we're using it.
    const char *temp_filename = "script_in_use.dll";
    while (!CopyFile(dll_filename, temp_filename, FALSE))
    {
        // Try again... I guess the FreeLibrary call might be a bit slow?
    }

    // Next, we get the function pointer addresses by
    // loading the library and asking for the pointers.
    handle = LoadLibrary(temp_filename);
    if (!handle)
    {
        printf("Failed to reload script: LoadLibrary failed\n");
        return;
    }

    ScriptLoop = (script_loop_t*)GetProcAddress(handle, "loop");

    if (!ScriptLoop)
    {
        printf("Failed to reload script: could not find routine 'loop'\n");
    }
}

void ScriptUpdateAndDraw(frame_input_t input)
{
    const char *script_filename = "C:/Programming/shaderport/script/script.cpp";
    const char *dll_filename = "C:/Temp/build/script.dll";

    bool should_check_write_time = false;
    {
        const float file_check_interval = 0.5f;
        static float last_file_check = 0.0f;
        if (input.elapsed_time - last_file_check > file_check_interval)
        {
            last_file_check = input.elapsed_time;
            should_check_write_time = true;
        }
    }

    bool should_recompile = false;
    if (should_check_write_time)
    {
        static FILETIME last_write_time = {0};
        if (FileExists(script_filename))
        {
            FILETIME write_time = FileLastWriteTime(script_filename);
            if (CompareFileTime(&write_time, &last_write_time) != 0)
            {
                should_recompile = true;
                last_write_time = write_time;
            }
        }
    }

    if (should_recompile)
    {
        ConsoleSetMessage(0.0f, NULL);
        thrd_t thrd = {0};
        thrd_create(&thrd, StartCompileScript, NULL);
        // todo: thrd_detach?
    }

    if (compile_done)
    {
        ReloadScriptDLL(dll_filename);
        compile_done = false;
        ConsoleSetMessage(5.0f, compiler_messages.buffer);
    }

    ConsoleDrawMessage();

    if (ScriptLoop)
    {
        draw_t draw = {0};

        draw.path_clear = vdb_path_clear;
        draw.path_to = vdb_path_to;
        draw.path_fill = vdb_path_fill;
        draw.path_stroke = vdb_path_stroke;
        draw.view = vdb_view;
        draw.color = vdb_color;
        draw.line_width = vdb_line_width;
        draw.point_size = vdb_point_size;
        draw.text = vdb_text_formatted;
        draw.point = vdb_point;
        draw.line = vdb_line;
        draw.triangle = vdb_triangle;
        draw.triangle_filled = vdb_triangle_filled;
        draw.quad = vdb_quad;
        draw.quad_filled = vdb_quad_filled;
        draw.rect = vdb_rect;
        draw.rect_filled = vdb_rect_filled;
        draw.circle = vdb_circle;
        draw.circle_filled = vdb_circle_filled;

        io_t io = {0};
        io.window_x = input.window_x;
        io.window_y = input.window_y;
        io.window_w = input.window_w;
        io.window_h = input.window_h;
        io.framebuffer_w = input.framebuffer_w;
        io.framebuffer_h = input.framebuffer_h;
        io.mouse_x = input.mouse_x;
        io.mouse_y = input.mouse_y;
        io.mouse_u = input.mouse_u;
        io.mouse_v = input.mouse_v;
        io.elapsed_time = input.elapsed_time;
        io.frame_time = input.frame_time;
        io.recording_video = input.recording_video;
        io.lost_focus = input.lost_focus;
        io.regained_focus = input.regained_focus;

        ScriptLoop(io, draw);
    }
}
