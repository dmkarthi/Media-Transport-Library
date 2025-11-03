#include "sdl_handler.h"
#include "frame_converter.h"
#include <SDL2/SDL.h>
#include <stdio.h>

// Include MTL headers for st_frame definition
#include "st_pipeline_api.h"

/* Initialize SDL display */
int init_sdl_display(struct sdl_display_ctx* display, uint32_t width, uint32_t height, int session_idx) {
  char window_title[64];

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Error: SDL initialization failed: %s\n", SDL_GetError());
    return -1;
  }

  snprintf(window_title, sizeof(window_title), "RxApp - Session %d (%dx%d)", session_idx, width, height);

  display->window = SDL_CreateWindow(window_title,
                                   SDL_WINDOWPOS_UNDEFINED + (session_idx * 50),
                                   SDL_WINDOWPOS_UNDEFINED + (session_idx * 50),
                                   width, height,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!display->window) {
    printf("Error: SDL window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  display->renderer = SDL_CreateRenderer((SDL_Window*)display->window, -1, SDL_RENDERER_ACCELERATED);
  if (!display->renderer) {
    printf("Error: SDL renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow((SDL_Window*)display->window);
    SDL_Quit();
    return -1;
  }

  display->texture = SDL_CreateTexture((SDL_Renderer*)display->renderer,
                                     SDL_PIXELFORMAT_RGB24,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     width, height);

  printf("SDL texture created with RGB24 format, size %dx%d\n", width, height);

  if (!display->texture) {
    printf("Error: SDL texture creation failed: %s\n", SDL_GetError());
    SDL_DestroyRenderer((SDL_Renderer*)display->renderer);
    SDL_DestroyWindow((SDL_Window*)display->window);
    SDL_Quit();
    return -1;
  }

  display->width = width;
  display->height = height;
  display->initialized = true;

  printf("SDL display initialized for session %d: %dx%d\n", session_idx, width, height);
  return 0;
}

/* Cleanup SDL display */
void cleanup_sdl_display(struct sdl_display_ctx* display) {
  if (!display->initialized) return;

  if (display->texture) {
    SDL_DestroyTexture((SDL_Texture*)display->texture);
    display->texture = NULL;
  }

  if (display->renderer) {
    SDL_DestroyRenderer((SDL_Renderer*)display->renderer);
    display->renderer = NULL;
  }

  if (display->window) {
    SDL_DestroyWindow((SDL_Window*)display->window);
    display->window = NULL;
  }

  display->initialized = false;
}

/* Display frame using SDL */
void display_frame_sdl(struct sdl_display_ctx* display, struct st_frame* frame) {
	if (!display->initialized) return;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) return;
	}
	if (!frame->addr[0]) { printf("Warning: Frame has no data\n"); return; }
	if (frame->data_size < (display->width * display->height * 2)) {
		printf("Warning: Frame appears incomplete (size=%zu, expected>=%d), skipping\n", frame->data_size, display->width * display->height * 2); return;
	}
	if (!frame->addr[1] || !frame->addr[2]) {
		printf("Warning: Frame missing U/V planes (U=%p, V=%p), skipping\n", frame->addr[1], frame->addr[2]); return;
	}
	static int frame_count = 0;
	if (frame_count < 5) {
		printf("SDL Frame %d: fmt=%d, size=%zu, %dx%d, planes=%p,%p,%p, linesize=[%zu,%zu,%zu]\n",
		       frame_count, frame->fmt, frame->data_size, display->width, display->height,
		       frame->addr[0], frame->addr[1], frame->addr[2],
		       frame->linesize[0], frame->linesize[1], frame->linesize[2]);
		frame_count++;

		// Sample first few pixels to check data validity for all supported formats
		if (frame->addr[0] && (frame->fmt == ST_FRAME_FMT_YUV422PLANAR10LE ||
		                       frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE ||
		                       frame->fmt == ST_FRAME_FMT_YUV444PLANAR10LE ||
		                       frame->fmt == ST_FRAME_FMT_YUV444PLANAR12LE ||
		                       frame->fmt == ST_FRAME_FMT_GBRPLANAR10LE ||
		                       frame->fmt == ST_FRAME_FMT_GBRPLANAR12LE)) {
			if (frame->fmt == ST_FRAME_FMT_YUV420PLANAR8) {
				uint8_t* y_data = (uint8_t*)frame->addr[0];
				printf("  Y samples (8-bit): [0]=%d [1]=%d [width]=%d [width+1]=%d\n",
				       y_data[0], y_data[1], y_data[display->width], y_data[display->width+1]);
			} else {
				uint16_t* y_data = (uint16_t*)frame->addr[0];
				printf("  First plane samples: [0]=%d [1]=%d [width]=%d [width+1]=%d\n",
				       y_data[0], y_data[1], y_data[display->width], y_data[display->width+1]);
			}
		}
	}
	uint8_t* rgb_pixels; int pitch;
	if (SDL_LockTexture((SDL_Texture*)display->texture, NULL, (void**)&rgb_pixels, &pitch) != 0) {
		printf("Warning: SDL_LockTexture failed: %s\n", SDL_GetError());
		return;
	}

	// Use common conversion function for all formats
	for (int y = 0; y < display->height; y++) {
		for (int x = 0; x < display->width; x++) {
			rgb_pixel_t pixel = convert_pixel_to_rgb(frame, x, y, display->width);
			int pixel_idx = y * pitch + x * 3;
			rgb_pixels[pixel_idx + 0] = pixel.r;
			rgb_pixels[pixel_idx + 1] = pixel.g;
			rgb_pixels[pixel_idx + 2] = pixel.b;
		}
	}
	SDL_UnlockTexture((SDL_Texture*)display->texture);
	if (SDL_RenderClear((SDL_Renderer*)display->renderer) != 0) {
		printf("Warning: SDL_RenderClear failed: %s\n", SDL_GetError());
		return;
	}
	if (SDL_RenderCopy((SDL_Renderer*)display->renderer, (SDL_Texture*)display->texture, NULL, NULL) != 0) {
		printf("Warning: SDL_RenderCopy failed: %s\n", SDL_GetError());
		return;
	}
	SDL_RenderPresent((SDL_Renderer*)display->renderer);
	static int render_count = 0;
	render_count++;
	if (render_count % 50 == 0) {
		printf("Successfully rendered RGB frame %d to SDL display\n", render_count);
	}
}