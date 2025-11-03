/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */

/*
 * DEBUG/TEST: Standalone test program to demonstrate X11 test patterns
 * This shows the test pattern functionality without requiring MTL setup
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "x11_handler.h"

static bool g_exit = false;

static void sig_handler(int sig) {
  printf("\nReceived signal %d, stopping...\n", sig);
  g_exit = true;
}

int main(int argc, char* argv[]) {
  struct x11_display_ctx display;
  int width = 800;
  int height = 600;
  
  printf("DEBUG: Starting standalone X11 test pattern demo\n");
  printf("This will show test patterns for 10 seconds, then switch to a blank display\n");
  printf("Press Ctrl+C to exit early\n\n");
  
  /* Install signal handler */
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  
  /* Initialize X11 display */
  if (init_x11_display(&display, width, height, 0) < 0) {
    printf("Error: Failed to initialize X11 display\n");
    return -1;
  }
  
  printf("X11 display initialized successfully\n");
  printf("Window size: %dx%d\n", width, height);
  printf("Starting test pattern display...\n\n");
  
  /* Display test patterns for 10 seconds */
  while (!g_exit && should_show_test_pattern()) {
    display_test_pattern_x11(&display, 0);
    process_x11_events(&display);
    usleep(100000); /* 100ms delay */
  }
  
  if (!g_exit) {
    printf("\nTest pattern phase complete - displaying blank screen\n");
    printf("Press Ctrl+C to exit\n");
    
    /* Clear display to black and wait */
    uint32_t* pixels = (uint32_t*)display.image_data;
    for (uint32_t i = 0; i < display.width * display.height; i++) {
      pixels[i] = 0xFF000000; /* Black */
    }
    
    /* Update display */
    XPutImage((Display*)display.display, 
              (Window)(uintptr_t)display.window, 
              (GC)display.gc, 
              (XImage*)display.ximage,
              0, 0, 0, 0, display.width, display.height);
    XFlush((Display*)display.display);
    
    /* Wait for user to exit */
    while (!g_exit) {
      process_x11_events(&display);
      usleep(100000);
    }
  }
  
  /* Cleanup */
  cleanup_x11_display(&display);
  
  printf("Test pattern demo completed\n");
  return 0;
}