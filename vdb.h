#pragma once

// todo: vdb_text_center_x(); vdb_text_left(); ...
void vdb_new_frame(); // this is a signal that we are to start writing commands to the backbuffer
void vdb_render(); // this will swap the backbuffer with the front buffer, to be drawn from now
void vdb_view(float left, float right, float bottom, float top);
void vdb_color(float r, float g, float b, float a);
void vdb_line_width(float pixel_width);
void vdb_point_size(float pixel_size);
void vdb_path_clear();
void vdb_path_to(float x, float y);
void vdb_path_fill();
void vdb_path_stroke();
void vdb_text(float x, float y, const char *fmt, ...);
void vdb_point(float x, float y);
void vdb_line(float x1, float y1, float x2, float y2);
void vdb_triangle(float x1, float y1, float x2, float y2, float x3, float y3);
void vdb_triangle_filled(float x1, float y1, float x2, float y2, float x3, float y3);
void vdb_quad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
void vdb_quad_filled(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
void vdb_rect(float x, float y, float w, float h);
void vdb_rect_filled(float x, float y, float w, float h);
void vdb_circle(float x, float y, float r);
void vdb_circle_filled(float x, float y, float r);

// 3D api?
// void vdb_load_mesh();
// void vdb_load_image();
