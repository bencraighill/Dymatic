#pragma once


typedef unsigned char uchar;

void std_to_box(int boxes[], float sigma, int n);
void horizontal_blur_rgb(uchar* in, uchar* out, int w, int h, int c, int r);
void total_blur_rgb(uchar* in, uchar* out, int w, int h, int c, int r);
void box_blur_rgb(uchar*& in, uchar*& out, int w, int h, int c, int r);
void fast_gaussian_blur_rgb(uchar*& in, uchar*& out, int w, int h, int c, float sigma);
