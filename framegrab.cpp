#include "frameinput.h"
#include "texture.h"

struct framegrab_options_t
{
    const char *filename;
    bool alpha_channel;
    bool draw_cursor;
    bool draw_imgui;
    bool is_video;
    bool use_ffmpeg;
    float ffmpeg_fps;
    int ffmpeg_crf;
    bool reset_num_screenshots;
    bool reset_num_video_frames;
    int start_from;
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

void StopFramegrab()
{
    framegrab.should_stop = true;
}

void StartFrameGrab(framegrab_options_t opt)
{
    framegrab.options = opt;
    if (opt.reset_num_screenshots)
        framegrab.num_screenshots = opt.start_from;
    if (opt.reset_num_video_frames)
        framegrab.num_video_frames = opt.start_from;
    framegrab.active = true;
    framegrab.should_stop = false;
}

void TakeScreenshot(
    const char *filename, bool imgui, bool cursor, bool reset, int start_from, bool alpha)
{
    framegrab_options_t opt = {0};
    opt.filename = filename;
    opt.reset_num_screenshots = reset;
    opt.start_from = start_from;
    opt.draw_imgui = imgui;
    opt.draw_cursor = cursor;
    opt.alpha_channel = alpha;
    StartFrameGrab(opt);
}

void RecordVideoToImageSequence(
    const char *filename, int frame_cap, bool imgui, bool cursor, bool reset, int start_from, bool alpha)
{
    framegrab_options_t opt = {0};
    opt.filename = filename;
    opt.reset_num_video_frames = reset;
    opt.start_from = start_from;
    opt.draw_imgui = imgui;
    opt.draw_cursor = cursor;
    opt.alpha_channel = alpha;
    opt.is_video = true;
    opt.video_frame_cap = frame_cap;
    StartFrameGrab(opt);
}

void RecordVideoToFfmpeg(
    const char *filename, float fps, int crf, int frame_cap, bool imgui, bool cursor, bool alpha)
{
    framegrab_options_t opt = {0};
    opt.filename = filename;
    opt.draw_imgui = imgui;
    opt.draw_cursor = cursor;
    opt.alpha_channel = alpha;
    opt.is_video = true;
    opt.use_ffmpeg = true;
    opt.ffmpeg_fps = fps;
    opt.ffmpeg_crf = crf;
    opt.video_frame_cap = frame_cap;
    StartFrameGrab(opt);
}

void FramegrabShowDialog(bool *escape_eaten, bool screenshot_button, bool enter_button, bool escape_button)
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
            static int start_from = 0;
            Checkbox("Continue counting", &do_continue);
            SameLine();
            ShowHelpMarker("Enable this to continue the image filename number suffix from the last screenshot captured (in this program session).");
            if (!do_continue)
            {
                SameLine();
                PushItemWidth(100.0f);
                InputInt("Start from", &start_from);
            }
            if (Button("OK", ImVec2(120,0)) || enter_button)
            {
                TakeScreenshot(filename, draw_imgui, draw_cursor, !do_continue, start_from, alpha);
                CloseCurrentPopup();
            }
            SameLine();
            if (Button("Cancel", ImVec2(120,0)))
            {
                CloseCurrentPopup();
            }
        }
        else if (mode == mode_sequence)
        {
            static bool do_continue = false;
            static int start_from = 0;
            static int frame_cap = 0;
            InputInt("Number of frames", &frame_cap);
            SameLine();
            ShowHelpMarker("0 for unlimited. To stop the recording at any time, press the same hotkey you used to open this dialog (CTRL+S by default).");

            Checkbox("Continue from last frame", &do_continue);
            SameLine();
            ShowHelpMarker("Enable this to continue the image filename number suffix from the last image sequence that was recording (in this program session).");
            if (!do_continue)
            {
                SameLine();
                PushItemWidth(100.0f);
                InputInt("Start from", &start_from);
            }

            if (Button("Start", ImVec2(120,0)) || enter_button)
            {
                RecordVideoToImageSequence(filename, frame_cap, draw_imgui, draw_cursor, !do_continue, start_from, alpha);
            }
            SameLine();
            ShowHelpMarker("Press ESCAPE or CTRL+S to stop.");
            SameLine();
            if (Button("Cancel", ImVec2(120,0)))
            {
                CloseCurrentPopup();
            }
        }
        else if (mode == mode_ffmpeg)
        {
            static int frame_cap = 0;
            static float framerate = 60;
            static int crf = 21;
            InputInt("Number of frames", &frame_cap);
            SameLine();
            ShowHelpMarker("0 for unlimited. To stop the recording at any time, press the same hotkey you used to open this dialog (CTRL+S by default).");
            SliderInt("Quality (lower is better)", &crf, 1, 51);
            InputFloat("Framerate", &framerate);

            if (Button("Start", ImVec2(120,0)) || enter_button)
            {
                RecordVideoToFfmpeg(filename, framerate, crf, frame_cap, draw_imgui, draw_cursor, alpha);
            }
            SameLine();
            ShowHelpMarker("Press ESCAPE or CTRL+S to stop.");
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

void FramegrabSaveOutput(unsigned char *data, int width, int height, int channels, GLenum format)
{
    framegrab_options_t opt = framegrab.options;

    // write output to ffmpeg or to file
    if (opt.use_ffmpeg)
    {
        static FILE *ffmpeg = 0;
        if (!ffmpeg)
        {
            // todo: linux/osx
            char cmd[1024];
            sprintf(cmd, "ffmpeg -r %f -f rawvideo -pix_fmt %s -s %dx%d -i - "
                          "-threads 0 -preset fast -y -pix_fmt yuv420p -crf %d -vf vflip %s",
                          opt.ffmpeg_fps, // -r
                          opt.alpha_channel ? "rgba" : "rgb24", // -pix_fmt
                          width, height, // -s
                          opt.ffmpeg_crf, // -crf
                          opt.filename);
            ffmpeg = _popen(cmd, "wb");
        }

        fwrite(data, width*height*channels, 1, ffmpeg);

        framegrab.num_video_frames++;
        if (opt.video_frame_cap && framegrab.num_video_frames == opt.video_frame_cap)
        {
            StopFramegrab();
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
            int stride = width*channels;
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
                StopFramegrab();
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

void FramegrabDrawScreenshotAnimation(float overlay_timer, GLuint overlay_tex)
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

void FramegrabDrawCrosshair(frame_input_t input)
{
    float aspect = (float)input.framebuffer_w/input.framebuffer_h;
    float w = 0.05f;
    float h = w*aspect;
    float dx = 2.0f/input.framebuffer_w;
    float dy = 2.0f/input.framebuffer_h;

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor4f(0.0f,0.0f,0.0f,1.0f);
    glVertex2f(input.mouse_u + dx, input.mouse_v-h - dy);
    glVertex2f(input.mouse_u + dx, input.mouse_v+h - dy);
    glVertex2f(input.mouse_u-w + dx, input.mouse_v - dy);
    glVertex2f(input.mouse_u+w + dx, input.mouse_v - dy);
    glEnd();

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glVertex2f(input.mouse_u, input.mouse_v-h);
    glVertex2f(input.mouse_u, input.mouse_v+h);
    glVertex2f(input.mouse_u-w, input.mouse_v);
    glVertex2f(input.mouse_u+w, input.mouse_v);
    glEnd();
}
