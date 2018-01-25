#include "script.h"

typedef void (*script_loop_t)(script_input_t);
static script_loop_t ScriptLoop = NULL;

void ReloadScript()
{
    ScriptLoop = NULL;

    Log("reloading script\n");
    TCCState *s;
    s = tcc_new();
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    tcc_add_library_path(s, ".");
    if (tcc_add_file(s, "../script/test.c", TCC_FILETYPE_C) == -1)
    {
        Log("failed to compile script\n");
        return;
    }

    // add API functions
    {
        tcc_add_symbol(s, "vdb_path_clear", vdb_path_clear);
        tcc_add_symbol(s, "vdb_path_line_to", vdb_path_line_to);
        tcc_add_symbol(s, "vdb_path_fill_convex", vdb_path_fill_convex);
    }

    static unsigned char *code = NULL;
    int n = tcc_relocate(s, NULL);
    if (code)
    {
        free(code);
    }
    code = (unsigned char*)malloc(n);
    if (tcc_relocate(s, code) == -1)
    {
        Log("failed to compile script\n");
        return;
    }
    ScriptLoop = (script_loop_t)tcc_get_symbol(s, "loop");
    if (!ScriptLoop)
    {
        Log("failed to compile script: where is the loop function?\n");
        return;
    }
    tcc_delete(s);
}
