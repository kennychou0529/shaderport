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

#include "texture.cpp"
#include "perframe.cpp"

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
    const char *filename, float fps, int frame_cap, bool imgui=false, bool cursor=false, bool alpha=false)
{
    framegrab_options_t opt = {0};
    opt.filename = filename;
    opt.draw_imgui = imgui;
    opt.draw_cursor = cursor;
    opt.alpha_channel = alpha;
    opt.is_video = true;
    opt.use_ffmpeg = true;
    opt.ffmpeg_fps = fps;
    opt.video_frame_cap = frame_cap;
    StartFrameGrab(opt);
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

#define Triggered(EVENT, DURATION)                      \
    static double tdb_timer_##__LINE__ = 0.0f;          \
    if (EVENT) tdb_timer_##__LINE__ = glfwGetTime();    \
    if (glfwGetTime() - tdb_timer_##__LINE__ < DURATION)

#define OneTimeEvent(VAR, EVENT)                     \
    static bool VAR##_was_active = (EVENT);          \
    bool VAR##_is_active = (EVENT);                  \
    bool VAR = VAR##_is_active && !VAR##_was_active; \
    VAR##_was_active = VAR##_is_active;

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
    glfwGetFramebufferSize(window, &input.window_w, &input.window_h);

    double mouse_x,mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    input.mouse_x = (int)mouse_x;
    input.mouse_y = (int)mouse_y;

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

void DrawScreenshotTakenOverlayAnimation(float overlay_timer, GLuint overlay_tex)
{
    float t0 = 1.0f - overlay_timer;
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
    glBindTexture(GL_TEXTURE_2D, overlay_tex);
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

    // todo: draw an overlay once we figure out how to draw imgui stuff that both is and isn't captured
    // {
    //     ImGui::SetNextWindowPos(ImVec2(10,10));
    //     ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
    //     if (ImGui::Begin("Example: Fixed Overlay", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings))
    //     {
    //         ImGui::Text("Simple overlay\nin the corner of the screen.\n(right-click to change position)");
    //         ImGui::Separator();
    //         ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
    //         ImGui::End();
    //     }
    // }
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

void StartFramegrabDialog(bool *escape_eaten, bool screenshot_button, bool enter_button, bool escape_button)
{
    using namespace ImGui;

    if (screenshot_button)
    {
        OpenPopup("Take screenshot##popup");
    }
    if (BeginPopupModal("Take screenshot##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char filename[1024] = { 's','c','r','e','e','n','s','h','o','t','%','0','4','d','.','p','n','g',0 };
        if (IsWindowAppearing())
            SetKeyboardFocusHere();
        InputText("Filename", filename, sizeof(filename));

        static bool alpha = false;
        static int mode = 0;
        const int mode_single = 0;
        const int mode_sequence = 1;
        const int mode_ffmpeg = 2;
        static bool draw_imgui = false;
        static bool draw_cursor = false;
        RadioButton("Screenshot", &mode, mode_single);
        SameLine();
        ShowHelpMarker("Take a single screenshot. Put a %d in the filename to use the counter for successive screenshots.");
        SameLine();
        RadioButton("Sequence", &mode, mode_sequence);
        SameLine();
        ShowHelpMarker("Record a video of images in succession (e.g. output0000.png, output0001.png, ... etc.). Put a %d in the filename to get frame numbers. Use %0nd to left-pad with n zeroes.");
        SameLine();
        RadioButton("ffmpeg", &mode, mode_ffmpeg);
        SameLine();
        ShowHelpMarker("Record a video with raw frames piped directly to ffmpeg, and save the output in the format specified by your filename extension (e.g. mp4). This option can be quicker as it avoids writing to the disk.\nMake sure the 'ffmpeg' executable is visible from the terminal you launched this program in.");

        Checkbox("Alpha (32bpp)", &alpha);
        SameLine();
        Checkbox("Draw GUI", &draw_imgui);
        SameLine();
        Checkbox("Draw cursor", &draw_cursor);

        if (mode == mode_single)
        {
            static bool do_continue = true;
            Checkbox("Continue counting", &do_continue);
            SameLine();
            ShowHelpMarker("Enable this to continue the image filename number suffix from the last screenshot captured (in this program session).");
            if (Button("OK", ImVec2(120,0)) || enter_button)
            {
                TakeScreenshot(filename, draw_imgui, draw_cursor, !do_continue, alpha);
                CloseCurrentPopup();
            }
            SameLine();
            if (Button("Cancel", ImVec2(120,0)))
            {
                CloseCurrentPopup();
            }
        }
        else if (mode == mode_sequence || mode == mode_ffmpeg)
        {
            static bool do_continue = false;
            static int frame_cap = 0;
            static float framerate = 60;
            InputInt("Number of frames", &frame_cap);
            SameLine();
            ShowHelpMarker("0 for unlimited. To stop the recording at any time, press the same hotkey you used to open this dialog (CTRL+S by default).");

            if (mode == mode_sequence)
            {
                Checkbox("Continue from last frame", &do_continue);
                SameLine();
                ShowHelpMarker("Enable this to continue the image filename number suffix from the last image sequence that was recording (in this program session).");
            }
            else if (mode == mode_ffmpeg)
            {
                InputFloat("Framerate", &framerate);
            }

            if (framegrab.active && (Button("Stop", ImVec2(120,0)) || enter_button))
            {
                framegrab.should_stop = true;
            }
            else if (Button("Start", ImVec2(120,0)) || enter_button)
            {
                if (mode == mode_ffmpeg)
                {
                    RecordVideoToFfmpeg(filename, framerate, frame_cap, draw_imgui, draw_cursor, alpha);
                }
                else
                {
                    RecordVideoToImageSequence(filename, frame_cap, draw_imgui, draw_cursor, !do_continue, alpha);
                }
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
            *escape_eaten = true;
        }
        EndPopup();
    }
}

void FramegrabSaveOutput(unsigned char *data, int width, int height, int stride, int channels, GLenum format)
{
    framegrab_options_t opt = framegrab.options;

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

        if (save_as_bmp)
        {
            stbi_write_bmp(filename, width, height, channels, data);
            printf("Saved %s...\n", filename);
        }
        else if (save_as_png)
        {
            stbi_write_png(filename, width, height, channels, data+stride*(height-1), -stride);
            printf("Saved %s...\n", filename);
        }
        else
        {
            stbi_write_bmp(filename, width, height, channels, data);
            printf("Saved %s (bmp)...\n", filename);
        }

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

        SetWindowSizeDialog(&escape_eaten, window, input, window_size_button, enter_button, escape_button);

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

            FramegrabSaveOutput(data, width, height, stride, channels, format);

            free(data);

            if (screenshot_button)
            {
                framegrab.should_stop = true;
            }
        }
        else
        {
            // todo: figure out how to render this dialog box on top of everything else
            // but not have it in the screenshot
            StartFramegrabDialog(&escape_eaten, screenshot_button, enter_button, escape_button);
            ImGui::Render();
        }

        if (framegrab.overlay_active)
        {
            DrawScreenshotTakenOverlayAnimation(framegrab.overlay_timer, framegrab.overlay_tex);

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
