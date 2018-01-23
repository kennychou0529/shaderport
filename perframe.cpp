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

void BeforeUpdateAndDraw(frame_input_t input)
{
    ResetGLState(input);
    glClearColor(0.1f, 0.12f, 0.15f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void AfterUpdateAndDraw(frame_input_t input)
{
    ResetGLState(input);
}

// todo: are x,y screen or framebuffer coordinates?
void DrawString(float x, float y, const char *text)
{
    ImDrawList *draw = ImGui::GetOverlayDrawList();
    draw->AddText(ImVec2(x+1,y+1), IM_COL32(0,0,0,255), text);
    draw->AddText(ImVec2(x,y), IM_COL32(255,255,255,255), text);
}

void UpdateAndDraw(frame_input_t input)
{
    static float anim_time = 0.0f;
    anim_time += 1.0f/60.0f;

    static bool first = true;
    if (first)
    {
        int x,y,n;
        unsigned char *data = stbi_load("C:/Temp/dummy.png", &x, &y, &n, 3);
        SetTexture(0, data, x, y, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR);
        first = false;
    }

    DrawTexture(0);

    DrawString(200,200, "hello sailor!");

    // for (int j = 0; j < 4; j++)
    // {
    //     glBegin(GL_TRIANGLE_FAN);
    //     if (j == 0)
    //         glColor4f(0.19f, 0.2f, 0.25f, 1.0f);
    //     else if (j == 1)
    //         glColor4f(0.38f, 0.31f, 0.51f, 1.0f);
    //     else if (j == 2)
    //         glColor4f(0.29f, 0.22f, 0.38f, 1.0f);
    //     else if (j == 3)
    //         glColor4f(0.11f, 0.11f, 0.12f, 1.0f);
    //     glVertex2f(-1.0f,-1.0f);
    //     float t = anim_time;
    //     for (int i = 0; i <= 128; i++)
    //     {
    //         float s = i/128.0f;
    //         float x = -1.0f+2.0f*s;
    //         float f = 1.0f - j*0.25f;
    //         float p = (1.0f + j*0.25f)*s + j*3.14f/2.0f;
    //         glVertex2f(x, 0.8f+0.1f*sinf(p + f*t)-0.45f*j);
    //     }
    //     glVertex2f(+1.0f,-1.0f);
    //     glEnd();
    // }

    // glPointSize(32.0f);
    // glBegin(GL_POINTS);
    // glColor4f(1.0f,1.0f,1.0f,0.3f);
    // glVertex2f(0.0f,0.0f);
    // glEnd();

    ImGui::Text("The time is: %ds", (int)input.elapsed_time);
    ImGui::ShowDemoWindow();
}
