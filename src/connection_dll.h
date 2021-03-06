#pragma once
#include <stdio.h>
#include <string.h>
#include "connection_dll_api.h"
#include "3rdparty/tinycthread.h"
#include "vdb_implementation.h"
#include "console.h"

#ifdef _WIN32
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
#else
#error "Implement missing functions"
#endif

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

bool CompileScript()
{
    // todo: check filenames?
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
        return false;
    }

    ConsoleClearMessage();
    int lines = 0;
    char buffer[1024];
    while (lines < 5 && fgets(buffer, sizeof(buffer), p))
    {
        ConsoleAppendMessage(buffer);
        lines++;
    }
    int return_value = _pclose(p);
    if (return_value == 0)
        return true;
    return false;
}

volatile bool compile_done = false;
volatile bool compile_success = false;
int StartCompileScript(void *arg)
{
    compile_done = false;
    compile_success = false;
    ConsoleHideMessage();
    if (CompileScript())
        compile_success = true;
    compile_done = true;
    thrd_exit(0);
    return 0;
}

typedef void script_loop_t(io_t, draw_t draw, gui_t, gl_t);
static script_loop_t *ScriptLoop = NULL;
bool ReloadScriptDLL()
{
    if (!FileExists(script_dll_path))
    {
        ConsoleAppendMessage("Failed to reload script: could not find DLL (%s)", script_dll_path);
        return false;
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
        ConsoleAppendMessage("Failed to reload script: LoadLibrary failed");
        return false;
    }

    ScriptLoop = (script_loop_t*)GetProcAddress(handle, "loop");

    if (!ScriptLoop)
    {
        ConsoleAppendMessage("Failed to reload script: Could not find routine 'loop'");
        return false;
    }

    return true;
}

#include "reset_gl_state.h"

void ScriptUpdateAndDraw(frame_input_t input, bool reload_button)
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
    if (reload_button)
        should_recompile = true;

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
            ConsoleMessage("Failed to compile script: %s does not exist\n", script_cpp_path);
        }
    }

    if (should_recompile)
    {
        thrd_t thrd = {0};
        thrd_create(&thrd, StartCompileScript, NULL);
        // todo: thrd_detach?
    }

    bool was_reload = false;
    if (compile_done)
    {
        if (compile_success)
        {
            ConsoleClearMessage();
            if (ReloadScriptDLL())
            {
                was_reload = true;
                ConsoleHideMessage();
            }
            else
            {
                ConsoleShowMessage();
            }
        }
        else
        {
            ConsoleShowMessage();
        }
        compile_done = false;
        compile_success = false;
    }

    ConsoleDraw();

    if (ScriptLoop)
    {
        draw_t draw = {0};

        draw.viewport = vdb_viewport;
        draw.transform = vdb_transform;

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

        draw.text_background = vdb_text_background;
        draw.text_no_background = vdb_text_no_background;
        draw.text_shadow = vdb_text_shadow;
        draw.text_x_left = vdb_text_x_left;
        draw.text_x_center = vdb_text_x_center;
        draw.text_x_right = vdb_text_x_right;
        draw.text_y_top = vdb_text_y_top;
        draw.text_y_center = vdb_text_y_center;
        draw.text_y_bottom = vdb_text_y_bottom;
        draw.text_size = vdb_text_size;
        draw.text_size_absolute = vdb_text_size_absolute;
        draw.text_font = vdb_text_font;
        draw.load_font = vdb_load_font;

        draw.load_image_file = vdb_load_image_file;
        draw.load_image_u08 = vdb_load_image_u08;
        draw.load_image_f32 = vdb_load_image_f32;
        draw.image = vdb_draw_image;
        draw.image_mono = vdb_draw_image_mono;

        draw.load_video = vdb_load_video;
        draw.video = vdb_draw_video;

        io_t io = {0};
        io.key_down = vdb_io_key_down;
        io.key_press = vdb_io_key_press;
        io.mouse_down = vdb_io_mouse_down;
        io.mouse_click = vdb_io_mouse_click;
        io.mouse_wheel = input.mouse_wheel;

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
        io.lost_focus = input.lost_focus;
        io.regained_focus = input.regained_focus;
        io.reload = was_reload;

        gui_t gui = {0};
        gui.begin = vdb_gui_begin;
        gui.begin_no_title = vdb_gui_begin_no_title;
        gui.end = vdb_gui_end;
        gui.slider1f = vdb_gui_slider1f;
        gui.slider1i = vdb_gui_slider1i;
        gui.button = vdb_gui_button;
        gui.checkbox = vdb_gui_checkbox;
        gui.radio = vdb_gui_radio;

        gl_t gl = {0};
        gl.clear_color = vdb_gl_clear_color;
        gl.clear_depth = vdb_gl_clear_depth;
        gl.point_size = vdb_gl_point_size;
        gl.line_width = vdb_gl_line_width;
        gl.projection = vdb_gl_projection;
        gl.push_transform = vdb_gl_push_transform;
        gl.pop_transform = vdb_gl_pop_transform;
        gl.blend_alpha = vdb_gl_blend_alpha;
        gl.blend_additive = vdb_gl_blend_additive;
        gl.blend_disable = vdb_gl_blend_disable;
        gl.depth_test = vdb_gl_depth_test;
        gl.color = vdb_gl_color;
        gl.vertex = vdb_gl_vertex;
        gl.texel = vdb_gl_texel;
        gl.texture = vdb_gl_texture;
        gl.bind_texture = vdb_gl_bind_texture;
        gl.begin_lines = vdb_gl_begin_lines;
        gl.begin_triangles = vdb_gl_begin_triangles;
        gl.begin_points = vdb_gl_begin_points;
        gl.end = vdb_gl_end;
        gl.load_points = vdb_gl_load_points;
        gl.draw_points = vdb_gl_draw_points;

        gl.load_shader = vdb_gl_load_shader;
        gl.begin_shader = vdb_gl_begin_shader;
        gl.end_shader = vdb_gl_end_shader;
        gl.shader_uniform1f = vdb_gl_shader_uniform1f;
        gl.shader_uniform2f = vdb_gl_shader_uniform2f;
        gl.shader_uniform3f = vdb_gl_shader_uniform3f;
        gl.shader_uniform4f = vdb_gl_shader_uniform4f;
        gl.shader_uniform3x3f = vdb_gl_shader_uniform3x3f;
        gl.shader_uniform4x4f = vdb_gl_shader_uniform4x4f;
        gl.enable_temporal_blend = vdb_gl_enable_temporal_blend;
        gl.supersample = vdb_gl_supersample;

        vdbBeforeUpdateAndDraw(input);

        ScriptLoop(io, draw, gui, gl);

        if (!vdbAfterUpdateAndDraw(input))
        {
            ScriptLoop = NULL;
        }
    }
}
