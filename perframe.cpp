#include "frameinput.h"
#include "transform.h"
#include "vdb_impl.cpp"

#include <stdint.h>
enum command_id_t
{
    id_view,
    id_color,
    id_line_width,
    id_point_size,
    id_path_clear,
    id_path_to,
    id_path_fill,
    id_path_stroke,
    id_text,
    id_point,
    id_line,
    id_triangle,
    id_triangle_filled,
    id_quad,
    id_quad_filled,
    id_rect,
    id_rect_filled,
    id_circle,
    id_circle_filled,
    id_count // todo: assert(id_count <= 256); // ensure id fits into uint8
};

struct command_buffer_parser_t
{
    uint8_t *data; // points to pre-allocated array of bytes
    uint32_t size; // number of bytes available in data
    uint32_t read; // number of bytes read in data

    float ReadFloat32() {
        assert((read + sizeof(float) <= size) && "Read past command buffer");
        float x = *(float*)(data + read);
        read += sizeof(float);
        return x;
    }
    uint8_t ReadUint8() {
        assert((read + sizeof(uint8_t) <= size) && "Read past command buffer");
        uint8_t x = *(uint8_t*)(data + read);
        read += sizeof(uint8_t);
        return x;
    }
    char *ReadString(uint8_t length) {
        assert((read + length <= size) && "Read past command buffer");
        char *x = (char*)(data + read);
        read += length; return x;
    }

    void Draw()
    {
        assert(data && "Data in command buffer was null");
        read = 0;
        while (read < size)
        {
            #define f ReadFloat32() // to make this part more readable
            uint8_t id = ReadUint8();
            if      (id == id_view)             vdb_view(f,f,f,f);
            else if (id == id_color)            vdb_color(f,f,f,f);
            else if (id == id_line_width)       vdb_line_width(f);
            else if (id == id_point_size)       vdb_point_size(f);
            else if (id == id_path_clear)       vdb_path_clear();
            else if (id == id_path_to)          vdb_path_to(f,f);
            else if (id == id_path_fill)        vdb_path_fill();
            else if (id == id_path_stroke)      vdb_path_stroke();
            else if (id == id_text)             vdb_text(f,f, ReadString(ReadUint8()));
            else if (id == id_point)            vdb_point(f,f);
            else if (id == id_line)             vdb_line(f,f,f,f);
            else if (id == id_triangle)         vdb_triangle(f,f,f,f,f,f);
            else if (id == id_triangle_filled)  vdb_triangle_filled(f,f,f,f,f,f);
            else if (id == id_quad)             vdb_quad(f,f,f,f,f,f,f,f);
            else if (id == id_quad_filled)      vdb_quad_filled(f,f,f,f,f,f,f,f);
            else if (id == id_rect)             vdb_rect(f,f,f,f);
            else if (id == id_rect_filled)      vdb_rect_filled(f,f,f,f);
            else if (id == id_circle)           vdb_circle(f,f,f);
            else if (id == id_circle_filled)    vdb_circle_filled(f,f,f);
            #undef f
        }
    }
};

struct command_buffer_t
{
    uint8_t *data;
    uint32_t size;
    uint32_t max_size;
};

static command_buffer_t *back_buffer = NULL;
static command_buffer_t *front_buffer = NULL;

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
