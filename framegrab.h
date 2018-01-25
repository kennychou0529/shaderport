#pragma once

void StopFramegrab();

void TakeScreenshot(const char *filename, bool imgui=false, bool cursor=false, bool reset=false, bool alpha=false);
// Save the framebuffer at the next main render loop and save it to a .png or .bmp.
// imgui: Enable drawing GUI
// cursor: Enable drawing a cursor or not (If drawing GUI, the cursor is arrow cursor otherwise you get a crosshair)
// reset: Reset screenshot frame counter
// alpha: Extract alpha channel from framebuffer
// filename: If no extension is provided, file will be saved as .bmp.
//           If %d is present, it will be filled with screenshot counter

void RecordVideoToImageSequence(const char *filename, int frame_cap, bool imgui=false, bool cursor=false, bool reset=false, bool alpha=false);
// Save the framebuffer starting from the next main render loop and save it to a sequence of .png or .bmp.
// frame_cap: Stop after capturing this number of frames (0 for no limit, call StopFramegrab to stop manually)
// imgui: Enable drawing GUI
// cursor: Enable drawing a cursor or not (If drawing GUI, the cursor is arrow cursor otherwise you get a crosshair)
// reset: Reset screenshot frame counter
// alpha: Extract alpha channel from framebuffer
// filename: If no extension is provided, file will be saved as .bmp.
//           If %d is present, it will be filled with screenshot counter

void RecordVideoToFfmpeg(const char *filename, float fps, int crf, int frame_cap, bool imgui=false, bool cursor=false, bool alpha=false);
// Get the framebuffer starting from the next main render loop and pipe them directly to ffmpeg
// fps: Framerate
// crf: Quality (lower is better)
// frame_cap: Stop after capturing this number of frames (0 for no limit, call StopFramegrab to stop manually)
// imgui: Enable drawing GUI
// cursor: Enable drawing a cursor or not (If drawing GUI, the cursor is arrow cursor otherwise you get a crosshair)
// reset: Reset video frame counter
// alpha: Extract alpha channel from framebuffer
// filename: If no extension is provided, file will be saved as .bmp.
//           If %d is present, it will be filled with video frame counter
//
// The function uses _popen to open a pipe, and assumes that the ffmpeg executable is present on the
// terminal that the application ran from (in the PATH variable). If you're on Windows, you will want
// to change your PATH environment variable to point to the folder holding the ffmpeg executable.
