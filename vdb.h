#pragma once

// todo: vdb_text_center_x(); vdb_text_left(); ...
void vdb_color(float r, float g, float b, float a);
void vdb_line_width(float px); // The width of lines is 1px (screen coordinates) by default
void vdb_point_size(float px); // The radius of points is 1px (screen coordinates) by default
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

// void PushView();
// void PopView();
// void vdbColor(float r, float g, float b, float a);
// void vdbText();
// void vdbPoint(float x, float y);
// void vdbLine(float x1, float y1, float x2, float y2);
// void vdbImageU08(void *data, int width, int height, int channels);
// void vdbImageF32(void *data, int width, int height, int channels);
// void vdbNewFrame();
// void vdbEndFrame();


// AddLine(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness = 1.0f);
// AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All, float thickness = 1.0f);   // a: upper-left, b: lower-right, rounding_corners_flags: 4-bits corresponding to which corner to round
// AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All);                     // a: upper-left, b: lower-right
// AddRectFilledMultiColor(const ImVec2& a, const ImVec2& b, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left);
// AddQuad(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col, float thickness = 1.0f);
// AddQuadFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col);
// AddTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col, float thickness = 1.0f);
// AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col);
// AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12, float thickness = 1.0f);
// AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12);
// AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL);
// AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL, float wrap_width = 0.0f, const ImVec4* cpu_fine_clip_rect = NULL);
// AddImage(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a = ImVec2(0,0), const ImVec2& uv_b = ImVec2(1,1), ImU32 col = 0xFFFFFFFF);
// AddImageQuad(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a = ImVec2(0,0), const ImVec2& uv_b = ImVec2(1,0), const ImVec2& uv_c = ImVec2(1,1), const ImVec2& uv_d = ImVec2(0,1), ImU32 col = 0xFFFFFFFF);
// AddImageRounded(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, ImU32 col, float rounding, int rounding_corners = ImDrawCornerFlags_All);
// AddPolyline(const ImVec2* points, const int num_points, ImU32 col, bool closed, float thickness);
// AddConvexPolyFilled(const ImVec2* points, const int num_points, ImU32 col);
// AddBezierCurve(const ImVec2& pos0, const ImVec2& cp0, const ImVec2& cp1, const ImVec2& pos1, ImU32 col, float thickness, int num_segments = 0);
