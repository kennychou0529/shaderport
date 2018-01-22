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
#include "3rdparty/stb_image_write.h"

#include <string.h>

#include "texture.cpp"

#define Triggered(EVENT, DURATION)                      \
    static double tdb_timer_##__LINE__ = 0.0f;          \
    if (EVENT) tdb_timer_##__LINE__ = glfwGetTime();    \
    if (glfwGetTime() - tdb_timer_##__LINE__ < DURATION)

#define OneTimeEvent(VAR, EVENT)                     \
    static bool VAR##_was_active = (EVENT);          \
    bool VAR##_is_active = (EVENT);                  \
    bool VAR = VAR##_is_active && !VAR##_was_active; \
    VAR##_was_active = VAR##_is_active;

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
    bool recording_video;
};

void UpdateAndDraw(frame_input_t input)
{
    static float anim_time = 0.0f;
    anim_time += 1.0f/60.0f;
    glViewport(0, 0, input.window_w, input.window_h);
    glClearColor(0.1f, 0.12f, 0.15f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int j = 0; j < 4; j++)
    {
        glBegin(GL_TRIANGLE_FAN);
        if (j == 0)
            glColor4f(0.19f, 0.2f, 0.25f, 1.0f);
        else if (j == 1)
            glColor4f(0.38f, 0.31f, 0.51f, 1.0f);
        else if (j == 2)
            glColor4f(0.29f, 0.22f, 0.38f, 1.0f);
        else if (j == 3)
            glColor4f(0.11f, 0.11f, 0.12f, 1.0f);
        glVertex2f(-1.0f,-1.0f);
        float t = anim_time;
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

    glPointSize(32.0f);
    glBegin(GL_POINTS);
    glColor4f(1.0f,1.0f,1.0f,0.3f);
    glVertex2f(0.0f,0.0f);
    glEnd();

    ImGui::Text("The time is: %ds", (int)input.elapsed_time);
    ImGui::ShowDemoWindow();
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

bool lost_focus = false;
bool regained_focus = false;
void WindowFocusChanged(GLFWwindow *window, int focused)
{
    if (focused == GL_TRUE)
    {
        regained_focus = true;
    }
    else if (focused == GL_FALSE)
    {
        lost_focus = true;
    }
}

struct framegrab_options_t
{
    const char *filename;
    bool alpha_channel;
    bool draw_cursor;
    bool draw_imgui;
    bool is_video;
    bool use_ffmpeg;
    float ffmpeg_fps;
    bool reset_num_screenshots;
    bool reset_num_video_frames;
    int video_frame_cap;
};

struct framegrab_t
{
    framegrab_options_t options;
    bool active;
    int num_screenshots;
    int num_video_frames;

    GLuint overlay_tex;
    float overlay_timer;
    bool overlay_active;
    bool should_stop;
};
framegrab_t framegrab = {0};

void StartFrameGrab(framegrab_options_t opt)
{
    framegrab.options = opt;
    if (opt.reset_num_screenshots)
        framegrab.num_screenshots = 0;
    if (opt.reset_num_video_frames)
        framegrab.num_video_frames = 0;
    framegrab.active = true;
    framegrab.should_stop = false;
}

void TakeScreenshot(
    const char *filename, bool imgui=false, bool cursor=false, bool reset=false, bool alpha=false)
{
    framegrab_options_t opt = {0};
    opt.filename = filename;
    opt.reset_num_screenshots = reset;
    opt.draw_imgui = imgui;
    opt.draw_cursor = cursor;
    opt.alpha_channel = alpha;
    StartFrameGrab(opt);
}

void RecordVideoToImageSequence(
    const char *filename, int frame_cap, bool imgui=false, bool cursor=false, bool reset=false, bool alpha=false)
{
    framegrab_options_t opt = {0};
    opt.filename = filename;
    opt.reset_num_video_frames = reset;
    opt.draw_imgui = imgui;
    opt.draw_cursor = cursor;
    opt.alpha_channel = alpha;
    opt.is_video = true;
    opt.video_frame_cap = frame_cap;
    StartFrameGrab(opt);
}

void RecordVideoToFfmpeg(
    const char *filename, float fps, int frame_cap, bool imgui=false, bool cursor=false, bool reset=false, bool alpha=false)
{
    framegrab_options_t opt = {0};
    opt.filename = filename;
    opt.reset_num_video_frames = reset;
    opt.draw_imgui = imgui;
    opt.draw_cursor = cursor;
    opt.alpha_channel = alpha;
    opt.is_video = true;
    opt.use_ffmpeg = true;
    opt.ffmpeg_fps = fps;
    opt.video_frame_cap = frame_cap;
    StartFrameGrab(opt);
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
    glfwWindowHint(GLFW_SAMPLES,        4);
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
    ImGui::StyleColorsDark();
    ImGui::GetStyle().FrameRounding = 5.0f;

    glfwSetTime(0.0);
    while (!glfwWindowShouldClose(window))
    {
        lost_focus = false;
        regained_focus = false;
        glfwPollEvents();

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
            #if 1
            input.frame_time = 1.0f/60.0f;
            // todo: if frame_time was big (e.g. because app running slow or
            // we took a screenshot or are recording a video, then things might
            // not behave the way you want)
            // todo: do we want frame_time for app to be different from imgui?
            #else
            input.frame_time = (float)(glfwGetTime() - last_elapsed_time);
            #endif
            last_elapsed_time = glfwGetTime();
            input.recording_video = framegrab.active;
        }

        ImGui_ImplGlfw_NewFrame();

        // Frame preamble
        {
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        }

        UpdateAndDraw(input);

        OneTimeEvent(escape_button, glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        OneTimeEvent(enter_button, glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        bool escape_eaten = false;

        // Set window size dialog
        {
            using namespace ImGui;
            OneTimeEvent(hotkey,
                         glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
                         glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
            if (hotkey)
            {
                OpenPopup("Set window size##popup");
            }
            if (BeginPopupModal("Set window size##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static int width = input.window_w;
                static int height = input.window_h;
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
                    escape_eaten = true;
                }
                EndPopup();
            }
        }

        // if (enter_button)
        // {
        //     framegrab_options_t opt = {0};
        //     opt.filename = "output.mp4";
        //     opt.alpha_channel = true;
        //     opt.draw_cursor = true;
        //     opt.draw_imgui = true;
        //     opt.use_ffmpeg = false;
        //     opt.is_video = false;
        //     opt.video_frame_cap = 120;
        //     opt.reset_num_screenshots = false;
        //     opt.reset_num_video_frames = true;
        //     StartFrameGrab(opt);
        // }

        if (framegrab.active)
        {
            framegrab_options_t opt = framegrab.options;

            // render frame and get pixel data
            unsigned char *data;
            int channels,width,height,stride;
            GLenum format;
            {
                if (opt.draw_imgui)
                {
                    ImGui::GetIO().MouseDrawCursor = opt.draw_cursor;
                    ImGui::Render();
                    ImGui::GetIO().MouseDrawCursor = true;
                }
                else
                {
                    // todo: draw a crosshair thing if we still wanted a cursor
                }
                format = opt.alpha_channel ? GL_RGBA : GL_RGB;
                channels = opt.alpha_channel ? 4 : 3;
                width = input.window_w;
                height = input.window_h;
                stride = width*channels;
                data = (unsigned char*)malloc(height*stride);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadBuffer(GL_BACK);
                glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
                if (!opt.draw_imgui)
                {
                    ImGui::Render();
                }
            }

            // write output to ffmpeg or to file
            if (opt.use_ffmpeg)
            {
                static FILE *ffmpeg = 0;
                if (!ffmpeg)
                {
                    // todo: -pix_fmt for RGB?
                    assert(opt.alpha_channel);
                    // todo: linux/osx
                    char cmd[1024];
                    sprintf(cmd, "ffmpeg -r %f -f rawvideo -pix_fmt %s -s %dx%d -i - "
                                  "-threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip %s",
                                  opt.ffmpeg_fps, // -r
                                  opt.alpha_channel ? "rgba" : "rgb", // -pix_fmt
                                  width, height, // -s
                                  opt.filename);
                    ffmpeg = _popen(cmd, "wb");
                }

                fwrite(data, height*stride, 1, ffmpeg);

                framegrab.num_video_frames++;
                if (opt.video_frame_cap && framegrab.num_video_frames == opt.video_frame_cap)
                {
                    framegrab.should_stop = true;
                }
                if (framegrab.should_stop)
                {
                    framegrab.active = false;
                    _pclose(ffmpeg);
                    ffmpeg = 0;
                }
            }
            else
            {
                bool save_as_bmp = false;
                bool save_as_png = false;

                if (strstr(opt.filename, ".png"))
                {
                    save_as_png = true;
                    save_as_bmp = false;
                }
                else if (strstr(opt.filename, ".bmp"))
                {
                    save_as_bmp = true;
                    save_as_png = false;
                }
                else
                {
                    save_as_bmp = false;
                    save_as_png = false;
                    // did user specify any extension at all?
                }

                char filename[1024];
                if (opt.is_video)
                {
                    // todo: check if filename template has %...d?
                    sprintf(filename, opt.filename, framegrab.num_video_frames);
                }
                else
                {
                    // todo: check if filename template has %...d?
                    sprintf(filename, opt.filename, framegrab.num_screenshots);
                }

                #if 1
                if (save_as_bmp)
                    printf("save bmp %s\n", filename);
                else if (save_as_png)
                    printf("save png %s\n", filename);
                else
                    printf("save bmp anyway %s\n", filename);
                #else
                if (save_as_bmp)
                    stbi_write_bmp(filename, width, height, channels, data);
                else if (save_as_png)
                    stbi_write_png(filename, width, height, channels, data+stride*(height-1), -stride);
                else
                    stbi_write_bmp(filename, width, height, channels, data);

                #endif

                if (opt.is_video)
                {
                    framegrab.num_video_frames++;
                    if (opt.video_frame_cap && framegrab.num_video_frames == opt.video_frame_cap)
                    {
                        framegrab.should_stop = true;
                    }
                    if (framegrab.should_stop)
                    {
                        framegrab.active = false;
                    }
                }
                else
                {
                    framegrab.overlay_active = true;
                    framegrab.overlay_timer = 1.0f;
                    framegrab.overlay_tex = TexImage2D(data, width, height, format, GL_UNSIGNED_BYTE);
                    framegrab.num_screenshots++;
                    framegrab.active = false;
                }
            }

            free(data);
        }
        else
        {
            // Take screenshot or video dialog
            {
                using namespace ImGui;
                OneTimeEvent(hotkey,
                    glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
                    glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);

                if (hotkey)
                {
                    OpenPopup("Take screenshot##popup");
                }
                if (BeginPopupModal("Take screenshot##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    static char filename[1024];
                    if (IsWindowAppearing())
                        SetKeyboardFocusHere();
                    InputText("Filename", filename, sizeof(filename));

                    static bool alpha = false;
                    static bool is_video = false;
                    static bool draw_imgui = false;
                    static bool draw_cursor = false;
                    Checkbox("Record video", &is_video);
                    Checkbox("Transparency (32bpp)", &alpha);
                    Checkbox("Draw GUI", &draw_imgui);
                    Checkbox("Draw cursor", &draw_cursor);
                    if (is_video)
                    {
                        static bool not_reset = false;
                        static bool use_ffmpeg = false;
                        static int frame_cap = 0;
                        static float framerate = 0;
                        Checkbox("Continue with filename number from last time", &not_reset);
                        InputInt("Number of frames", &frame_cap);
                        Checkbox("Stream to ffmpeg", &use_ffmpeg);
                        if (use_ffmpeg)
                            InputFloat("Framerate", &framerate);
                        if (framegrab.active && (Button("Stop", ImVec2(120,0)) || enter_button))
                        {
                            framegrab.should_stop = true;
                        }
                        else if (Button("Start", ImVec2(120,0)) || enter_button)
                        {
                            if (use_ffmpeg)
                            {
                                RecordVideoToFfmpeg(filename, framerate, frame_cap, draw_imgui, draw_cursor, !not_reset, alpha);
                            }
                            else
                            {
                                RecordVideoToImageSequence(filename, frame_cap, draw_imgui, draw_cursor, !not_reset, alpha);
                            }
                        }
                        SameLine();
                        if (Button("Cancel", ImVec2(120,0)))
                        {
                            CloseCurrentPopup();
                        }
                    }
                    else
                    {
                        static bool reset_screenshot_counter = false;
                        Checkbox("Reset screenshot counter", &reset_screenshot_counter);
                        if (Button("OK", ImVec2(120,0)) || enter_button)
                        {
                            TakeScreenshot(filename, draw_imgui, draw_cursor, reset_screenshot_counter, alpha);
                            CloseCurrentPopup();
                        }
                        SameLine();
                        if (Button("Cancel", ImVec2(120,0)))
                        {
                            CloseCurrentPopup();
                        }
                    }

                    if (escape_button)
                    {
                        CloseCurrentPopup();
                        escape_eaten = true;
                    }
                    EndPopup();
                }
            }
            ImGui::Render();
        }


        if (framegrab.overlay_active && !framegrab.active)
        {
            // record start_time instead
            float t0 = 1.0f - framegrab.overlay_timer;
            float t1 = 2.0f*t0;
            float t2 = 2.0f*(t0-0.2f);

            if (t1 < 0.0f) t1 = 0.0f;
            if (t1 > 1.0f) t1 = 1.0f;
            if (t2 < 0.0f) t2 = 0.0f;
            if (t2 > 1.0f) t2 = 1.0f;

            float a = 0.5f+0.5f*sinf((3.14f)*(t1-0.5f));
            float w = 1.0f - 0.2f*a;

            float b = 0.5f+0.5f*sinf((3.14f)*(t2-0.5f));
            float x = -2.0f*b*b*b*b;

            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glColor4f(1,1,1,0.4f);
            glVertex2f(-w+x,-w); glVertex2f(+w+x,-w);
            glVertex2f(+w+x,-w); glVertex2f(+w+x,+w);
            glVertex2f(+w+x,+w); glVertex2f(-w+x,+w);
            glVertex2f(-w+x,+w); glVertex2f(-w+x,-w);
            glEnd();

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, framegrab.overlay_tex);
            glBegin(GL_TRIANGLES);
            glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-w+x,-w);
            glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+w+x,-w);
            glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+w+x,+w);
            glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+w+x,+w);
            glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-w+x,+w);
            glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-w+x,-w);
            glEnd();
            glDisable(GL_TEXTURE_2D);

            glBegin(GL_TRIANGLES);
            glColor4f(1,1,1,0.3f*(1.0f-a)); glVertex2f(-w+x,-w);
            glColor4f(1,1,1,0.3f*(1.0f-a)); glVertex2f(+w+x,-w);
            glColor4f(1,1,1,0.3f*(1.0f-a)); glVertex2f(+w+x,+w);
            glColor4f(1,1,1,0.3f*(1.0f-a)); glVertex2f(+w+x,+w);
            glColor4f(1,1,1,0.3f*(1.0f-a)); glVertex2f(-w+x,+w);
            glColor4f(1,1,1,0.3f*(1.0f-a)); glVertex2f(-w+x,-w);
            glEnd();

            framegrab.overlay_timer -= input.frame_time;
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
