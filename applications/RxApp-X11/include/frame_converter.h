#pragma once
#include <stdint.h>
#include <stdbool.h>

// Forward declaration for st_frame
struct st_frame;

// Common YUV to RGB conversion functions
void yuv_to_rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b);

// Format-aware conversion functions
typedef struct {
    uint8_t r, g, b;
} rgb_pixel_t;

// Convert a pixel from any supported format to RGB
rgb_pixel_t convert_pixel_to_rgb(struct st_frame* frame, uint32_t x, uint32_t y, uint32_t width);

// Get bit shift value for format
int get_format_bit_shift(int format);

// Check if format uses YUV422 subsampling
bool is_yuv422_format(int format);

// Frame saving functions
void save_frame_as_ppm(struct st_frame* frame, int frame_num, const char* prefix, uint32_t width, uint32_t height);
void save_frame_as_pgm(struct st_frame* frame, int frame_num, const char* prefix, uint32_t width, uint32_t height);

// X11 display conversion function
int convert_frame_to_rgb(struct st_frame* frame, uint8_t* rgb_buffer, uint32_t width, uint32_t height);
