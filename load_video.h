#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "texture.h"
#include "console.h"

struct video_t
{
    bool failed;
    char *filename;
    int width;
    int height;
    int num_frames;
    char *frame_buffer;
    int decoded_frame;
    FILE *decode_proc;
    GLuint tex;
};

#define max_videos 1024
#define load_video_components 4
static video_t videos[max_videos];
static int num_videos = 0;
const int invalid_video = -1;

FILE *OpenVideoAndReadFirstFrame(const char *filename, int width, int height, char *frame_buffer)
{
    static char cmd[1024];
    // -loglevel 0: don't print progress information
    // (which would have gone to stderr but we don't want that stuff clogging up the user's console anyway)
    sprintf(cmd, "ffmpeg -loglevel 0 -i %s -f image2pipe -pix_fmt bgra -vcodec rawvideo -vf scale=%d:%d - ", filename, width, height);
    FILE *proc = _popen(cmd, "rb");
    if (!proc)
    {
        ConsoleMessage("Failed to load video %s (could not open pipe)\n", filename);
        return NULL;
    }
    if (fread(frame_buffer, width*height*load_video_components, 1, proc) == 0)
    {
        ConsoleMessage("Failed to load video %s\n", filename);
        _pclose(proc); // optionally check the return value of this?
        return NULL;
    }
    return proc;
}

int LoadVideo(const char *filename, int width, int height)
{
    // check if we have already (tried) loading it
    for (int i = 0; i < num_videos; i++)
    {
        if (strcmp(filename, videos[i].filename) == 0)
            return i;
    }

    // otherwise add a video if we can
    if (num_videos == max_videos)
    {
        ConsoleMessage("Reached maximum number of videos that can be loaded (%d), %s was not loaded.", max_videos, filename);
        return invalid_video;
    }

    video_t *video = &videos[num_videos++];
    video->filename = strdup(filename);
    video->width = width;
    video->height = height;
    video->failed = true;
    video->frame_buffer = (char*)malloc(width*height*load_video_components);
    if (!video->frame_buffer)
    {
        ConsoleMessage("Ran out of memory loading video %s\n", filename);
        return invalid_video;
    }
    video->decode_proc = OpenVideoAndReadFirstFrame(filename, width, height, video->frame_buffer);
    if (!video->decode_proc)
    {
        free(video->frame_buffer);
        return invalid_video;
    }
    video->failed = false;
    return num_videos-1;
}

GLuint GetAndBindVideoFrameTexture(int video_index, int frame)
{
    bool replay = true; // argument? or check if frame wraps?
    if (video_index < 0 && video_index >= num_videos)
        return 0;
    video_t *video = &videos[video_index];
    if (video->width <= 0 ||
        video->height <= 0 ||
        video->failed)
        return 0;

    // wrap behaviour
    if (video->num_frames > 0)
        frame = frame % video->num_frames;
    // frame = frame % video->num_frames;
    // todo: mirror
    // todo: clamp

    while (video->decoded_frame != frame)
    {
        if (fread(video->frame_buffer, video->width*video->height*load_video_components, 1, video->decode_proc))
        {
            video->decoded_frame++;
        }
        else
        {
            int status = _pclose(video->decode_proc);
            video->decode_proc = NULL;
            if (status != 0)
            {
                ConsoleMessage("Failed to decode video %s\n", video->filename);
                video->failed = true;
                return 0;
            }
            else if (replay)
            {
                printf("Reached end of video, replaying\n"); // todo: remove
                video->decode_proc = OpenVideoAndReadFirstFrame(video->filename, video->width, video->height, video->frame_buffer);
                if (!video->decode_proc)
                {
                    video->failed = true;
                    return 0;
                }
                video->num_frames = video->decoded_frame+1;
                video->decoded_frame = 0;
                frame = frame % video->num_frames; // todo: wrap behaviour argument
            }
            else
            {
                printf("Reached end of video\n");
                video->num_frames = video->decoded_frame+1;
                break; // todo: don't break if replay!
            }
        }
    }

    if (!video->tex) video->tex = TexImage2D(NULL, video->width, video->height, GL_BGRA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8);
    glBindTexture(GL_TEXTURE_2D, video->tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video->width, video->height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, video->frame_buffer);

    return video->tex;
}
