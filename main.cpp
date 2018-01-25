#include "3rdparty/glad.c"
#include "3rdparty/glfw3.h"

// required for IME input functionality in imgui
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "3rdparty/glfw3native.h"

#ifdef _WIN32
#undef APIENTRY // becomes defined by imgui when it include windows stuff for IME and clipboard
#endif
#include "3rdparty/imgui.cpp"
#include "3rdparty/imgui_demo.cpp"
#include "3rdparty/imgui_draw.cpp"
#include "3rdparty/imgui_impl_glfw.cpp"
#include "fonts/source_sans_pro.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image_write.h"
#include "3rdparty/stb_image.h"

// todo: replace with internal console rendered to screen
void Log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

#include "3rdparty/libtcc.h"
#include "script.cpp"
// #include "script/include/vdb.h"

#include "texture.cpp"
#include "perframe.cpp"
#include "framegrab.cpp"
#include "macro_triggered.h"
#include "macro_one_time_event.h"

void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

#ifdef _WIN32
#include <winuser.h>
#endif
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

bool lost_focus = false;
bool regained_focus = false;
void WindowFocusChanged(GLFWwindow *window, int focused)
{
    regained_focus = focused == GL_TRUE;
    lost_focus = focused == GL_FALSE;
}

bool is_iconified = false;
void WindowIconifyChanged(GLFWwindow *window, int iconified)
{
    if (iconified == GL_TRUE)
        is_iconified = true;
    else if (iconified == GL_FALSE)
        is_iconified = false;
}

frame_input_t PollFrameEvents(GLFWwindow *window)
{
    frame_input_t input = {0};

    // These global variables are set in the callback WindowFocusChanged,
    // which is called by glfwPollEvents if their corresponding events occur.
    lost_focus = false;
    regained_focus = false;

    glfwPollEvents();

    input.lost_focus = lost_focus;
    input.regained_focus = regained_focus;

    glfwGetWindowPos(window, &input.window_x, &input.window_y);
    glfwGetWindowSize(window, &input.window_w, &input.window_h);
    glfwGetFramebufferSize(window, &input.framebuffer_w, &input.framebuffer_h);

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

    input.recording_video = framegrab.active;

    return input;
}

void AfterImGuiInit()
{
    // Add the cool extra fonts we want here
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
        (const char*)source_sans_pro_compressed_data,
        source_sans_pro_compressed_size, 18.0f);

    // Adding fonts should be done before this is called
    // (it's called automatically on first frame)
    // ImGui_ImplGlfw_CreateDeviceObjects();

    ImGui::GetIO().MouseDrawCursor = true;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().FrameRounding = 5.0f;
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

int main(int argc, char **argv)
{
    // assert(false && "See comment above");

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
    glfwWindowHint(GLFW_SAMPLES,        0);
    GLFWwindow *window = glfwCreateWindow(1000, 600, "Visual Debugger", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    gladLoadGL();

    glfwSetWindowFocusCallback(window, WindowFocusChanged);
    glfwSetWindowIconifyCallback(window, WindowIconifyChanged);

    ImGui_ImplGlfw_Init(window, true);
    AfterImGuiInit();

    glfwSetTime(0.0);
    while (!glfwWindowShouldClose(window))
    {
        if (is_iconified && !framegrab.active)
        {
            glfwWaitEvents();
            continue;
        }

        frame_input_t input = PollFrameEvents(window);

        ImGui_ImplGlfw_NewFrame();
        BeforeUpdateAndDraw(input);
        UpdateAndDraw(input);
        AfterUpdateAndDraw(input);

        OneTimeEvent(escape_button, glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        OneTimeEvent(enter_button, glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        OneTimeEvent(screenshot_button, glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
        OneTimeEvent(window_size_button, glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
        bool escape_eaten = false;

        if (enter_button)
            ReloadScript();

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

            if (screenshot_button)
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

        glfwSwapBuffers(window);

        if (escape_button && !escape_eaten)
        {
            glfwSetWindowShouldClose(window, true);
        }

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            printf("OpenGL error: %x (%x)\n", error);
            glfwSetWindowShouldClose(window, true);
        }
    }

    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return 0;
}
