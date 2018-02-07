// todo: create st2 build script that calls shaderport.exe ${FILEPATH}
// existing shaderport instance then reads from stdin the filename to
// compile and compiles it. if new instance, it runs vcvarsall x86_amd64
// and compiles it.

#pragma once

typedef void script_loop_t();
static script_loop_t *ScriptLoop = NULL;

bool FileExists(const char *filename)
{
    WIN32_FIND_DATA data;
    HANDLE handle = FindFirstFileA(filename, &data);
    if (handle != INVALID_HANDLE_VALUE)
    {
        FindClose(handle);
        return true;
    }
    else
    {
        return false;
    }
}

FILETIME FileLastWriteTime(const char *filename)
{
    FILETIME t = {};
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFileA(filename, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        t = fd.ftLastWriteTime;
        FindClose(h);
    }
    return t;
}

// todo: do we want the user to compile, or do we check for file update and compile?
void ReloadScript(const char *dll_filename)
{
    // compile using msvc
    {
        static bool first = true;
        if (first)
        {
            system("vcvarsall x86_amd64");
            first = false;
        }
        system("del script.dll > NUL 2> NUL");
        system("del script_in_use.dll > NUL 2> NUL");
        system("del script*.pdb > NUL 2> NUL");
        system("echo WAITING FOR PDB > lock.tmp");
        system("cl "
               "-Zi -nologo -Oi -Od -WX -W4 -wd4505 -wd4189 -wd4100 -fp:fast "
               "../script/script.cpp /link "
               "-debug -DLL -opt:ref -PDB:script_%%random%%.pdb -export:loop "
               "-out:script.dll");
    }

    // We store a handle to the DLL in order to free it
    // later, when we need to reload.
    static HMODULE handle = NULL;

    // We need to unload the library in order to copy the user's
    // dll and overwrite the dll that we're using/going to use.
    if (handle)
    {
        FreeLibrary(handle);
        handle = NULL;
    }

    if (!FileExists(dll_filename))
    {
        printf("Failed to reload script: could not find dll\n");
        return;
    }

    // First, we must rename the DLL file that our
    // game code resides in so that we can overwrite
    // it in a later compilation. If we do not rename
    // it, the vc compiler would not be able to overwrite
    // the DLL, because hey, we're using it.
    const char *temp_filename = "script_in_use.dll";
    while (!CopyFile(dll_filename, temp_filename, FALSE))
    {
        // Try again... I guess the FreeLibrary call might be a bit slow?
    }

    // Next, we get the function pointer addresses by
    // loading the library and asking for the pointers.
    handle = LoadLibrary(temp_filename);
    if (!handle)
    {
        printf("Failed to reload script: LoadLibrary failed\n");
        return;
    }

    ScriptLoop = (script_loop_t*)GetProcAddress(handle, "loop");

    if (!ScriptLoop)
    {
        printf("Failed to reload script: could not find routine 'loop'\n");
    }
}

void ScriptUpdateAndDraw(frame_input_t input, bool reload)
{
    static FILETIME last_write_time = {0};
    const char *script_filename = "C:/Programming/shaderport/script/script.cpp";
    if (FileExists(script_filename))
    {
        FILETIME write_time = FileLastWriteTime(script_filename);
        if (CompareFileTime(&write_time, &last_write_time) != 0)
        {
            last_write_time = write_time;
            ReloadScript("script.dll");
        }
    }

    // if (reload)
        // ReloadScript("script.dll");
    if (ScriptLoop)
        ScriptLoop();
}
