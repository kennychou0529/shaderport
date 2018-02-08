// todo: create st2 build script that calls shaderport.exe ${FILEPATH}
// existing shaderport instance then reads from stdin the filename to
// compile and compiles it. if new instance, it runs vcvarsall x86_amd64
// and compiles it.

#pragma once
#include "dll.h"
#include "3rdparty/tinycthread.h"
#include "console.h"
#include "format.h"

static char script_cpp_path[1024] = {0};
static char script_build_folder[1024] = {0};
static char script_dll_path[1024] = {0};
static char script_dll_temp_path[1024] = {0};
void ScriptSetPaths(const char *cpp_path, const char *build_folder)
{
    sprintf(script_cpp_path, cpp_path);
    sprintf(script_build_folder, build_folder);
    sprintf(script_dll_path, "%s/script.dll", build_folder);
    sprintf(script_dll_temp_path, "%s/script_in_use.dll", build_folder);
}

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

void CompileScript()
{
    // todo: check filenames?

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
        "cd %s &&" // change directory so we don't clutter up the script directory
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
        "%s " // absolute path to cpp file
        "/link -debug -DLL -opt:ref -PDB:script_%%random%%.pdb -export:loop "
        "-out:script.dll",
        script_build_folder,
        script_cpp_path);

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
    CompileScript();
    compile_done = true;
    thrd_exit(0);
    return 0;
}

typedef void script_loop_t(io_t, draw_t draw, gui_t);
static script_loop_t *ScriptLoop = NULL;
void ReloadScriptDLL()
{
    if (!FileExists(script_dll_path))
    {
        // todo: display error message?
        printf("Failed to reload script: could not find dll %s\n", script_dll_path);
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
    while (!CopyFile(script_dll_path, script_dll_temp_path, FALSE))
    {
        // Try again... I guess the FreeLibrary call might be a bit slow?
    }

    // Next, we get the function pointer addresses by
    // loading the library and asking for the pointers.
    handle = LoadLibrary(script_dll_temp_path);
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

void gui_begin(const char *label) { ImGui::Begin(label); }
void gui_begin_no_title(const char *label) { ImGui::Begin(label, NULL, ImGuiWindowFlags_NoTitleBar); }
void gui_end() { ImGui::End(); }
bool gui_slider1f(const char* label, float* v, float v_min, float v_max) { return ImGui::SliderFloat(label, v, v_min, v_max); }
bool gui_slider1i(const char* label, int* v, int v_min, int v_max) { return ImGui::SliderInt(label, v, v_min, v_max); }
bool gui_button(const char *label) { return ImGui::Button(label); }
bool gui_checkbox(const char *label, bool *v) { return ImGui::Checkbox(label, v); }
bool gui_radio(const char *label, int *v, int v_button) { return ImGui::RadioButton(label, v, v_button); }

// todo: keys
#if 1
bool io_key_down(char key) { return false; }
bool io_key_press(char key) { return false; }
#else
bool io_key_down(char key) { return glfwGetKey(key) == GLFW_PRESS; }
bool io_key_press(char key)
{
    static bool was_down[256] = {0};
    bool is_down = io_key_down(key);
    bool press = !was_down[key] && is_down;
    was_down[key] = is_down;
    return press;
}
#endif
bool io_mouse_down(int button)  { return ImGui::IsMouseDown(button); }
bool io_mouse_click(int button) { return ImGui::IsMouseClicked(button); }

void ScriptUpdateAndDraw(frame_input_t input)
{
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
        if (FileExists(script_cpp_path))
        {
            FILETIME write_time = FileLastWriteTime(script_cpp_path);
            if (CompareFileTime(&write_time, &last_write_time) != 0)
            {
                should_recompile = true;
                last_write_time = write_time;
            }
        }
        else
        {
            // todo: display error message
            printf("%s does not exist\n", script_cpp_path);
        }
    }

    if (should_recompile)
    {
        ConsoleSetMessage(0.0f, NULL);
        thrd_t thrd = {0};
        thrd_create(&thrd, StartCompileScript, NULL);
        // todo: thrd_detach?
    }

    if (compile_done) // todo: && compile_success
    {
        ReloadScriptDLL();
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
        io.key_down = io_key_down;
        io.key_press = io_key_press;
        io.mouse_down = io_mouse_down;
        io.mouse_click = io_mouse_click;

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

        gui_t gui = {0};
        gui.begin = gui_begin;
        gui.begin_no_title = gui_begin_no_title;
        gui.end = gui_end;
        gui.slider1f = gui_slider1f;
        gui.slider1i = gui_slider1i;
        gui.button = gui_button;
        gui.checkbox = gui_checkbox;
        gui.radio = gui_radio;

        ScriptLoop(io, draw, gui);
    }
}
