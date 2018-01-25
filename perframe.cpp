#include "frameinput.h"
#include "transform.h"

void ResetGLState(frame_input_t input)
{
    glUseProgram(0);

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
}

static frame_input_t frame_input = {0};
static int draw_string_id = 0;
static ImDrawList *user_draw_list = NULL;
void BeforeUpdateAndDraw(frame_input_t input)
{
    draw_string_id = 0;
    frame_input = input;

    ResetGLState(input);
    glClearColor(0.1f, 0.12f, 0.15f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("##UserDrawWindow", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings);
    user_draw_list = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void AfterUpdateAndDraw(frame_input_t input)
{
    ResetGLState(input);
}

// todo: are x,y screen or framebuffer coordinates?
void DrawStringCenteredUnformatted(float x, float y, const char *text)
{
    ImVec2 text_size = ImGui::CalcTextSize(text);
    x -= text_size.x*0.5f;
    y -= text_size.y*0.5f;
    #if 1
    char id[32];
    sprintf(id, "##DrawString%d", draw_string_id++);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,1.0f,1.0f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f,0.0f,0.0f,0.4f));
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::Begin(id, 0, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text(text);
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    #else
    // this will render on top of everything, which I don't really want!
    ImDrawList *draw = ImGui::GetOverlayDrawList();
    draw->AddText(ImVec2(x+1,y+1), IM_COL32(0,0,0,255), text);
    draw->AddText(ImVec2(x,y), IM_COL32(255,255,255,255), text);
    #endif
}

void DrawStringCentered(float x, float y, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (w == -1 || w >= (int)sizeof(buffer))
        w = (int)sizeof(buffer) - 1;
    buffer[w] = 0;
    DrawStringCenteredUnformatted(x, y, buffer);
    va_end(args);
}

// todo: put these in a more logical place
void vdb_path_clear()
{
    user_draw_list->PathClear();
}
void vdb_path_line_to(float x, float y)
{
    user_draw_list->PathLineTo(ImVec2(NdcToFbX(x), NdcToFbY(y)));
}
void vdb_path_fill_convex(float r, float g, float b, float a)
{
    user_draw_list->PathFillConvex(IM_COL32((int)r, (int)g, (int)b, (int)a));
}

void UpdateAndDraw(frame_input_t input)
{
    static float anim_time = 0.0f;
    anim_time += 1.0f/60.0f;

    // test running a script!
    #if 1
    if (ScriptLoop)
    {
        script_input_t s = {0};
        s.window_x = input.window_x;
        s.window_y = input.window_y;
        s.window_w = input.window_w;
        s.window_h = input.window_h;
        s.framebuffer_w = input.framebuffer_w;
        s.framebuffer_h = input.framebuffer_h;
        s.mouse_x = input.mouse_x;
        s.mouse_y = input.mouse_y;
        s.mouse_u = input.mouse_u;
        s.mouse_v = input.mouse_v;
        s.elapsed_time = input.elapsed_time;
        s.frame_time = input.frame_time;
        s.recording_video = input.recording_video;
        s.lost_focus = input.lost_focus;
        s.regained_focus = input.regained_focus;
        ScriptLoop(s);
    }
    #endif

    // test using imgui draw api
    #if 0
    {
        ImDrawList *draw = user_draw_list;
        // draw->Flags = 0; // Disable anti-aliasing on both lines and fill shapes. Todo: optional
        for (int j = 0; j < 4; j++)
        {
            // todo: user coordinates to imgui coordinates
            draw->PathClear();
            draw->PathLineTo(ImVec2(NdcToFbX(-1.0f), NdcToFbY(-1.0f)));
            float t = anim_time;
            for (int i = 0; i <= 128; i++)
            {
                float s = i/128.0f;
                float x = -1.0f+2.0f*s;
                float f = 1.0f - j*0.25f;
                float p = (1.0f + j*0.25f)*s + j*3.14f/2.0f;
                float y = 0.8f+0.1f*sinf(p + f*t)-0.45f*j;
                draw->PathLineTo(ImVec2(NdcToFbX(x), NdcToFbY(y)));

            }
            draw->PathLineTo(ImVec2(NdcToFbX(+1.0f), NdcToFbY(-1.0f)));
                 if (j == 0) draw->PathFillConvex(IM_COL32(20,20,20,255));
            else if (j == 1) draw->PathFillConvex(IM_COL32(40,40,40,255));
            else if (j == 2) draw->PathFillConvex(IM_COL32(80,80,80,255));
            else if (j == 3) draw->PathFillConvex(IM_COL32(120,120,120,255));
        }

        for (int i = 0; i <= 4; i++)
        for (int j = 0; j <= 4; j++)
        {
            float x = NdcToFbX(-1.0f+2.0f*i/4.0f);
            float y = NdcToFbY(-1.0f+2.0f*j/4.0f);
            // draw->AddRectFilled(ImVec2(x-1,y-1), ImVec2(x+1,y+1), IM_COL32(255,255,255,255));
            draw->AddCircleFilled(ImVec2(x,y), 4.0f, IM_COL32(255,255,255,255));
        }
    }
    #endif

    #if 0
    static bool first = true;
    if (first)
    {
        int x,y,n;
        unsigned char *data = stbi_load("C:/Temp/dummy.png", &x, &y, &n, 3);
        SetTexture(0, data, x, y, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR);
        first = false;
    }

    DrawTexture(0);
    #endif

    #if 1
    DrawStringCentered(NdcToFbX(-0.5f),NdcToFbY(-0.5f), "lower-left");
    DrawStringCentered(NdcToFbX(+0.5f),NdcToFbY(-0.5f), "lower-right");
    DrawStringCentered(NdcToFbX(+0.5f),NdcToFbY(+0.5f), "upper-right");
    DrawStringCentered(NdcToFbX(-0.5f),NdcToFbY(+0.5f), "upper-left");
    #endif

    #if 0
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
        for (int i = 0; i <= 8; i++)
        {
            float s = i/8.0f;
            float x = -1.0f+2.0f*s;
            float f = 1.0f - j*0.25f;
            float p = (1.0f + j*0.25f)*s + j*3.14f/2.0f;
            float y = 0.8f+0.1f*sinf(p + f*t)-0.45f*j;
            glVertex2f(x, y);

        }
        glVertex2f(+1.0f,-1.0f);
        glEnd();
    }
    #endif

    {
        GLfloat data[16];
        glGetFloatv(GL_PROJECTION_MATRIX,data);
        for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            DrawStringCentered(20.0f + i*40.0f, 30.0f + j*24.0f, "%.2f", data[i*4 + j]);
        }
    }

    ImGui::Text("The time is: %ds", (int)input.elapsed_time);
    ImGui::ShowDemoWindow();
}
