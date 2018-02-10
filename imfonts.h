#pragma once
#include "3rdparty/imgui.h"
#include "fonts/source_sans_pro.h"
#include "console.h"

struct font_t
{
    ImFont *imgui_handle; // NULL if unloaded
    char *filename; // memory allocated in AddDefaultFont or GetFont
    float load_size;
    bool failed;
};

#define max_fonts 1024
static font_t fonts[max_fonts];
static int num_fonts = 0;

void AddDefaultFont(float size)
{
    font_t f = {0};
    f.imgui_handle =
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
        (const char*)source_sans_pro_compressed_data,
        source_sans_pro_compressed_size, size);
    f.filename = strdup("default");
    f.load_size = size;
    fonts[num_fonts++] = f;
}

// This should be called before ImGui NewFrame
void LoadUnloadedFonts()
{
    bool invalidated = false;
    for (int i = 0; i < num_fonts; i++)
    {
        if (!fonts[i].imgui_handle && !fonts[i].failed)
        {
            const char *filename = fonts[i].filename;
            float size = fonts[i].load_size;
            fonts[i].imgui_handle = ImGui::GetIO().Fonts->AddFontFromFileTTF(filename, size);
            if (!fonts[i].imgui_handle)
            {
                fonts[i].failed = true;
                ConsoleMessage("Failed to load TTF font (%s)", filename);
            }
            else
            {
                if (!invalidated)
                {
                    ImGui_ImplGlfw_InvalidateDeviceObjects();
                    invalidated = true;
                }
            }
        }
    }
    if (invalidated)
        ImGui_ImplGlfw_CreateDeviceObjects();
}

// todo: load same font at different size
int GetFont(const char *filename, float size)
{
    for (int i = 0; i < num_fonts; i++)
    {
        if (strcmp(filename, fonts[i].filename) == 0)
            return i;
    }

    // otherwise we should add the font to the list and mark it as unloaded
    // to be loaded at the next opportunity from main.cpp

    if (num_fonts < max_fonts)
    {
        font_t f = {0};
        f.imgui_handle = NULL;
        f.load_size = size;
        f.filename = strdup(filename);
        fonts[num_fonts++] = f;
        return num_fonts-1;
    }
    else
    {
        // tried to use too many fonts... return default
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
