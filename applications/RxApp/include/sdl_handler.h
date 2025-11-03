#pragma once

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
struct st_frame;

/* SDL Display context */
struct sdl_display_ctx {
  void* window;      /* SDL_Window* */
  void* renderer;    /* SDL_Renderer* */
  void* texture;     /* SDL_Texture* */
  uint32_t width;
  uint32_t height;
  bool initialized;
};

/* SDL initialization and cleanup functions */
int init_sdl_display(struct sdl_display_ctx* display, uint32_t width, uint32_t height, int session_idx);
void cleanup_sdl_display(struct sdl_display_ctx* display);

/* SDL frame display function */
void display_frame_sdl(struct sdl_display_ctx* display, struct st_frame* frame);