#include "3rdparty/glad.c"
#include "3rdparty/glfw3.h"
#include <stdio.h>

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

struct frame_input_t
{
    int window_w;
    int window_h;
    int window_x;
    int window_y;
    int mouse_x;
    int mouse_y;
    double elapsed_time;
    double frame_time;
};

void UpdateAndDraw(frame_input_t input)
{
    glViewport(0, 0, input.window_w, input.window_h);
    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_LINES);
    glColor4f(1,1,1,1);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(+0.5f, +0.5f);
    glEnd();

    ImGui::Text("Hello ImGui!");
    ImGui::ShowTestWindow();
}

void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
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
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF((const char*)source_sans_pro_compressed_data, source_sans_pro_compressed_size, 18.0f);
    // ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\NotoSans-Bold.ttf", 17.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
    ImGui_ImplGlfw_CreateDeviceObjects();

    glfwSetTime(0.0);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        ImGui_ImplGlfw_NewFrame();
        frame_input_t input = {0};
        {
            glfwGetWindowPos(window, &input.window_x, &input.window_y);

            glfwGetFramebufferSize(window, &input.window_w, &input.window_h);

            double mouse_x,mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            input.mouse_x = (int)mouse_x;
            input.mouse_y = (int)mouse_y;

            input.elapsed_time = glfwGetTime();

            static double last_elapsed_time = 0.0;
            input.frame_time = glfwGetTime() - last_elapsed_time;
            last_elapsed_time = glfwGetTime();
        }
        UpdateAndDraw(input);
        ImGui::Render();
        glfwSwapBuffers(window);
    }

    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return 0;
}
