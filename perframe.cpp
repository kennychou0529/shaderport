struct frame_input_t
{
    // Note: for retina displays screen coordinates != framebuffer coordinates
    int window_x,window_y; // This is the position of the window's client area in screen coordinates
    int window_w,window_h; // This is the size of the window's client area in screen coordinates
    int framebuffer_w,framebuffer_h; // This is the size in pixels of the framebuffer in the window

    float mouse_x,mouse_y; // The position of the mouse in the client area in screen coordinates where (0,0):top-left
    float mouse_u,mouse_v; // -||- in normalized mouse coordinates where (-1,-1):bottom-left (+1,+1):top-right

    float elapsed_time;
    float frame_time; // Note: When recording video you probably want to use your own animation timer
                      // that increments at a fixed time step per loop. It is also possible that camera
                      // movement based on frame_time will explode...
    bool recording_video;
    bool lost_focus;
    bool regained_focus;
};


static frame_input_t frame_input = {0}; // global for convenience, is updated in BeforeUpdateAndDraw.

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

static int draw_string_id = 0;
void BeforeUpdateAndDraw(frame_input_t input)
{
    draw_string_id = 0;
    frame_input = input;

    ResetGLState(input);
    glClearColor(0.1f, 0.12f, 0.15f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
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

float NdcToFbX(float x_ndc)
{
    return (0.5f+0.5f*x_ndc)*frame_input.framebuffer_w;
}
float NdcToFbY(float y_ndc)
{
    return (0.5f-0.5f*y_ndc)*frame_input.framebuffer_h;
}

// void UserToNdc(float x, float y, float z, float *x_ndc, float *y_ndc, float *z_ndc)
// {
//     GLfloat P[16];
//     GLfloat M[16];
//     glGetFloatv(GL_PROJECTION_MATRIX,P);
//     glGetFloatv(GL_MODELVIEW_MATRIX,M);
//     float x_view = M[ 0]*x + M[ 1]*y + M[ 2]*z + M[ 3];
//     float y_view = M[ 4]*x + M[ 5]*y + M[ 6]*z + M[ 7];
//     float z_view = M[ 8]*x + M[ 9]*y + M[10]*z + M[11];
//     float w_view = M[12]*x + M[13]*y + M[14]*z + M[15];

//     float x_clip = P[ 0]*x_view + P[ 1]*y_view + P[ 2]*z_view + P[ 3]*w_view;
//     float y_clip = P[ 4]*x_view + P[ 5]*y_view + P[ 6]*z_view + P[ 7]*w_view;
//     float z_clip = P[ 8]*x_view + P[ 9]*y_view + P[10]*z_view + P[11]*w_view;
//     float w_clip = P[12]*x_view + P[13]*y_view + P[14]*z_view + P[15]*w_view;

//     *x_ndc = x_clip/w_clip;
//     *y_ndc = y_clip/w_clip;
//     *z_ndc = z_clip/w_clip;
// }

void UpdateAndDraw(frame_input_t input)
{
    static float anim_time = 0.0f;
    anim_time += 1.0f/60.0f;

    {
        // ImGui::GetStyle().AntiAliasedLines = false;
        // ImGui::GetStyle().AntiAliasedFill = false;
        ImDrawList *draw = ImGui::GetOverlayDrawList();
        draw->Flags = 0;
        for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            float x = NdcToFbX(-1.0f+2.0f*i/4.0f);
            float y = NdcToFbY(-1.0f+2.0f*j/4.0f);
            // draw->AddRectFilled(ImVec2(x-1,y-1), ImVec2(x+1,y+1), IM_COL32(255,255,255,255));
            draw->AddCircleFilled(ImVec2(x,y), 32.0f, IM_COL32(255,255,255,255));
        }
    }

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

    #if 1
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
