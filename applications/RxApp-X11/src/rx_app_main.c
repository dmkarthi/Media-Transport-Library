/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */

#include <arpa/inet.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mtl_api.h"
#include "st_pipeline_api.h"
#include "frame_converter.h"
#include "x11_handler.h"
#include "config_reader.h"
#include "session_manager.h"

/* Function declarations */
static int parse_args(struct rx_app_context* app, int argc, char* argv[]);

/* Application context for RX sessions */
struct rx_app_context {
  /* MTL library handle */
  mtl_handle mtl;

  /* Configuration */
  char port[MTL_PORT_MAX_LEN];
  char rx_url[256];
  char sip_addr_str[INET_ADDRSTRLEN];
  uint8_t sip_addr[MTL_IP_ADDR_LEN];
  char dip_addr_str[INET_ADDRSTRLEN];
  uint8_t dip_addr[MTL_IP_ADDR_LEN];
  uint16_t udp_port;

  /* Video parameters */
  uint32_t width;
  uint32_t height;
  enum st_fps fps;
  enum st_frame_fmt fmt;

  /* Session controls */
  int st20p_sessions;
  int st30p_sessions;
  bool exit;
  int test_time_s;

  /* Display and saving controls */
  bool enable_display;
  bool enable_save;
  bool enable_test_pattern;

  /* Configuration file */
  char config_file[256];
};

/* Global application context for signal handling */
static struct rx_app_context* g_app_ctx = NULL;

/* Signal handler */
static void rx_app_sig_handler(int sig) {
  printf("\n=== SIGNAL RECEIVED ===\n");
  printf("Signal: %d (%s)\n", sig, 
         sig == SIGINT ? "SIGINT(Ctrl+C)" : 
         sig == SIGTERM ? "SIGTERM" : 
         sig == SIGQUIT ? "SIGQUIT(Ctrl+\\)" : "UNKNOWN");
  printf("Global context: %p\n", (void*)g_app_ctx);
  
  if (g_app_ctx) {
    printf("Setting exit flag to true\n");
    g_app_ctx->exit = true;
    printf("Exit flag set: %d\n", g_app_ctx->exit);
  } else {
    printf("ERROR: No global context available!\n");
  }
  
  printf("========================\n");
  fflush(stdout);
}

/* Parse command line arguments */
static int parse_args(struct rx_app_context* app, int argc, char* argv[]) {
  static struct option long_options[] = {
    {"port", required_argument, 0, 'p'},
    {"sip", required_argument, 0, 's'},
    {"dip", required_argument, 0, 'd'},
    {"udp_port", required_argument, 0, 'u'},
    {"width", required_argument, 0, 'w'},
    {"height", required_argument, 0, 'h'},
    {"fps", required_argument, 0, 'f'},
    {"fmt", required_argument, 0, 'F'},
    {"rx_url", required_argument, 0, 'r'},
    {"st20p_sessions", required_argument, 0, '2'},
    {"st30p_sessions", required_argument, 0, '3'},
    {"time", required_argument, 0, 'T'},
    {"no_display", no_argument, 0, 'D'},
    {"save", no_argument, 0, 'S'},
    {"display", no_argument, 0, 'R'},
    {"test_pattern", no_argument, 0, 'P'},
    {"config", required_argument, 0, 'C'},
    {"help", no_argument, 0, '?'},
    {0, 0, 0, 0}
  };

  /* Set default values */
  strncpy(app->port, "0000:af:01.0", sizeof(app->port));
  app->sip_addr_str[0] = '\0'; /* No default - must be provided */
  strncpy(app->dip_addr_str, "239.168.85.20", sizeof(app->dip_addr_str));
  app->udp_port = 20000;
  app->width = 1920;
  app->height = 1080;
  app->fps = ST_FPS_P25;
  app->fmt = ST_FRAME_FMT_YUV422PLANAR10LE;
  app->st20p_sessions = 1;
  app->st30p_sessions = 0;
  app->test_time_s = 0; /* Run indefinitely by default */
  app->rx_url[0] = '\0';
  app->enable_display = false; /* Disable by default, enable with --display */
  app->enable_save = false;
  app->enable_test_pattern = false; /* Test patterns disabled by default */
  app->config_file[0] = '\0';

  int c, option_index = 0;
  while ((c = getopt_long(argc, argv, "p:s:d:u:w:h:f:F:r:2:3:T:DSRPC:?", long_options, &option_index)) != -1) {
    switch (c) {
      case 'p':
        strncpy(app->port, optarg, sizeof(app->port) - 1);
        break;
      case 's':
        strncpy(app->sip_addr_str, optarg, sizeof(app->sip_addr_str) - 1);
        break;
      case 'd':
        strncpy(app->dip_addr_str, optarg, sizeof(app->dip_addr_str) - 1);
        break;
      case 'u':
        app->udp_port = atoi(optarg);
        break;
      case 'w':
        app->width = atoi(optarg);
        break;
      case 'h':
        app->height = atoi(optarg);
        break;
      case 'f':
        app->fps = atoi(optarg);
        break;
      case 'F':
        app->fmt = atoi(optarg);
        break;
      case 'r':
        strncpy(app->rx_url, optarg, sizeof(app->rx_url) - 1);
        break;
      case '2':
        app->st20p_sessions = atoi(optarg);
        break;
      case '3':
        app->st30p_sessions = atoi(optarg);
        break;
      case 'T':
        app->test_time_s = atoi(optarg);
        break;
      case 'D':
        app->enable_display = false;
        break;
      case 'S':
        app->enable_save = true;
        break;
      case 'R':
        app->enable_display = true;
        break;
      case 'P':
        app->enable_test_pattern = true;
        break;
      case 'C':
        strncpy(app->config_file, optarg, sizeof(app->config_file) - 1);
        break;
      case '?':
      default:
        printf("Usage: %s [options]\n", argv[0]);
        printf("Options:\n");
        printf("  --port <port>       Network port (default: %s)\n", app->port);
        printf("  --sip <ip>          Source IP (required)\n");
        printf("  --dip <ip>          Destination IP (default: %s)\n", app->dip_addr_str);
        printf("  --udp_port <port>   UDP port (default: %d)\n", app->udp_port);
        printf("  --width <width>     Video width (default: %d)\n", app->width);
        printf("  --height <height>   Video height (default: %d)\n", app->height);
        printf("  --fps <fps>         Frame rate (default: %d)\n", app->fps);
        printf("  --fmt <fmt>         Frame format (default: %d)\n", app->fmt);
        printf("  --st20p_sessions <count> ST20P sessions (default: %d)\n", app->st20p_sessions);
        printf("  --st30p_sessions <count> ST30P sessions (default: %d)\n", app->st30p_sessions);
        printf("  --time <seconds>    Test duration (default: %d = indefinite)\n", app->test_time_s);
        printf("  --no_display       Disable display\n");
        printf("  --display          Enable display\n");
        printf("  --save             Enable frame saving\n");
        printf("  --test_pattern     Enable test pattern display for 10 seconds\n");
        printf("  --config <file>    JSON config file\n");
        printf("  --help             Show this help\n");
        return -1;
    }
  }

  /* Parse IP addresses */
  if (inet_pton(AF_INET, app->sip_addr_str, app->sip_addr) != 1) {
    printf("Error: Invalid source IP address: %s\n", app->sip_addr_str);
    return -1;
  }

  if (inet_pton(AF_INET, app->dip_addr_str, app->dip_addr) != 1) {
    printf("Error: Invalid destination IP address: %s\n", app->dip_addr_str);
    return -1;
  }

  return 0;
}

/* Main application */
int main(int argc, char** argv) {
  struct rx_app_context app;
  session_manager_t session_manager;
  int ret = 0;

  memset(&app, 0, sizeof(app));
  g_app_ctx = &app;

  /* Parse arguments */
  if (parse_args(&app, argc, argv) < 0) {
    return -1;
  }

  /* Load configuration from JSON if specified */
  struct rx_config config;
  if (strlen(app.config_file) > 0) {
    if (load_and_validate_rx_config(app.config_file, &config) == 0) {
      apply_config_to_app(&config,
                         &app.width, &app.height, (int*)&app.fps,
                         app.dip_addr_str, sizeof(app.dip_addr_str), app.dip_addr,
                         &app.udp_port, &app.enable_display,
                         app.port, sizeof(app.port),
                         app.sip_addr_str, sizeof(app.sip_addr_str), app.sip_addr);
    }
  }

  /* Final validation - ensure we have a source IP from either command line or config */
  if (app.sip_addr_str[0] == '\0') {
    printf("Error: Source IP address must be provided via --sip or config file\n");
    return -1;
  }

  /* Install signal handler */
  signal(SIGINT, rx_app_sig_handler);
  signal(SIGTERM, rx_app_sig_handler);
  signal(SIGQUIT, rx_app_sig_handler);  // Ctrl+\ 
  g_app_ctx = &app;
  
  printf("Signal handlers installed. Press Ctrl+C to stop.\n");

  /* Initialize session manager */
  if (session_manager_init(&session_manager, &app) < 0) {
    printf("Error: Failed to initialize session manager\n");
    return -1;
  }

  /* Start all sessions */
  if (session_manager_start(&session_manager, &app) < 0) {
    printf("Error: Failed to start sessions\n");
    ret = -1;
    goto cleanup;
  }

  printf("Press Ctrl+C to stop...\n");

  /* Main loop - handles both test patterns (if enabled) and X11 events */
  if (app.enable_test_pattern && app.enable_display) {
    printf("DEBUG: Test patterns enabled - will display for 10 seconds, then switch to received frames\n");
  }
  
  /* Main frame reception loop */
  if (app.test_time_s > 0) {
    printf("Running for %d seconds...\n", app.test_time_s);
    
    time_t start_time = time(NULL);
    while (!app.exit) {
      /* Process X11 events and test patterns for all displays */
      if (app.enable_display) {
        for (int i = 0; i < session_manager.st20p_count; i++) {
          struct st20p_rx_ctx* ctx = &session_manager.st20p_sessions[i];
          if (ctx->display.initialized) {
            /* Show test patterns if enabled and in test phase */
            if (app.enable_test_pattern && should_show_test_pattern()) {
              display_test_pattern_x11(&ctx->display, i);
            }
            process_x11_events(&ctx->display);
          }
        }
      }
      
      /* Check if time limit reached */
      time_t current_time = time(NULL);
      if ((current_time - start_time) >= app.test_time_s) {
        printf("Test duration completed\n");
        app.exit = true;
      }
      
      usleep(50000); /* 50ms delay to avoid busy waiting */
    }
  } else {
    printf("Running indefinitely (press Ctrl+C to stop)...\n");
    
    /* Continue until user terminates */
    while (!app.exit) {
      /* Check for early exit before any processing */
      if (app.exit) {
        printf("Early exit detected\n");
        break;
      }
      
      /* Process X11 events and test patterns for all displays */
      if (app.enable_display) {
        for (int i = 0; i < session_manager.st20p_count; i++) {
          struct st20p_rx_ctx* ctx = &session_manager.st20p_sessions[i];
          if (ctx->display.initialized) {
            /* Show test patterns if enabled and in test phase */
            if (app.enable_test_pattern && should_show_test_pattern()) {
              display_test_pattern_x11(&ctx->display, i);
            }
            process_x11_events(&ctx->display);
          }
        }
      }
      
      usleep(10000); /* 10ms delay for better signal responsiveness */
      
      /* Debug: Check exit status more frequently and print signal debug info */
      static int debug_counter = 0;
      if (++debug_counter % 100 == 0) { /* Every 1 second */
        printf("Main loop running... (exit=%d, counter=%d, signal handler active=%s)\n", 
               app.exit, debug_counter, g_app_ctx ? "yes" : "no");
      }
      
      /* Force check exit condition more frequently */
      if (app.exit) {
        printf("Exit flag detected, breaking main loop\n");
        break;
      }
    }
  }

  printf("Stopping application...\n");

cleanup:
  /* Cleanup session manager */
  session_manager_cleanup(&session_manager);

  printf("Application stopped\n");
  return ret;
}
