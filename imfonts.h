#pragma once
#include "3rdparty/imgui.h"
#include "fonts/source_sans_pro.h"
#include "console.h"

struct font_t
{
    ImFont *imgui_handle; // NULL if unloaded
    char *filename;
    float size; // height in pixels divided by framebuffer height
                // The pixel height will necessarily change if the
                // framebuffer is resized, so we invalidate all fonts
                // if the user resizes the window.
    bool failed;
};

#define max_fonts 1024
static font_t fonts[max_fonts];
static int num_fonts = 1; // default font is always index 0
static bool fonts_loaded = false;

void InvalidateAllFonts()
{
    printf("Invalidating fonts\n");
    fonts_loaded = false;
}

// This should be called before ImGui NewFrame
void LoadFontsIfNecessary(int framebuffer_height)
{
    if (fonts_loaded)
        return;
    fonts_loaded = true;
    ImGui_ImplGlfw_InvalidateDeviceObjects();
    ImGui::GetIO().Fonts->Clear();
    {
        // hard-code the size for this font... maybe we want it to be relative?
        const float default_font_size_pixels = 18.0f;
        fonts[0].size = default_font_size_pixels/framebuffer_height;

        fonts[0].imgui_handle =
        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
            (const char*)source_sans_pro_compressed_data,
            source_sans_pro_compressed_size, default_font_size_pixels);
        fonts[0].filename = "default";
    }
    for (int i = 1; i < num_fonts; i++)
    {
        if (fonts[i].failed) // we already tried to load it, but it didn't work
            continue;
        const char *filename = fonts[i].filename;
        float pixel_height = (float)(int)(fonts[i].size*framebuffer_height);
        fonts[i].imgui_handle = ImGui::GetIO().Fonts->AddFontFromFileTTF(filename, pixel_height);
        // todo: for some reason imgui can successfully load a font with size 21888.0...
        if (!fonts[i].imgui_handle)
        {
            fonts[i].failed = true;
            ConsoleMessage("Failed to load TTF font (%s)", filename);
        }
        else
        {
            printf("loaded %s at %.2f pixels\n", filename, pixel_height);
        }
    }
    ImGui_ImplGlfw_CreateDeviceObjects();
}

int GetFont(const char *filename, float size)
{
    // find existing font at specified size...
    for (int i = 0; i < num_fonts; i++)
    {
        // todo: float comparison within epsilon?
        if (strcmp(filename, fonts[i].filename) == 0 && fonts[i].size == size)
            return i;
    }

    // ... or load it at the next opportunity (before next frame)
    // (this frame will use the default font instead, so the user
    // may see the wrong font for a slip frame...)

    if (num_fonts < max_fonts)
    {
        font_t f = {0};
        f.imgui_handle = NULL;
        f.size = size;
        f.filename = strdup(filename);
        fonts[num_fonts++] = f;
        InvalidateAllFonts(); // todo: maybe not invalidate *all* the fonts?
        return num_fonts-1;
    }
    else
    {
        // tried to use too many fonts... return default
        // todo: display message
        ConsoleMessage("Ran out of fonts. %s was not loaded, using default.", filename);
        return 0;
    }
}

ImFont *GetFontImGuiHandleOrDefault(int index)
{
    if (index < 0 || index >= num_fonts)
        return ImGui::GetFont(); // return default font, maybe assert error?
    else if (fonts[index].imgui_handle)
        return fonts[index].imgui_handle;
    else
        return ImGui::GetFont();
}
