#define STB_IMAGE_WRITE_IMPLEMENTATION
#define SO_PLATFORM_IMPLEMENTATION
#define SO_PLATFORM_IMGUI
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"
#include "so_platform_sdl.h"
#include "stb_image_write.h"
#include "tcp.c"

static bool running = false;
static bool first_tick = true;
static bool connected = false;
static int ip[4] = { 127, 0, 0, 1 };
static int port = { 8000 };

void tick_imgui(so_input input)
{
    #define KeyPress(KEY) input.keys[SO_PLATFORM_KEY(KEY)].pressed
    #define Triggered(EVENT, DURATION) \
        static float tdb_timer_##__LINE__ = 0.0f; \
        if (EVENT) tdb_timer_##__LINE__ = input.t; \
        if (input.t - tdb_timer_##__LINE__ < DURATION)

    bool escape_eaten = false;

    using namespace ImGui;
    NewFrame();

    if (first_tick)
    {
        OpenPopup("Connect##popup");
    }
    if (BeginPopupModal("Connect##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        InputInt4("IP", ip);
        InputInt("Port", &port);
        if (Button("Connect", ImVec2(120,0)) || KeyPress(RETURN))
        {
            CloseCurrentPopup();
        }
        EndPopup();
    }

    if (KeyPress(PRINTSCREEN))
    {
        OpenPopup("Take screenshot##popup");
    }
    if (BeginPopupModal("Take screenshot##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char filename[1024];
        Triggered(KeyPress(PRINTSCREEN), 1.0f)
        {
            SetKeyboardFocusHere();
        }
        InputText("Filename", filename, sizeof(filename));

        static bool checkbox;
        Checkbox("32bpp (alpha channel)", &checkbox);

        if (Button("OK", ImVec2(120,0)) || KeyPress(RETURN))
        {
            int channels = 3;
            GLenum format = GL_RGB;
            if (checkbox)
            {
                format = GL_RGBA;
                channels = 4;
            }
            int width = input.width;
            int height = input.height;
            int stride = width*channels;
            unsigned char *data = (unsigned char*)malloc(height*stride);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
            stbi_write_png(filename, width, height, channels, data+stride*(height-1), -stride);
            free(data);
            CloseCurrentPopup();
        }
        SameLine();
        if (Button("Cancel", ImVec2(120,0)))
        {
            CloseCurrentPopup();
        }
        if (KeyPress(ESCAPE))
        {
            CloseCurrentPopup();
            escape_eaten = true;
        }
        EndPopup();
    }

    if (KeyPress(ESCAPE) && !escape_eaten)
        running = false;

    Render();
}

void tick(so_input input)
{
    glViewport(0, 0, input.width, input.height);
    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

int main(int, char**)
{
    // The state of ImGui windows is remembered between sessions.
    // This path specifies the executable-relative path where the
    // information is stored.
    const char *imgui_ini_filename = "./.build/imgui.ini";

    int multisamples = 4;   // Set to > 0 to get smooth edges on points, lines and triangles.
    int alpha_bits = 8;     // Set to > 0 to be able to take screenshots with transparent backgrounds.
    int depth_bits = 24;    // Set to > 0 if you want to use OpenGL depth testing.
    int stencil_bits = 0;   // Set to > 0 if you want to use the OpenGL stencil operations.
    int gl_major = 3;
    int gl_minor = 1;
    int w = 640;
    int h = 480;
    int x = -1;
    int y = -1;

    so_openWindow("vdb", w, h, x, y, gl_major, gl_minor, multisamples, alpha_bits, depth_bits, stencil_bits);
    so_imgui_init();

    {
        #ifdef VDB_FONT
        ImGui::GetIO().Fonts->AddFontFromFileTTF(VDB_FONT);
        #endif
        ImGui::GetIO().IniFilename = imgui_ini_filename;
    }

    so_input input = {0};
    running = true;
    first_tick = true;
    while (running)
    {
        if (!so_loopWindow(&input))
            break;
        so_imgui_processEvents(input);
        tick(input);
        so_swapBuffersAndSleep(input.dt);
        first_tick = false;
    }

    return 0;
}
