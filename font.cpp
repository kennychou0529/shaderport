#include "stb_truetype.h"

// todo: if you change this remember to update Text and TextLength
stbtt_bakedchar font_atlas_chars[96]; // ASCII 32..126 is 95 glyphs
GLuint font_atlas_texture;
const int font_atlas_width = 1024;
const int font_atlas_height = 1024;

float WidthOfString(const char *text)
{
    float x = 0.0f;
    float y = 0.0f;
    while (*text)
    {
       if (*text >= 32 && *text < 128)
       {
          stbtt_aligned_quad q;
          stbtt_GetBakedQuad(font_atlas_chars, font_atlas_width, font_atlas_height, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
       }
       ++text;
    }
    return x;
}

bool LoadFont(const char *filename, float pixel_height)
{
    static unsigned char bitmap[font_atlas_width*font_atlas_height];
    unsigned int ttf_length;
    unsigned char *ttf = ReadFileAndNullTerminate(filename, &ttf_length);
    if (!ttf)
    {
        printf("Failed to load font file %s.\n", filename);
        return false;
    }
    int fit = stbtt_BakeFontBitmap(ttf, 0, pixel_height, bitmap, font_atlas_width, font_atlas_height, 32,96, font_atlas_chars);
    if (fit <= 0)
    {
        printf("Failed to pack %.2fpt font (%s) into %dx%d bitmap. Try increasing the resolution.\n", pixel_height, filename, font_atlas_width, font_atlas_height);
        return false;
    }
    free(ttf);

    glGenTextures(1, &font_atlas_texture);
    glBindTexture(GL_TEXTURE_2D, font_atlas_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, font_atlas_width, font_atlas_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
}

void DrawString(float x, float y, const char *text, unsigned char options=0)
{
    float length = WidthOfString(text);
    if (options & AlignCenterX)
        x -= length*0.5f;

    mat4 pvm = _pvm;
    Ortho(0.0f, window_w, window_h, 0.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font_atlas_texture);
    glBegin(GL_QUADS);
    while (*text)
    {
       if (*text >= 32 && *text < 128)
       {
          stbtt_aligned_quad q;
          stbtt_GetBakedQuad(font_atlas_chars, font_atlas_width, font_atlas_height, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
          glColor4f(1.0f,1.0f,0.2f,1.0f);glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
          glColor4f(1.0f,1.0f,0.2f,1.0f);glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
          glColor4f(1.0f,1.0f,0.2f,1.0f);glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
          glColor4f(1.0f,1.0f,0.2f,1.0f);glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
       }
       ++text;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    PVM(pvm);
}
