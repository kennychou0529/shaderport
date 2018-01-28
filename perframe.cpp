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
void BeforeUpdateAndDraw(frame_input_t input)
{
    frame_input = input;

    ResetGLState(input);
    glClearColor(0.1f, 0.12f, 0.15f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    vdbBeforeUpdateAndDraw(input);
}

void AfterUpdateAndDraw(frame_input_t input)
{
    ResetGLState(input);
}

void UpdateAndDraw(frame_input_t input)
{
    static float anim_time = 0.0f;
    anim_time += 1.0f/60.0f;

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

    #if 0
    {
        GLfloat data[16];
        glGetFloatv(GL_PROJECTION_MATRIX,data);
        for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            DrawStringCentered(20.0f + i*40.0f, 30.0f + j*24.0f, "%.2f", data[i*4 + j]);
        }
    }
    #endif

    ImGui::Text("The time is: %ds", (int)input.elapsed_time);
    ImGui::ShowDemoWindow();
}
