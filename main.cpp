#include "3rdparty/glad.c"
#include "3rdparty/glfw3.h"
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include "imconfig.h"
#include "3rdparty/imgui.h"
#include "3rdparty/imgui_demo.cpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image_write.h"
#include "3rdparty/stb_image.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "3rdparty/glfw3native.h" // required for IME input functionality in imgui
#include <winuser.h> // required for SetWindowSize (to set top-most)
#endif
#include "3rdparty/imgui.cpp"
#include "3rdparty/imgui_draw.cpp"
#include "3rdparty/imgui_impl_glfw.cpp"
#include "imfonts.h"

#include "frame_input.h"
#include "frame_grab.h"
#include "vdb_implementation.h"
// #include "command_buffer.h"
#include "settings.h"

#include "connection_dll.h"

// important that this is included after imgui for some reason, otherwise app crashes on start-up
#include "3rdparty/tinycthread.c"

void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void SetWindowSize(GLFWwindow *window, int width, int height, bool topmost=false)
{
    #ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    int aw = rect.right-rect.left;
    int ah = rect.bottom-rect.top;
    if (topmost)
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, aw, ah, SWP_NOMOVE);
    else
        SetWindowPos(hwnd, HWND_TOP, 0, 0, aw, ah, SWP_NOMOVE);
    #else
    glfwSetWindowSize(window, width, height);
    #endif
}

frame_input_t PollFrameEvents(frame_input_t prev_input, GLFWwindow *window)
{
    frame_input_t input = {0};

    glfwPollEvents();

    glfwGetWindowPos(window, &input.window_x, &input.window_y);
    glfwGetWindowSize(window, &input.window_w, &input.window_h);
    glfwGetFramebufferSize(window, &input.framebuffer_w, &input.framebuffer_h);
    input.is_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == 1;

    if (input.framebuffer_w != prev_input.framebuffer_w ||
        input.framebuffer_h != prev_input.framebuffer_h)
        input.framebuffer_size_changed = true;

    // todo: reuse imgui instead
    static bool key_down[256] = {0};
    for (int i = 0; i < 256; i++)
    {
        bool is_down = glfwGetKey(window, i) == GLFW_PRESS;
        if (!key_down[i] && is_down)
            input.key_press[i] = true;
        input.key_down[i] = is_down;
        key_down[i] = is_down;
    }

    if (prev_input.is_focused && !input.is_focused)
        input.lost_focus = true;
    if (!prev_input.is_focused && input.is_focused)
        input.regained_focus = true;

    double mouse_x,mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    input.mouse_x = (float)mouse_x;
    input.mouse_y = (float)mouse_y;
    input.mouse_u = -1.0f + 2.0f*input.mouse_x/input.window_w;
    input.mouse_v = +1.0f - 2.0f*input.mouse_y/input.window_h;

    static double last_elapsed_time = 0.0;
    input.elapsed_time = (float)glfwGetTime();
    input.frame_time = (float)(glfwGetTime() - last_elapsed_time);
    last_elapsed_time = glfwGetTime();

    return input;
}

GLFWwindow *RecreateWindow(GLFWwindow *old_window, int x, int y, int width, int height, bool borderless=false, bool topmost=false)
{
    glfwWindowHint(GLFW_VISIBLE, 0);
    glfwWindowHint(GLFW_DECORATED, !borderless);
    glfwWindowHint(GLFW_FLOATING, topmost);
    glfwWindowHint(GLFW_FOCUSED, true); // hope this fixes the bug where sometimes window appears in bg
    GLFWwindow *window = glfwCreateWindow(width,height,"ShaderPort",NULL,old_window);
    glfwDestroyWindow(old_window);
    glfwSetWindowPos(window, x, y);
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);

    // todo: make a function that do all of these and replace with here and start of main
    glfwSwapInterval(1);
    ImGui_ImplGlfw_Init(window, true);

    return window;
}

void AfterImGuiInit()
{
    ImGui::GetIO().MouseDrawCursor = true;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().FrameRounding = 5.0f;

    // ImGui::GetIO().IniFilename = "shaderport.ini";
}

void SetWindowSizeDialog(bool *escape_eaten, GLFWwindow *window, frame_input_t input, bool window_size_button, bool enter_button, bool escape_button)
{
    using namespace ImGui;
    if (window_size_button)
    {
        OpenPopup("Set window size##popup");
    }
    if (BeginPopupModal("Set window size##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int width = 0, height = 0;
        if (IsWindowAppearing())
        {
            width = input.window_w;
            height = input.window_h;
        }

        static bool topmost = false;
        InputInt("Width", &width);
        InputInt("Height", &height);
        Separator();
        Checkbox("Topmost", &topmost);

        if (Button("OK", ImVec2(120,0)) || enter_button)
        {
            SetWindowSize(window, width, height, topmost);
            CloseCurrentPopup();
        }
        SameLine();
        if (Button("Cancel", ImVec2(120,0)))
        {
            CloseCurrentPopup();
        }
        if (escape_button)
        {
            CloseCurrentPopup();
            *escape_eaten = true;
        }
        EndPopup();
    }
}

#define Triggered(EVENT, DURATION)                      \
    static double tdb_timer_##__LINE__ = 0.0f;          \
    if (EVENT) tdb_timer_##__LINE__ = glfwGetTime();    \
    if (glfwGetTime() - tdb_timer_##__LINE__ < DURATION)

#define OneTimeEvent(VAR, EVENT)                     \
    static bool VAR##_was_active = (EVENT);          \
    bool VAR##_is_active = (EVENT);                  \
    bool VAR = VAR##_is_active && !VAR##_was_active; \
    VAR##_was_active = VAR##_is_active;

void ResetGLState(frame_input_t input)
{
    glUseProgram(0);

    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glViewport(0, 0, (GLsizei)input.framebuffer_w, (GLsizei)input.framebuffer_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLineWidth(1.0f);
    glPointSize(1.0f);

    // todo: gl deprecation
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // "default" color for gl draw commands. todo: is it legal to call this outside begin/end?

    // Assuming user uploads images that are one-byte packed?
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

static frame_input_t frame_input = {0};
void BeforeUpdateAndDraw(frame_input_t input)
{
    frame_input = input;

    ResetGLState(input);
    glClearColor(0.1f, 0.12f, 0.15f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void AfterUpdateAndDraw(frame_input_t input)
{
    ResetGLState(input);
}

void UpdateAndDraw(frame_input_t input)
{
    ImGui::ShowDemoWindow();
}

int main(int argc, char **argv)
{
    if (argc == 3)
    {
        const char *script_cpp_path = argv[1];
        const char *script_build_folder = argv[2];
        ScriptSetPaths(script_cpp_path, script_build_folder);
    }
    else
    {
        printf("usage: shaderport.exe <path/to/script.cpp> <path/to/build/directory>\n");
        return 0;
    }

    // todo: temporarily disabled for now
    #if 0
    assert(id_count <= 256 && "I limit the ID to 1 byte for now, so there can only be 256 unique draw commands.");
    unsigned int command_buffer_allocation_size = 1024*1024*10;
    if (!AllocateCommandBuffers(command_buffer_allocation_size))
    {
        printf("Did not have enough memory to allocate draw command buffers. Lower the allocation size by passing the command-line argument -buffer_size <your size in bytes>\n");
        return 1;
    }
    #endif

    const char *settings_filename = "shaderport.ini";
    settings_t settings = LoadSettingsOrDefault(settings_filename);

    glfwSetErrorCallback(ErrorCallback);
    if (!glfwInit())
    {
        printf("Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_ALPHA_BITS,     8);
    glfwWindowHint(GLFW_DEPTH_BITS,     24);
    glfwWindowHint(GLFW_STENCIL_BITS,   8);
    glfwWindowHint(GLFW_DOUBLEBUFFER,   1);
    glfwWindowHint(GLFW_SAMPLES,        4);
    glfwWindowHint(GLFW_VISIBLE,        0);
    GLFWwindow *window = glfwCreateWindow(1000, 600, "ShaderPort", NULL, NULL);

    if (settings.window_x >= 0 && settings.window_y >= 0)
        glfwSetWindowPos(window, settings.window_x, settings.window_y);
    glfwSetWindowSize(window, settings.window_w, settings.window_h);
    glfwShowWindow(window);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    gladLoadGL();

    ImGui_ImplGlfw_Init(window, true);
    AfterImGuiInit();

    glfwSetTime(0.0);
    frame_input_t input = {0};
    while (!glfwWindowShouldClose(window))
    {
        {
            bool iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED) == 1;
            if (iconified && !framegrab.active)
            {
                glfwWaitEvents();
                continue;
            }
        }

        input = PollFrameEvents(input, window);

        settings.window_x = input.window_x;
        settings.window_y = input.window_y;
        settings.window_w = input.window_w;
        settings.window_h = input.window_h;

        bool imgui_want_keyboard = ImGui::GetIO().WantCaptureKeyboard;
        bool ctrl_down = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        OneTimeEvent(escape_button, glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        OneTimeEvent(enter_button, glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        OneTimeEvent(s_button,  glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
        OneTimeEvent(w_button, glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
        OneTimeEvent(r_button, glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
        bool screenshot_button = s_button && ctrl_down;
        bool window_size_button = w_button && ctrl_down;
        bool reload_button = r_button && ctrl_down;
        bool escape_eaten = false;

        if (input.framebuffer_size_changed)
            InvalidateAllFonts();

        if (!LoadFontsIfNecessary(input.framebuffer_h))
            glfwSetWindowShouldClose(window, true);

        #if 1
        ImGui_ImplGlfw_NewFrame();
        BeforeUpdateAndDraw(input);
        UpdateAndDraw(input);
        AfterUpdateAndDraw(input);
        #else
        ImGui_ImplGlfw_NewFrame();
        BeforeUpdateAndDraw(input);
        ScriptUpdateAndDraw(input, reload_button);
        AfterUpdateAndDraw(input);
        #endif

        SetWindowSizeDialog(&escape_eaten, window, input, window_size_button, enter_button, escape_button);

        if (framegrab.active)
        {
            framegrab_options_t opt = framegrab.options;

            // render frame and get pixel data
            unsigned char *data;
            int channels,width,height;
            GLenum format;
            {
                if (opt.draw_imgui)
                {
                    if (opt.draw_cursor)
                    {
                        ImGui::Render();
                    }
                    else
                    {
                        ImGui::GetIO().MouseDrawCursor = false;
                        ImGui::Render();

                        // Temporarily re-enable system cursor. This is automatically disabled on the next call to NewFramec
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        ImGui::GetIO().MouseDrawCursor = true;
                    }
                }
                else if (opt.draw_cursor)
                {
                    FramegrabDrawCrosshair(input);
                }
                format = opt.alpha_channel ? GL_RGBA : GL_RGB;
                channels = opt.alpha_channel ? 4 : 3;
                glfwGetFramebufferSize(window, &width, &height);
                data = (unsigned char*)malloc(width*height*channels);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadBuffer(GL_BACK);
                glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
                if (!opt.draw_imgui)
                {
                    ImGui::Render();
                }
            }

            FramegrabSaveOutput(data, width, height, channels, format);

            free(data);

            if (escape_button)
            {
                StopFramegrab();
                escape_eaten = true;
            }
            else if (screenshot_button)
            {
                StopFramegrab();
            }
        }
        else
        {
            // todo: I currently hide the screenshot/framegrab dialog box if the screenshot is
            // being taken (or a video is being recorded). Ideally, I'd like the dialog box to
            // be visible to the user, but invisible to the framegrab. This requires a way to
            // bisect the ImGui rendering: I have to call Render() in order to draw the gui
            // elements, but I don't want to render *this* dialog box yet; I want to render it
            // after I captured the frame.
            FramegrabShowDialog(&escape_eaten, screenshot_button, enter_button, escape_button);

            if (!settings.never_ask_on_exit && escape_button && !escape_eaten)
            {
                ImGui::OpenPopup("Do you want to exit?##popup_exit");
            }
            else if (ImGui::BeginPopupModal("Do you want to exit?##popup_exit", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::Button("Yes"))
                {
                    glfwSetWindowShouldClose(window, true);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                ImGui::Checkbox("Never ask me again", &settings.never_ask_on_exit);
                if (escape_button)
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::Render();
        }

        if (framegrab.overlay_active)
        {
            FramegrabDrawScreenshotAnimation(framegrab.overlay_timer, framegrab.overlay_tex);

            // todo: taking a screenshot usually means that frame will have taken a crazy amount of time
            // so using input.frame_time will make the animation go by super fast that one frame...
            // todo: do we introduce a boolean 'frame_took_longer_than_expected'?
            // framegrab.overlay_timer -= input.frame_time;
            framegrab.overlay_timer -= 1.0f/60.0f;
            if (framegrab.overlay_timer < 0.0f)
            {
                framegrab.overlay_active = false;
                glDeleteTextures(1, &framegrab.overlay_tex);
            }
        }

        if (settings.never_ask_on_exit && escape_button && !escape_eaten)
        {
            glfwSetWindowShouldClose(window, true);
        }

        glfwSwapBuffers(window);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            printf("OpenGL error: %x (%x)\n", error);
            glfwSetWindowShouldClose(window, true);
        }
    }

    SaveSettings(settings, settings_filename);
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return 0;
}
