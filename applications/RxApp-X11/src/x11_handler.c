/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */

#include "x11_handler.h"
#include "frame_converter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <time.h>
#include <math.h>

// Include MTL headers for st_frame definition
#include "st_pipeline_api.h"

/* DEBUG/TEST: Global variables for test pattern timing */
static time_t g_start_time = 0;
static const int TEST_PATTERN_DURATION_SEC = 10;

/* X11 Error handler */
static int x11_error_handler(Display *display, XErrorEvent *error) {
  char error_text[256];
  XGetErrorText(display, error->error_code, error_text, sizeof(error_text));
  
  printf("X11 Error: %s\n", error_text);
  printf("  Request: %d (0x%02x)\n", error->request_code, error->request_code);
  printf("  Minor code: %d\n", error->minor_code);
  printf("  Resource ID: 0x%lx\n", error->resourceid);
  printf("  Serial: %lu\n", error->serial);
  
  // Don't exit on error, just log it
  return 0;
}

/* X11 IO Error handler */
static int x11_io_error_handler(Display *display) {
  printf("X11 IO Error: Connection lost to X server\n");
  exit(1);
}

/* Initialize X11 display */
int init_x11_display(struct x11_display_ctx* display, uint32_t width, uint32_t height, int session_idx) {
  Display* x_display;
  Window x_window;
  GC x_gc;
  Visual* x_visual;
  XImage* x_image;
  char* image_data;
  char window_title[64];
  
  // Initialize structure
  memset(display, 0, sizeof(*display));
  
  // Check DISPLAY environment variable
  const char* display_env = getenv("DISPLAY");
  if (!display_env) {
    printf("Error: DISPLAY environment variable not set. X11 display not available.\n");
    printf("Hint: Run 'export DISPLAY=:0' or use SSH with X11 forwarding (-X flag)\n");
    return -1;
  }
  
  // Set up X11 error handlers
  XSetErrorHandler(x11_error_handler);
  XSetIOErrorHandler(x11_io_error_handler);
  
  // Open X11 display connection
  x_display = XOpenDisplay(NULL);
  if (!x_display) {
    printf("Error: Cannot open X11 display '%s'\n", display_env);
    printf("Hint: Check if X11 server is running or use SSH with X11 forwarding\n");
    return -1;
  }
  
  display->display = x_display;
  display->screen = DefaultScreen(x_display);
  display->depth = DefaultDepth(x_display, display->screen);
  display->width = width;
  display->height = height;
  
  // Get visual info
  x_visual = DefaultVisual(x_display, display->screen);
  display->visual = x_visual;
  
  // Create window
  snprintf(window_title, sizeof(window_title), "RxApp-X11 - Session %d (%dx%d)", 
           session_idx, width, height);
  
  x_window = XCreateSimpleWindow(x_display,
                                RootWindow(x_display, display->screen),
                                100 + (session_idx * 50),    // x position
                                100 + (session_idx * 50),    // y position  
                                width, height,               // dimensions
                                1,                           // border width
                                BlackPixel(x_display, display->screen), // border
                                WhitePixel(x_display, display->screen)  // background
  );
  
  if (!x_window) {
    printf("Error: Cannot create X11 window\n");
    XCloseDisplay(x_display);
    return -1;
  }
  
  display->window = (void*)(uintptr_t)x_window;
  
  // Set window properties
  XStoreName(x_display, x_window, window_title);
  XSelectInput(x_display, x_window, ExposureMask | KeyPressMask | StructureNotifyMask);
  
  // Create graphics context
  x_gc = XCreateGC(x_display, x_window, 0, NULL);
  display->gc = x_gc;
  
  // Allocate image buffer (RGB format)
  size_t image_size = width * height * 4; // RGBA
  image_data = (char*)malloc(image_size);
  if (!image_data) {
    printf("Error: Cannot allocate image buffer\n");
    XFreeGC(x_display, x_gc);
    XDestroyWindow(x_display, x_window);
    XCloseDisplay(x_display);
    return -1;
  }
  
  display->image_data = image_data;
  memset(image_data, 128, image_size); // Initialize with gray
  
  // Create XImage with proper parameters
  // Force 24-bit depth for RGB data, 32-bit alignment
  int image_depth = (display->depth >= 24) ? 24 : display->depth;
  x_image = XCreateImage(x_display, x_visual, image_depth, ZPixmap, 0,
                        image_data, width, height, 32, width * 4);
  
  if (!x_image) {
    printf("Error: Cannot create XImage (depth=%d, size=%dx%d)\n", 
           image_depth, width, height);
    free(image_data);
    XFreeGC(x_display, x_gc);
    XDestroyWindow(x_display, x_window);
    XCloseDisplay(x_display);
    return -1;
  }
  
  printf("XImage created: %dx%d, depth=%d, bpp=%d, bytes_per_line=%d\n",
         x_image->width, x_image->height, x_image->depth, 
         x_image->bits_per_pixel, x_image->bytes_per_line);
  
  display->ximage = x_image;
  
  // Map window (make it visible)
  XMapWindow(x_display, x_window);
  XFlush(x_display);

  display->initialized = true;

  /* DEBUG/TEST: Initialize test pattern timing */
  if (g_start_time == 0) {
    g_start_time = time(NULL);
    printf("DEBUG: Test pattern mode enabled for %d seconds\n", TEST_PATTERN_DURATION_SEC);
  }

  printf("X11 display initialized: %dx%d, depth=%d, session=%d\n", 
         width, height, display->depth, session_idx);

  return 0;
}

/* Cleanup X11 display */
void cleanup_x11_display(struct x11_display_ctx* display) {
  if (!display || !display->initialized) {
    return;
  }
  
  Display* x_display = (Display*)display->display;
  Window x_window = (Window)(uintptr_t)display->window;
  GC x_gc = (GC)display->gc;
  XImage* x_image = (XImage*)display->ximage;
  
  if (x_image) {
    // Note: XDestroyImage will also free the image_data
    x_image->data = NULL; // Prevent double free since we allocated separately
    XDestroyImage(x_image);
  }
  
  if (display->image_data) {
    free(display->image_data);
  }
  
  if (x_gc) {
    XFreeGC(x_display, x_gc);
  }
  
  if (x_window) {
    XDestroyWindow(x_display, x_window);
  }
  
  if (x_display) {
    XCloseDisplay(x_display);
  }
  
  memset(display, 0, sizeof(*display));
  
  printf("X11 display cleaned up\n");
}

/* Safe YUV422PLANAR10LE to RGB conversion */
static bool convert_yuv422_to_rgb_safe(struct st_frame* frame, uint32_t* rgb_buffer, 
                                      uint32_t width, uint32_t height) {
  // Validate frame parameters
  if (!frame || !rgb_buffer || !frame->addr[0] || !frame->addr[1] || !frame->addr[2]) {
    printf("Error: Invalid frame data for YUV conversion\n");
    return false;
  }
  
  if (frame->fmt != ST_FRAME_FMT_YUV422PLANAR10LE) {
    printf("Error: Unsupported format %d for YUV conversion\n", frame->fmt);
    return false;
  }
  
  // Validate frame dimensions
  if (width > 3840 || height > 2160) {  // Safety limits
    printf("Error: Frame dimensions too large: %dx%d\n", width, height);
    return false;
  }
  
  uint16_t* src_y = (uint16_t*)frame->addr[0];
  uint16_t* src_u = (uint16_t*)frame->addr[1];
  uint16_t* src_v = (uint16_t*)frame->addr[2];
  
  // YUV422 has Y at full resolution, U/V at half horizontal resolution
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      // Get Y sample (full resolution)
      uint32_t y_idx = y * width + x;
      if (y_idx >= width * height) continue;  // Safety check
      
      uint8_t Y = (src_y[y_idx] >> 2) & 0xFF;  // Convert 10-bit to 8-bit
      
      // Get U/V samples (half horizontal resolution for YUV422)
      uint32_t uv_x = x / 2;
      uint32_t uv_idx = y * (width / 2) + uv_x;
      if (uv_idx >= (width / 2) * height) continue;  // Safety check
      
      uint8_t U = (src_u[uv_idx] >> 2) & 0xFF;  // Convert 10-bit to 8-bit
      uint8_t V = (src_v[uv_idx] >> 2) & 0xFF;  // Convert 10-bit to 8-bit
      
      // YUV to RGB conversion (ITU-R BT.601)
      int c = Y - 16;
      int d = U - 128;
      int e = V - 128;
      
      int r = (298 * c + 409 * e + 128) >> 8;
      int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
      int b = (298 * c + 516 * d + 128) >> 8;
      
      // Clamp values
      r = (r < 0) ? 0 : (r > 255) ? 255 : r;
      g = (g < 0) ? 0 : (g > 255) ? 255 : g;  
      b = (b < 0) ? 0 : (b > 255) ? 255 : b;
      
      // Store as ARGB (X11 native format)
      uint32_t rgb_idx = y * width + x;
      if (rgb_idx < width * height) {  // Safety check
        rgb_buffer[rgb_idx] = 0xFF000000 | (r << 16) | (g << 8) | b;
      }
    }
  }
  
  return true;
}

/* Display frame using X11 - Enhanced with video content */
void display_frame_x11(struct x11_display_ctx* display, struct st_frame* frame) {
  if (!display || !display->initialized) {
    return;
  }
  
  Display* x_display = (Display*)display->display;
  Window x_window = (Window)(uintptr_t)display->window;
  GC x_gc = (GC)display->gc;
  XImage* x_image = (XImage*)display->ximage;
  
  static int frame_counter = 0;
  frame_counter++;
  
  uint32_t width = display->width;
  uint32_t height = display->height;
  uint32_t* rgb_buffer = (uint32_t*)display->image_data;
  
  bool video_displayed = false;
  
  // Try to display actual video frame if available
  if (frame && frame->addr[0]) {
    if (convert_yuv422_to_rgb_safe(frame, rgb_buffer, width, height)) {
      // Successfully converted video frame
      video_displayed = true;
      
      // Use safe XPutImage with error handling
      XSync(x_display, False);  // Clear any pending errors
      
      int result = XPutImage(x_display, x_window, x_gc, x_image, 
                           0, 0, 0, 0, width, height);
      
      if (result == BadMatch || result == BadDrawable || result == BadGC) {
        printf("Warning: XPutImage failed, falling back to safe display\n");
        video_displayed = false;
      }
    }
  }
  
  // Fallback to safe animated display if video conversion failed
  if (!video_displayed) {
    // Clear window and show animated rectangle (safe fallback)
    XSetForeground(x_display, x_gc, 0x808080);  // Gray background
    XFillRectangle(x_display, x_window, x_gc, 0, 0, width, height);
    
    // Animated rectangle to show activity
    XSetForeground(x_display, x_gc, (frame_counter * 0x100) % 0xFFFFFF);
    int rect_x = (frame_counter * 2) % (width - 100);
    int rect_y = (frame_counter) % (height - 100);
    XFillRectangle(x_display, x_window, x_gc, rect_x, rect_y, 100, 100);
    
    // Show status text
    XSetForeground(x_display, x_gc, 0xFFFFFF);  // White text
    char status_text[64];
    if (frame && frame->addr[0]) {
      snprintf(status_text, sizeof(status_text), "Processing frame %d...", frame_counter);
    } else {
      snprintf(status_text, sizeof(status_text), "Waiting for video data...");
    }
    XDrawString(x_display, x_window, x_gc, 10, 30, status_text, strlen(status_text));
  }
  
  // Safe flush
  XFlush(x_display);
  
  if (frame_counter % 25 == 0) {
    if (video_displayed) {
      printf("Frame %d: Video content displayed\n", frame_counter);
    } else {
      printf("Frame %d: Safe fallback display active\n", frame_counter);
    }
  }
}

/* Process X11 events (non-blocking) */
void process_x11_events(struct x11_display_ctx* display) {
  if (!display || !display->initialized) {
    return;
  }
  
  // Safe, minimal event processing - only handle essential events
  static int event_counter = 0;
  
  Display* x_display = (Display*)display->display;
  
  // Check if X11 connection is still valid
  if (!x_display || XConnectionNumber(x_display) < 0) {
    printf("Error: X11 connection lost in process_x11_events\n");
    display->initialized = false;
    return;
  }
  XEvent event;
  
  // Process events very carefully - avoid any operations that might corrupt protocol
  int events_processed = 0;
  
  while (XPending(x_display) && events_processed < 10) {  // Limit events per call
    XNextEvent(x_display, &event);
    events_processed++;
    
    switch (event.type) {
      case Expose:
        // Just clear window on expose, don't try to redraw complex images
        XClearWindow(x_display, (Window)(uintptr_t)display->window);
        if (++event_counter % 10 == 0) {
          printf("X11 Expose event %d processed\n", event_counter);
        }
        break;
        
      case KeyPress:
        // Handle key events safely
        printf("Key pressed in X11 window\n");
        break;
        
      case ConfigureNotify:
        // Log resize but don't do anything that might cause errors
        if (++event_counter % 5 == 0) {
          printf("X11 window resized to %dx%d (event %d)\n", 
                 event.xconfigure.width, event.xconfigure.height, event_counter);
        }
        break;
        
      default:
        // Ignore all other events to avoid potential issues
        break;
    }
  }
  
  // Simple flush without sync
  if (events_processed > 0) {
    XFlush(x_display);
  }
}

/* DEBUG/TEST: Check if we should still show test patterns */
bool should_show_test_pattern(void) {
  if (g_start_time == 0) {
    g_start_time = time(NULL);
    return true;
  }
  
  time_t current_time = time(NULL);
  int elapsed = (int)(current_time - g_start_time);
  
  if (elapsed >= TEST_PATTERN_DURATION_SEC) {
    static bool transition_logged = false;
    if (!transition_logged) {
      printf("DEBUG: Test pattern phase complete, switching to received frames\n");
      transition_logged = true;
    }
    return false;
  }
  
  return true;
}

/* DEBUG/TEST: Generate and display test patterns */
void display_test_pattern_x11(struct x11_display_ctx* display, int pattern_type) {
  if (!display || !display->initialized) {
    return;
  }

  Display* x_display = (Display*)display->display;
  Window x_window = (Window)(uintptr_t)display->window;
  GC x_gc = (GC)display->gc;
  XImage* x_image = (XImage*)display->ximage;
  
  uint32_t width = display->width;
  uint32_t height = display->height;
  uint32_t* pixels = (uint32_t*)display->image_data;
  
  time_t current_time = time(NULL);
  int elapsed = (int)(current_time - g_start_time);
  
  // Cycle through different patterns every 2 seconds
  int current_pattern = (elapsed / 2) % 4;
  
  switch (current_pattern) {
    case 0: {
      // Color bars pattern
      uint32_t colors[] = {
        0xFF0000FF, // Red
        0xFF00FF00, // Green  
        0xFFFF0000, // Blue
        0xFFFFFF00, // Yellow
        0xFFFF00FF, // Magenta
        0xFF00FFFF, // Cyan
        0xFFFFFFFF, // White
        0xFF000000  // Black
      };
      
      int bar_width = width / 8;
      for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
          int bar_index = x / bar_width;
          if (bar_index >= 8) bar_index = 7;
          pixels[y * width + x] = colors[bar_index];
        }
      }
      
      printf("DEBUG: Displaying color bars (pattern %d/4), elapsed: %ds\n", 
             current_pattern + 1, elapsed);
      break;
    }
    
    case 1: {
      // Moving gradient pattern
      float phase = (elapsed % 4) * M_PI / 2.0f;
      
      for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
          float fx = (float)x / width;
          float fy = (float)y / height;
          
          uint8_t r = (uint8_t)(127 + 127 * sin(fx * M_PI * 2 + phase));
          uint8_t g = (uint8_t)(127 + 127 * sin(fy * M_PI * 2 + phase));
          uint8_t b = (uint8_t)(127 + 127 * sin((fx + fy) * M_PI + phase));
          
          pixels[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
      }
      
      printf("DEBUG: Displaying moving gradient (pattern %d/4), elapsed: %ds\n", 
             current_pattern + 1, elapsed);
      break;
    }
    
    case 2: {
      // Checkerboard pattern
      int check_size = 32;
      
      for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
          int check_x = (x / check_size) % 2;
          int check_y = (y / check_size) % 2;
          
          uint32_t color = ((check_x ^ check_y) == 0) ? 0xFFFFFFFF : 0xFF000000;
          pixels[y * width + x] = color;
        }
      }
      
      printf("DEBUG: Displaying checkerboard (pattern %d/4), elapsed: %ds\n", 
             current_pattern + 1, elapsed);
      break;
    }
    
    case 3: {
      // Concentric circles
      float center_x = width / 2.0f;
      float center_y = height / 2.0f;
      float max_radius = sqrtf(center_x * center_x + center_y * center_y);
      
      for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
          float dx = x - center_x;
          float dy = y - center_y;
          float radius = sqrtf(dx * dx + dy * dy);
          
          float normalized_radius = radius / max_radius;
          uint8_t intensity = (uint8_t)(127 + 127 * sin(normalized_radius * M_PI * 8));
          
          pixels[y * width + x] = 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
        }
      }
      
      printf("DEBUG: Displaying concentric circles (pattern %d/4), elapsed: %ds\n", 
             current_pattern + 1, elapsed);
      break;
    }
  }
  
  // Display the pattern
  XPutImage(x_display, x_window, x_gc, x_image, 
           0, 0, 0, 0, width, height);
  XFlush(x_display);
}