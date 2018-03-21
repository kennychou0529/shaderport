float f = 1.0;
float x = -1.0 + 2.0*texel.x;
float y = -1.0 + 2.0*texel.y;
float beta = asin(y / sqrt(x*x + y*y + f*f));
float alpha = atan(x,-f) + 1.5;
if (alpha > 3.1415926)
    alpha -= 2.0*3.1415926;
float u = (alpha + 3.1415926) / (2.0*3.1415926);
float v = (beta + 3.1415926/2.0) / 3.1415926;
