struct frame_input_t
{
    int window_w;
    int window_h;
    int window_x;
    int window_y;
    int mouse_x;
    int mouse_y;
    float elapsed_time;
    float frame_time; // Note: When recording video you probably want to use your own animation timer
                      // that increments at a fixed time step per loop. It is also possible that camera
                      // movement based on frame_time will explode...
    bool recording_video;
    bool lost_focus;
    bool regained_focus;
};

void BeforeUpdateAndDraw(frame_input_t input)
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}

void AfterUpdateAndDraw(frame_input_t input)
{
    // glUseProgram(0);
    // todo: set up fixed-function pipeline stuff
    // e.g. to support DrawScreenshotTakenOverlayAnimation
}

void UpdateAndDraw(frame_input_t input)
{
    static float anim_time = 0.0f;
    anim_time += 1.0f/60.0f;
    glViewport(0, 0, input.window_w, input.window_h);
    glClearColor(0.1f, 0.12f, 0.15f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    static bool first = true;
    if (first)
    {
        int x,y,n;
        unsigned char *data = stbi_load("C:/Temp/dummy.png", &x, &y, &n, 3);
        SetTexture(0, data, x, y, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
        first = false;
    }

    DrawTexture(0);

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
