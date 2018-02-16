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
        printf("Failed to load video %s (could not open pipe)\n", filename);
        return NULL;
    }
    if (fread(frame_buffer, width*height*load_video_components, 1, proc) == 0)
    {
        printf("Failed to load video %s\n", filename);
        _pclose(proc); // optionally check the return value of this?
        return NULL;
    }
    return proc;
}

int LoadVideo(const char *filename, int width, int height)
{
    if (num_videos == max_videos)
    {
        printf("Reached maximum number of videos that can be loaded (%d), %s was not loaded.", max_videos, filename);
        return invalid_video;
    }

    for (int i = 0; i < num_videos; i++)
    {
        if (strcmp(filename, videos[i].filename) == 0)
            return i;
    }

    char *frame_buffer = (char*)malloc(width*height*load_video_components);
    if (!frame_buffer)
    {
        printf("Ran out of memory loading video %s\n", filename);
        return invalid_video;
    }

    FILE *decode_proc = OpenVideoAndReadFirstFrame(filename, width, height, frame_buffer);
    if (!decode_proc)
        return invalid_video;

    video_t video = {0};
    video.filename = strdup(filename);
    video.width = width;
    video.height = height;
    video.decode_proc = decode_proc;
    video.frame_buffer = frame_buffer;
    video.decoded_frame = 0;
    videos[num_videos] = video;
    num_videos++;
    return num_videos - 1;
}

void DrawVideoFrame(int video_index, int frame, float x, float y, float w, float h)
{
    bool replay = true; // argument? or check if frame wraps?
    if (video_index < 0 && video_index >= num_videos)
        return;
    video_t *video = &videos[video_index];
    if (video->width <= 0 ||
        video->height <= 0)
        return;

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
                printf("Failed to decode video %s\n", video->filename);
                return;
            }
            else if (replay)
            {
                printf("Reached end of video, replaying\n");
                video->decode_proc = OpenVideoAndReadFirstFrame(video->filename, video->width, video->height, video->frame_buffer);
                if (!video->decode_proc)
                    return;
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

    if (!video->tex)
    {
        video->tex = TexImage2D(NULL, video->width, video->height, GL_BGRA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8);
    }

    glBindTexture(GL_TEXTURE_2D, video->tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video->width, video->height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, video->frame_buffer);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, video->tex);
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
