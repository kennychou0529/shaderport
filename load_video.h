#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "texture.h"

struct video_t
{
    char *filename;
    int width;
    int height;
    int num_frames;
    int bytes_per_frame;
    char **frames;
    GLuint tex;
};

#define max_videos 1024
static video_t videos[max_videos];
static int num_videos = 0;
const int invalid_video = -1;

int LoadVideo(const char *filename, int width, int height)
{
    if (num_videos == max_videos)
    {
        printf("Reached maximum number of videos that can be loaded (%d), %s was not loaded.", max_videos, filename);
        return invalid_video;
    }

    for (int i = 0; i < num_videos; i++)
    {
        // todo: compare width,height and reload possibly
        if (strcmp(filename, videos[i].filename) == 0)
            return i;
    }

    video_t video = {0};
    FILE *proc = NULL;
    {
        static char cmd[1024];
        sprintf(cmd, "ffmpeg -i %s -f image2pipe -pix_fmt bgra -vcodec rawvideo -vf scale=%d:%d - ", filename, width, height);
        proc = _popen(cmd, "rb");
        if (!proc)
        {
            printf("Failed to load video %s (could not open pipe)\n", filename);
            return invalid_video;
        }
    }

    int components = 4;
    int max_frames = 10000;
    int bytes_per_frame = width*height*components;
    int num_frames = 0;

    char **frames = (char**)malloc(max_frames*sizeof(char*));
    if (!frames)
    {
        printf("Ran out of memory loading video %s (%d x %d)\n", filename, width, height);
        _pclose(proc);
        goto free_memory_return_null;
    }

    frames[0] = (char*)malloc(bytes_per_frame*sizeof(char));
    if (!frames[0])
    {
        printf("Ran out of memory loading video %s (%d x %d)\n", filename, width, height);
        _pclose(proc);
        goto free_memory_return_null;
    }

    while (fread(frames[num_frames], bytes_per_frame, 1, proc))
    {
        num_frames++;
        if (num_frames == max_frames)
        {
            printf("Reached max number of frames while loading video %s\n", filename);
            break;
        }
        frames[num_frames] = (char*)malloc(bytes_per_frame*sizeof(char));
        if (!frames[num_frames])
        {
            printf("Ran out of memory loading video %s (%d x %d)\n", filename, width, height);
            _pclose(proc);
            goto free_memory_return_null;
        }
    }
    int status = _pclose(proc);
    if (status != 0)
    {
        // todo: print ffmpeg error
        printf("Failed to load video %s\n", filename);
        goto free_memory_return_null;
    }

    printf("Loaded video %s (%dx%d %.2f MB)\n", filename, width, height, num_frames*bytes_per_frame/(1024.0f*1024.0f));
    video.filename = strdup(filename);
    video.width = width;
    video.height = height;
    video.frames = frames;
    video.num_frames = num_frames;
    video.bytes_per_frame = bytes_per_frame;
    videos[num_videos] = video;
    num_videos++;
    return num_videos - 1;

    free_memory_return_null:
    if (frames)
    {
        for (int i = 0; i < num_frames; i++)
        {
            if (frames[i])
                free(frames[i]);
        }
        free(frames);
    }
    return invalid_video;
}

#if 0
void LoadVideoTest(frame_input_t input)
{
    static video_t video = {0};
    static bool first = true;
    if (first)
    {
        video = LoadVideo("C:/Temp/images/animation1.gif", 500*2, 288*2);
        first = false;
    }
    DrawVideoFrame(&video, (int)(input.elapsed_time*6.0f), -1,-1,2,2);
}
#endif

#if 0
void LoadVideoTest(frame_input_t input)
{
    static video_t video = {0};
    static bool first = true;
    static GLuint tex = 0;
    if (first)
    {
        first = false;
        video = LoadVideo("C:/Temp/images/animation1.gif", 500*2, 288*2);
        // video = LoadVideo("C:/Writing/portfolio/4035.c5445c4f5165c_particles.mp4", 360, 240);
        // video = LoadVideo("C:/Temp/images/animation2.gif", 500, 281);
        tex = TexImage2D(NULL, video.width, video.height, GL_BGRA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8);
    }

    static int frame = 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video.width, video.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, video.frames[frame]);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_TRIANGLES);
    glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
    glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+1,-1);
    glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
    glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
    glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-1,+1);
    glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    frame = (frame + 1) % video.num_frames;
}
#endif

#if 0
void LoadVideoTest(frame_input_t input)
{
    static video_t video = {0};
    static bool first = true;
    static GLuint pbo[2] = {0};
    static GLuint tex = 0;
    if (first)
    {
        first = false;
        video = LoadVideo("C:/Temp/images/animation1.gif", 500, 288);
        // video = LoadVideo("C:/Temp/images/animation2.gif", 500*2, 281*2);

        size_t bytes_per_frame = video.width*video.height*4;
        glGenBuffers(2, pbo);

        // PREPARE SECOND PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes_per_frame, NULL, GL_STREAM_DRAW);

        // PREPARE FIRST PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes_per_frame, NULL, GL_STREAM_DRAW);

        // UPLOAD FIRST FRAME
        unsigned char* ptr = (unsigned char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        assert(ptr && "glMapBuffer failed");
        memcpy(ptr, video.frames[0], bytes_per_frame);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        // PREPARE TEXTURE STORAGE
        tex = TexImage2D(NULL, video.width, video.height, GL_BGRA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);
    }

    static int frame = 0;
    int next_frame = (frame + 1) % video.num_frames;
    static int use_index = 0;
    static int upload_index = 1;

    // COPY PREVIOUSLY UPLOADED VIDEO FRAME FROM BUFFER INTO TEXTURE STORAGE
    #if 1
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[use_index]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video.width, video.height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    }
    #endif

    // DRAW TEXTURE FOR CURRENT FRAME
    #if 1
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_TRIANGLES);
        glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
        glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+1,-1);
        glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
    #endif

    // SCHEDULE A DMA UPLOAD OF NEXT VIDEO FRAME INTO GPU BUFFER
    #if 1
    {
        int bytes_per_frame = video.width*video.height*4;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[upload_index]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes_per_frame, video.frames[next_frame], GL_STREAM_DRAW);
        // unsigned char* ptr = (unsigned char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        // assert(ptr && "glMapBuffer failed");
        // memcpy(ptr, video.frames[next_frame], bytes_per_frame);
        // glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    #endif

    use_index = (use_index + 1) % 2;
    upload_index = (use_index + 1) % 2;
    frame = (frame + 1) % video.num_frames;
}
#endif

#if 0
void LoadVideoTest(frame_input_t input)
{
    static video_t video = {0};
    static bool first = true;
    static GLuint pbo[2] = {0};
    static bool cache_valid[2] = {0};
    static GLuint tex = 0;
    if (first)
    {
        first = false;
        // video = LoadVideo("C:/Temp/images/animation1.gif", 500, 288);
        video = LoadVideo("C:/Temp/images/animation2.gif", 500, 281);
    }

    int frames_per_cache = 32;
    if (frames_per_cache > video.num_frames)
        frames_per_cache = video.num_frames; // todo: if the video is this short we should just use one PBO
    int bytes_per_frame = video.width*video.height*3;
    int bytes_per_cache = bytes_per_frame*frames_per_cache;

    if (!tex)
    {
        glGenBuffers(2, pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes_per_cache, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes_per_cache, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        tex = TexImage2D(NULL, video.width, video.height, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);

        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
            unsigned char* ptr = (unsigned char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            assert(ptr && "glMapBuffer failed");
            size_t bytes_per_frame = video.width*video.height*3;
            for (int i = 0; i < frames_per_cache; i++)
            {
                memcpy(ptr, video.frames[i], bytes_per_frame);
                ptr += bytes_per_frame;
            }
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
        cache_valid[0] = true;
    }

    static int use_index = 0;
    static int upload_index = 1;
    static int frame = 0;
    #if 0
    if (frame % frames_per_cache == 0)
    {
        printf("caching %d to %d\n", frame + frames_per_cache, frame + 2*frames_per_cache - 1);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[upload_index]);
        unsigned char* ptr = (unsigned char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        assert(ptr && "glMapBuffer failed");
        size_t bytes_per_frame = video.width*video.height*3;
        for (int i = 0; i < frames_per_cache; i++)
        {
            int frame_to_upload = (frame + i + frames_per_cache) % video.num_frames;
            memcpy(ptr, video.frames[frame_to_upload], bytes_per_frame);
            ptr += bytes_per_frame;
        }
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        use_index = (use_index + 1) % 2;
        upload_index = (upload_index + 1) % 2;
    }
    #endif

    // COPY PREVIOUSLY UPLOADED VIDEO FRAME FROM BUFFER INTO TEXTURE STORAGE
    #if 1
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[use_index]);
        size_t byte_offset = (frame % frames_per_cache)*bytes_per_frame;
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video.width, video.height, GL_RGB, GL_UNSIGNED_BYTE, (const void*)byte_offset);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    #endif

    // cycle through video
    #if 0
    frame = (frame + 1) % video.num_frames;
    #endif

    // cycle through cache
    #if 1
    frame = (frame + 1) % frames_per_cache;
    #endif

    // DRAW TEXTURE FOR CURRENT FRAME
    #if 0
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_TRIANGLES);
        glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
        glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+1,-1);
        glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
    #endif
}
#endif

// caching = upload AND transfer to texture
#if 0
void LoadVideoTest(frame_input_t input)
{
    static video_t video = {0};
    static bool first = true;
    static GLuint pbo[2] = {0};
    static bool cache_valid[2] = {0};
    static GLuint tex = 0;
    if (first)
    {
        first = false;
        // video = LoadVideo("C:/Temp/images/animation1.gif", 500, 288);
        video = LoadVideo("C:/Temp/images/animation2.gif", 500, 281);
    }

    int frames_per_cache = 8;
    if (frames_per_cache > video.num_frames)
        frames_per_cache = video.num_frames; // todo: if the video is this short we should just use one PBO
    int bytes_per_frame = video.width*video.height*3;
    int bytes_per_cache = bytes_per_frame*frames_per_cache;

    if (!tex)
    {
        glGenBuffers(2, pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes_per_cache, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes_per_cache, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        tex = TexImage2D(NULL, video.width, video.height, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA);

        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
            unsigned char* ptr = (unsigned char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            assert(ptr && "glMapBuffer failed");
            size_t bytes_per_frame = video.width*video.height*3;
            for (int i = 0; i < frames_per_cache; i++)
            {
                memcpy(ptr, video.frames[i], bytes_per_frame);
                ptr += bytes_per_frame;
            }
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
        cache_valid[0] = true;
    }

    static int use_index = 0;
    static int upload_index = 1;
    static int frame = 0;
    #if 1
    if (frame % frames_per_cache == 0)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[upload_index]);
        unsigned char* ptr = (unsigned char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        assert(ptr && "glMapBuffer failed");
        size_t bytes_per_frame = video.width*video.height*3;
        for (int i = 0; i < frames_per_cache; i++)
        {
            int frame_to_upload = (frame + i + frames_per_cache) % video.num_frames;
            memcpy(ptr, video.frames[frame_to_upload], bytes_per_frame);
            ptr += bytes_per_frame;
        }
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        use_index = (use_index + 1) % 2;
        upload_index = (upload_index + 1) % 2;
    }
    #endif

    // COPY PREVIOUSLY UPLOADED VIDEO FRAME FROM BUFFER INTO TEXTURE STORAGE
    #if 1
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[use_index]);
        size_t byte_offset = (frame % frames_per_cache)*bytes_per_frame;
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video.width, video.height, GL_RGB, GL_UNSIGNED_BYTE, (const void*)byte_offset);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    #endif

    // cycle through video
    #if 0
    frame = (frame + 1) % video.num_frames;
    #endif

    // cycle through cache
    #if 1
    frame = (frame + 1) % frames_per_cache;
    #endif

    // DRAW TEXTURE FOR CURRENT FRAME
    #if 0
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_TRIANGLES);
        glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
        glColor4f(1,1,1,1); glTexCoord2f(1,1); glVertex2f(+1,-1);
        glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(1,0); glVertex2f(+1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(0,0); glVertex2f(-1,+1);
        glColor4f(1,1,1,1); glTexCoord2f(0,1); glVertex2f(-1,-1);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
    #endif
}
#endif
