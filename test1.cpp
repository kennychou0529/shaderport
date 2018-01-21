#define SDL_ASSERT_LEVEL 2
#include <SDL.h>
#include <SDL_assert.h>
#include <stdint.h>
#include "3rdparty/glad.c"

#ifdef _WIN32
// Including this in order to call the win32 function that lets me
// set a window to be top-most. I haven't found a nice way to do
// this on osx/linux, so it only works on windows for now.
#undef WIN32_LEAN_AND_MEAN // because glad.c defines this as well
#include <winuser.h>
#include <SDL_syswm.h>
#endif

#define IM_ASSERT SDL_assert
#include "3rdparty/imgui.cpp"
#include "3rdparty/imgui_demo.cpp"
#include "3rdparty/imgui_draw.cpp"
#include "3rdparty/imgui_impl_sdl.cpp"

void SetWindowSize(SDL_Window *window, int w, int h, bool topmost)
{
    #ifdef _WIN32
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info))
    {
        HWND hwnd = info.info.win.window;
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = w;
        rect.bottom = h;
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
        int aw = rect.right-rect.left;
        int ah = rect.bottom-rect.top;
        if (topmost)
        {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, aw, ah, SWP_NOMOVE);
        }
        else
        {
            SetWindowPos(hwnd, HWND_TOP, 0, 0, aw, ah, SWP_NOMOVE);
        }
    }
    else
    {
        SDL_SetWindowSize(window, w, h);
    }
    #else
    SDL_SetWindowSize(window, w, h);
    #endif
}

struct window_t
{
    SDL_Window *sdl_window;
    SDL_GLContext gl_context;
    bool has_vsync;
};

struct window_options_t
{
    int width;
    int height;
    int x;
    int y;
    int major;
    int minor;
    int multisamples;
    int alpha_bits;
    int depth_bits;
    int stencil_bits;
};

window_t OpenWindow(const char *title, window_options_t opt)
{
    bool init_ok = SDL_Init(SDL_INIT_EVERYTHING) == 0;
    SDL_assert(init_ok);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, opt.major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, opt.minor);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, opt.alpha_bits);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, opt.depth_bits);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, opt.stencil_bits);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, opt.multisamples > 0 ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, opt.multisamples);
    SDL_Window *sdl_window = SDL_CreateWindow(
        title,
        (opt.x < 0) ? SDL_WINDOWPOS_CENTERED : opt.x,
        (opt.y < 0) ? SDL_WINDOWPOS_CENTERED : opt.y,
        opt.width,
        opt.height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_assert(sdl_window);
    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
    SDL_assert(gl_context);

    SDL_GL_LoadLibrary(NULL);
    SDL_assert(gladLoadGLLoader(SDL_GL_GetProcAddress));
    SDL_assert(gladLoadGL());

    // 0 for immediate updates, 1 for updates synchronized with the
    // vertical retrace. If the system supports it, you may
    // specify -1 to allow late swaps to happen immediately
    // instead of waiting for the next retrace.
    SDL_GL_SetSwapInterval(1);

    // Instead of using vsync, you can specify a desired framerate
    // that the application will attempt to keep. If a frame rendered
    // too fast, it will sleep the remaining time. Leave swap_interval
    // at 0 when using this.
    bool has_vsync = SDL_GL_GetSwapInterval() == 1 ? true : false;

    window_t result = {0};
    result.has_vsync = has_vsync;
    result.sdl_window = sdl_window;
    result.gl_context = gl_context;
    return result;
}

struct frame_input_t
{
    int window_w;
    int window_h;
    int window_x;
    int window_y;
    int mouse_x;
    int mouse_y;
    float mouse_x_ndc;
    float mouse_y_ndc;
    float elapsed_time;
    float frame_time;
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

int main(int argc, char **argv)
{
    window_options_t options = {0};
    options.width = 640;
    options.height = 480;
    options.x = -1;
    options.y = -1;
    options.major = 3;
    options.minor = 1;
    options.multisamples = 4;
    options.alpha_bits = 8;
    options.depth_bits = 8;
    options.stencil_bits = 8;
    window_t window = OpenWindow("Visual Debugger", options);

    ImGui_ImplSdl_Init(window.sdl_window);
    ImGui_ImplSdl_CreateDeviceObjects();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
            }
            ImGui_ImplSdl_ProcessEvent(&event);
        }

        frame_input_t input = {0};
        {
            int window_w,window_h,window_x,window_y;
            SDL_GetWindowSize(window.sdl_window, &window_w, &window_h);
            SDL_GetWindowPosition(window.sdl_window, &window_x, &window_y);

            int mouse_x,mouse_y;
            SDL_GetMouseState(&mouse_x, &mouse_y);

            float mouse_x_ndc = -1.0f + 2.0f*mouse_x/window_w;
            float mouse_y_ndc = -1.0f + 2.0f*(window_h-mouse_y-1)/window_h;

            input.window_w = window_w;
            input.window_h = window_h;
            input.window_x = window_x;
            input.window_y = window_y;
            input.mouse_x = mouse_x;
            input.mouse_y = mouse_y;
            input.mouse_x_ndc = mouse_x_ndc;
            input.mouse_y_ndc = mouse_y_ndc;
        }

        ImGui_ImplSdl_NewFrame(window.sdl_window);
        UpdateAndDraw(input);
        ImGui::Render();

        // Swap buffers and sleep
        {
            float frame_time;
            float elapsed_time;
            {
                uint64_t count = SDL_GetPerformanceCounter();
                uint64_t frequency = SDL_GetPerformanceFrequency();

                static uint64_t prev_count = 0;
                static uint64_t init_count = 0;
                if (!init_count)
                {
                    init_count = count;
                    prev_count = count;
                }

                frame_time = (float)(count - prev_count) / (float)frequency;
                elapsed_time = (float)(count - init_count) / (float)frequency;

                prev_count = count;

                input.frame_time = frame_time;
                input.elapsed_time = elapsed_time;
            }

            SDL_GL_SwapWindow(window.sdl_window);
            const float target_frame_rate = 60.0f;
            const float target_frame_time = 1.0f/target_frame_rate;
            if (!window.has_vsync && frame_time < target_frame_time)
            {
                float sec = target_frame_time - frame_time;
                uint32_t msec = (uint32_t)(sec*1000.0f);
                SDL_Delay(msec);
            }
        }
    }

    ImGui_ImplSdl_Shutdown();
    SDL_GL_DeleteContext(window.gl_context);
    SDL_DestroyWindow(window.sdl_window);
    SDL_Quit();

    return 0;
}
