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

#define Triggered(EVENT, DURATION) \
    static double tdb_timer_##__LINE__ = 0.0f; \
    if (EVENT) tdb_timer_##__LINE__ = glfwGetTime(); \
    if (glfwGetTime() - tdb_timer_##__LINE__ < DURATION)

struct frame_input_t
{
    int window_w;
    int window_h;
    int window_x;
    int window_y;
    int mouse_x;
    int mouse_y;
    float elapsed_time;
    float frame_time;
};

void UpdateAndDraw(frame_input_t input)
{
    glViewport(0, 0, input.window_w, input.window_h);
    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int j = 0; j < 4; j++)
    {
        glBegin(GL_TRIANGLE_FAN);
        if (j == 0)
            glColor4f(0.19f, 0.2f, 0.25f, 1);
        else if (j == 1)
            glColor4f(0.38f, 0.31f, 0.51f, 1);
        else if (j == 2)
            glColor4f(0.29f, 0.22f, 0.38f, 1);
        else if (j == 3)
            glColor4f(0.11f, 0.11f, 0.12f, 1);
        glVertex2f(-1.0f,-1.0f);
        float t = input.elapsed_time;
        for (int i = 0; i <= 128; i++)
        {
            float s = i/128.0f;
            float x = -1.0f+2.0f*s;
            float f = 1.0f - j*0.25f;
            float p = (1.0f + j*0.25f)*s + j*3.14f/2.0f;
            glVertex2f(x, 0.8f+0.1f*sinf(p + f*t)-0.45f*j);
        }
        glVertex2f(+1.0f,-1.0f);
        glEnd();
    }

    ImGui::Text("The time is: %ds", (int)input.elapsed_time);
}

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

void WindowFocusChanged(GLFWwindow *window, int focused)
{
    if (focused == GL_TRUE)
    {
        printf("got focus\n");
    }
    else if (focused == GL_FALSE)
    {
        printf("lost focus\n");
    }
}

int main(int argc, char **argv)
{
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
    glfwWindowHint(GLFW_SAMPLES,        8);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Visual Debugger", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    gladLoadGL();

    glfwSetWindowFocusCallback(window, WindowFocusChanged);

    ImGui_ImplGlfw_Init(window, true);
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
        (const char*)source_sans_pro_compressed_data,
        source_sans_pro_compressed_size, 18.0f);
    ImGui_ImplGlfw_CreateDeviceObjects();
    ImGui::GetIO().MouseDrawCursor = true;

    SetWindowSize(window,480,480);

    glfwSetTime(0.0);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplGlfw_NewFrame();
        frame_input_t input = {0};
        {
            static double last_elapsed_time = 0.0;
            double mouse_x,mouse_y;
            glfwGetWindowPos(window, &input.window_x, &input.window_y);
            glfwGetFramebufferSize(window, &input.window_w, &input.window_h);
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            input.mouse_x = (int)mouse_x;
            input.mouse_y = (int)mouse_y;
            input.elapsed_time = (float)glfwGetTime();
            input.frame_time = (float)(glfwGetTime() - last_elapsed_time);
            last_elapsed_time = glfwGetTime();
        }
        UpdateAndDraw(input);

        #define OneTimeEvent(Var, Event)                     \
            static bool Var##_was_active = (Event);          \
            bool Var##_is_active = (Event);                  \
            bool Var = Var##_is_active && !Var##_was_active; \
            Var##_was_active = Var##_is_active;

        OneTimeEvent(screenshot_button,
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
        OneTimeEvent(escape_button, glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        OneTimeEvent(enter_button, glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        bool escape_eaten = false;

        if (screenshot_button)
        {
            printf("open popup\n");
            ImGui::OpenPopup("Take screenshot##popup");
        }
        if (ImGui::BeginPopupModal("Take screenshot##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            using namespace ImGui;
            static char filename[1024];
            Triggered(screenshot_button, 1.0f)
            {
                printf("focus %f!\n", glfwGetTime());
                SetKeyboardFocusHere();
            }
            InputText("Filename", filename, sizeof(filename));

            static bool checkbox;
            Checkbox("32bpp (alpha channel)", &checkbox);

            if (Button("OK", ImVec2(120,0)) || enter_button)
            {
                // int channels = 3;
                // GLenum format = GL_RGB;
                // if (checkbox)
                // {
                //     format = GL_RGBA;
                //     channels = 4;
                // }
                // int width = input.width;
                // int height = input.height;
                // int stride = width*channels;
                // unsigned char *data = (unsigned char*)malloc(height*stride);
                // glPixelStorei(GL_PACK_ALIGNMENT, 1);
                // glReadBuffer(GL_BACK);
                // glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
                // stbi_write_png(filename, width, height, channels, data+stride*(height-1), -stride);
                // free(data);
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
                escape_eaten = true;
            }
            ImGui::EndPopup();
        }

        ImGui::Render();
        glfwSwapBuffers(window);

        if (escape_button && !escape_eaten)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return 0;
}
