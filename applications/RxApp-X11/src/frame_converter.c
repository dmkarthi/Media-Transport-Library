#include "frame_converter.h"
#include <stdio.h>

// Include MTL headers for st_frame definition
#include "st_pipeline_api.h"

/*
 * Common YUV/RGB Conversion System
 *
 * This module provides unified format conversion functions used by both:
 * - SDL renderer (real-time display)
 * - Frame converter (PPM/PGM file saving)
 *
 * Benefits:
 * - Single conversion implementation ensures consistency
 * - Eliminates code duplication between renderer and saver
 * - Centralized format support makes maintenance easier
 * - All formats handled with proper bit-depth scaling and subsampling
 */

static inline uint8_t clamp_u8(int val) {
	return (val < 0) ? 0 : (val > 255) ? 255 : val;
}

void yuv_to_rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b) {
	// ITU-R BT.601 conversion (studio range)
	int c = y - 16;
	int d = u - 128;
	int e = v - 128;
	*r = clamp_u8((298 * c + 409 * e + 128) >> 8);
	*g = clamp_u8((298 * c - 100 * d - 208 * e + 128) >> 8);
	*b = clamp_u8((298 * c + 516 * d + 128) >> 8);
}

int get_format_bit_shift(int format) {
	switch (format) {
		case ST_FRAME_FMT_YUV422PLANAR12LE:
		case ST_FRAME_FMT_YUV444PLANAR12LE:
		case ST_FRAME_FMT_GBRPLANAR12LE:
			return 4; // 12-bit formats
		case ST_FRAME_FMT_YUV422PLANAR10LE:
		case ST_FRAME_FMT_YUV444PLANAR10LE:
		case ST_FRAME_FMT_GBRPLANAR10LE:
			return 2; // 10-bit formats
		case ST_FRAME_FMT_YUV420PLANAR8:
		default:
			return 0; // 8-bit formats
	}
}

bool is_yuv422_format(int format) {
	return (format == ST_FRAME_FMT_YUV422PLANAR10LE || format == ST_FRAME_FMT_YUV422PLANAR12LE);
}

rgb_pixel_t convert_pixel_to_rgb(struct st_frame* frame, uint32_t x, uint32_t y, uint32_t width) {
	rgb_pixel_t pixel = {0, 0, 0};

	if (!frame->addr[0]) {
		return pixel; // Return black pixel if no data
	}

	if (frame->fmt == ST_FRAME_FMT_YUV422PLANAR10LE || frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE) {
		// YUV422 formats (4:2:2 subsampling)
		uint16_t* src_y = (uint16_t*)frame->addr[0];
		uint16_t* src_u = (uint16_t*)frame->addr[1];
		uint16_t* src_v = (uint16_t*)frame->addr[2];
		int bit_shift = get_format_bit_shift(frame->fmt);

		int y_idx = y * width + x;
		uint8_t Y = (src_y[y_idx] >> bit_shift) & 0xFF;
		int uv_x = x / 2; // YUV422 horizontal subsampling
		int uv_idx = y * (width/2) + uv_x;
		uint8_t U = (src_u[uv_idx] >> bit_shift) & 0xFF;
		uint8_t V = (src_v[uv_idx] >> bit_shift) & 0xFF;
		yuv_to_rgb(Y, U, V, &pixel.r, &pixel.g, &pixel.b);

	} else if (frame->fmt == ST_FRAME_FMT_YUV444PLANAR10LE || frame->fmt == ST_FRAME_FMT_YUV444PLANAR12LE) {
		// YUV444 formats (4:4:4 no subsampling)
		uint16_t* src_y = (uint16_t*)frame->addr[0];
		uint16_t* src_u = (uint16_t*)frame->addr[1];
		uint16_t* src_v = (uint16_t*)frame->addr[2];
		int bit_shift = get_format_bit_shift(frame->fmt);

		int idx = y * width + x; // Same index for all planes in 4:4:4
		uint8_t Y = (src_y[idx] >> bit_shift) & 0xFF;
		uint8_t U = (src_u[idx] >> bit_shift) & 0xFF;
		uint8_t V = (src_v[idx] >> bit_shift) & 0xFF;
		yuv_to_rgb(Y, U, V, &pixel.r, &pixel.g, &pixel.b);

	} else if (frame->fmt == ST_FRAME_FMT_GBRPLANAR10LE || frame->fmt == ST_FRAME_FMT_GBRPLANAR12LE) {
		// RGB planar formats (direct RGB)
		uint16_t* src_g = (uint16_t*)frame->addr[0]; // G plane
		uint16_t* src_b = (uint16_t*)frame->addr[1]; // B plane
		uint16_t* src_r = (uint16_t*)frame->addr[2]; // R plane
		int bit_shift = get_format_bit_shift(frame->fmt);

		int idx = y * width + x;
		pixel.r = (src_r[idx] >> bit_shift) & 0xFF;
		pixel.g = (src_g[idx] >> bit_shift) & 0xFF;
		pixel.b = (src_b[idx] >> bit_shift) & 0xFF;

	} else if (frame->fmt == ST_FRAME_FMT_YUV420PLANAR8) {
		// YUV420 8-bit format - simplified grayscale
		uint8_t* src_y = (uint8_t*)frame->addr[0];
		uint8_t Y = src_y[y * width + x];
		pixel.r = pixel.g = pixel.b = Y; // Grayscale conversion
	}

	return pixel;
}

void save_frame_as_ppm(struct st_frame* frame, int frame_num, const char* prefix, uint32_t width, uint32_t height) {
	char filename[256];
	snprintf(filename, sizeof(filename), "/tmp/%s_frame_%04d.ppm", prefix, frame_num);
	FILE* f = fopen(filename, "wb");
	if (!f) { printf("Warning: Could not create %s\n", filename); return; }
	fprintf(f, "P6\n%u %u\n255\n", width, height);

	// Use common conversion function for all formats
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			rgb_pixel_t pixel = convert_pixel_to_rgb(frame, x, y, width);
			fwrite(&pixel.r, 1, 1, f);
			fwrite(&pixel.g, 1, 1, f);
			fwrite(&pixel.b, 1, 1, f);
		}
	}
	fclose(f);
	printf("🎨 Saved COLOR frame %d as %s (%ux%u) (viewable with image viewers)\n", frame_num, filename, width, height);
}

void save_frame_as_pgm(struct st_frame* frame, int frame_num, const char* prefix, uint32_t width, uint32_t height) {
	char filename[256];
	snprintf(filename, sizeof(filename), "/tmp/%s_luma_%04d.pgm", prefix, frame_num);
	FILE* f = fopen(filename, "wb");
	if (!f) { printf("Warning: Could not create %s\n", filename); return; }
	fprintf(f, "P5\n%u %u\n255\n", width, height);

	if (frame->fmt == ST_FRAME_FMT_YUV422PLANAR10LE || frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE ||
	    frame->fmt == ST_FRAME_FMT_YUV444PLANAR10LE || frame->fmt == ST_FRAME_FMT_YUV444PLANAR12LE) {
		// All YUV formats - extract Y (luma) plane
		uint16_t* src_y = (uint16_t*)frame->addr[0];
		int bit_shift;
		if (frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE || frame->fmt == ST_FRAME_FMT_YUV444PLANAR12LE) {
			bit_shift = 4; // 12-bit formats
		} else {
			bit_shift = 2; // 10-bit formats
		}

		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {
				int y_idx = y * width + x;
				uint8_t y_val = (src_y[y_idx] >> bit_shift) & 0xFF;
				fwrite(&y_val, 1, 1, f);
			}
		}
	} else if (frame->fmt == ST_FRAME_FMT_GBRPLANAR10LE || frame->fmt == ST_FRAME_FMT_GBRPLANAR12LE) {
		// RGB formats - use G (green) plane as luma approximation
		uint16_t* src_g = (uint16_t*)frame->addr[0]; // G plane
		int bit_shift = (frame->fmt == ST_FRAME_FMT_GBRPLANAR12LE) ? 4 : 2;

		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {
				int idx = y * width + x;
				uint8_t g_val = (src_g[idx] >> bit_shift) & 0xFF;
				fwrite(&g_val, 1, 1, f);
			}
		}
	} else if (frame->fmt == ST_FRAME_FMT_YUV420PLANAR8) {
		// YUV420 8-bit format
		uint8_t* src_y = (uint8_t*)frame->addr[0];
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {
				uint8_t y_val = src_y[y * width + x];
				fwrite(&y_val, 1, 1, f);
			}
		}
	} else {
		// Unsupported format - save as black
		printf("Warning: Unsupported format %d for PGM save, saving as black\n", frame->fmt);
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {
				uint8_t black = 0;
				fwrite(&black, 1, 1, f);
			}
		}
	}

	fclose(f);
	printf("Saved grayscale frame: %s\n", filename);
}

/* Convert frame to RGB format for X11 display */
int convert_frame_to_rgb(struct st_frame* frame, uint8_t* rgb_buffer, uint32_t width, uint32_t height) {
	if (!frame || !rgb_buffer) {
		printf("ERROR: convert_frame_to_rgb - NULL pointer (frame=%p, buffer=%p)\n", frame, rgb_buffer);
		return -1;
	}
	
	if (!frame->addr[0]) {
		printf("ERROR: convert_frame_to_rgb - No frame data (addr[0]=NULL)\n");
		return -1;
	}
	
	if (frame->width != width || frame->height != height) {
		printf("ERROR: convert_frame_to_rgb - Size mismatch (frame=%dx%d, target=%dx%d)\n",
		       frame->width, frame->height, width, height);
		return -1;
	}

	switch (frame->fmt) {
		case ST_FRAME_FMT_YUV422PLANAR10LE: {
			uint16_t* src_y = (uint16_t*)frame->addr[0];
			uint16_t* src_u = (uint16_t*)frame->addr[1];
			uint16_t* src_v = (uint16_t*)frame->addr[2];

			// Use linesize from frame (in bytes), convert to stride in pixels for 16-bit data
			size_t y_stride = frame->linesize[0] / 2;  // Y linesize in 16-bit pixels
			size_t u_stride = frame->linesize[1] / 2;  // U linesize in 16-bit pixels  
			size_t v_stride = frame->linesize[2] / 2;  // V linesize in 16-bit pixels

			for (uint32_t y = 0; y < height; y++) {
				for (uint32_t x = 0; x < width; x++) {
					// YUV422: horizontal subsampling for U,V
					uint8_t y_val = (src_y[y * y_stride + x] >> 2) & 0xFF;     // 10->8 bit
					uint8_t u_val = (src_u[y * u_stride + x/2] >> 2) & 0xFF;   // U subsampled
					uint8_t v_val = (src_v[y * v_stride + x/2] >> 2) & 0xFF;   // V subsampled

					uint8_t r, g, b;
					yuv_to_rgb(y_val, u_val, v_val, &r, &g, &b);

					// Store as BGRA (X11 native format on most systems)
					uint32_t idx = (y * width + x) * 4;
					
					// Bounds check
					if (idx + 3 >= width * height * 4) {
						printf("ERROR: Buffer overflow prevented at pixel (%d,%d)\n", x, y);
						continue;
					}
					
					rgb_buffer[idx + 0] = b;     // Blue
					rgb_buffer[idx + 1] = g;     // Green  
					rgb_buffer[idx + 2] = r;     // Red
					rgb_buffer[idx + 3] = 255;   // Alpha
				}
			}
			break;
		}

		case ST_FRAME_FMT_YUV420PLANAR8: {
			uint8_t* src_y = (uint8_t*)frame->addr[0];
			uint8_t* src_u = (uint8_t*)frame->addr[1];
			uint8_t* src_v = (uint8_t*)frame->addr[2];

			for (uint32_t y = 0; y < height; y++) {
				for (uint32_t x = 0; x < width; x++) {
					uint8_t y_val = src_y[y * width + x];
					uint8_t u_val = src_u[(y/2) * (width/2) + (x/2)];
					uint8_t v_val = src_v[(y/2) * (width/2) + (x/2)];

					uint8_t r, g, b;
					yuv_to_rgb(y_val, u_val, v_val, &r, &g, &b);

					uint32_t idx = (y * width + x) * 4;
					rgb_buffer[idx + 0] = b;     // Blue
					rgb_buffer[idx + 1] = g;     // Green
					rgb_buffer[idx + 2] = r;     // Red
					rgb_buffer[idx + 3] = 255;   // Alpha
				}
			}
			break;
		}

		default:
			printf("Warning: Unsupported format %d for X11 display, filling with test pattern\n", frame->fmt);
			// Fill with test pattern
			for (uint32_t y = 0; y < height; y++) {
				for (uint32_t x = 0; x < width; x++) {
					uint32_t idx = (y * width + x) * 4;
					rgb_buffer[idx + 0] = (x + y) % 256;     // Blue gradient
					rgb_buffer[idx + 1] = x % 256;           // Green gradient
					rgb_buffer[idx + 2] = y % 256;           // Red gradient
					rgb_buffer[idx + 3] = 255;               // Alpha
				}
			}
			break;
	}

	return 0;
}
