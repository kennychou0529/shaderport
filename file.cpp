#include <stdio.h>
unsigned char *ReadFileAndNullTerminate(const char *filename, unsigned int *length)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    long num_bytes = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *result = (unsigned char*)malloc(num_bytes+1);
    if (fread(result, num_bytes, 1, f) != 1)
    {
        fclose(f);
        return 0;
    }
    fclose(f);
    result[num_bytes] = 0;
    *length = (unsigned int)num_bytes;
    return result;
}
