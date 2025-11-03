/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */

#ifndef X11_HANDLER_H
#define X11_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
struct st_frame;

/* X11 Display context */
struct x11_display_ctx {
  void* display;     /* Display* - X11 display connection */
  void* window;      /* Window - X11 window handle */
  void* gc;          /* GC - Graphics context */
  void* visual;      /* Visual* - X11 visual info */
  void* ximage;      /* XImage* - X11 image buffer */
  int screen;        /* Screen number */
  uint32_t width;    /* Window width */
  uint32_t height;   /* Window height */
  int depth;         /* Color depth */
  char* image_data;  /* Image buffer */
  bool initialized;  /* Initialization status */
};

/* X11 initialization and cleanup functions */
int init_x11_display(struct x11_display_ctx* display, uint32_t width, uint32_t height, int session_idx);
void cleanup_x11_display(struct x11_display_ctx* display);

/* X11 frame display function */
void display_frame_x11(struct x11_display_ctx* display, struct st_frame* frame);

/* X11 event processing */
void process_x11_events(struct x11_display_ctx* display);

/* DEBUG/TEST: Test pattern generation for initial 10 seconds */
void display_test_pattern_x11(struct x11_display_ctx* display, int pattern_type);
bool should_show_test_pattern(void);

#endif /* X11_HANDLER_H */