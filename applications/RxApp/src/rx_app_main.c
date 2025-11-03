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
#include "sdl_handler.h"
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

  /* Configuration file */
  char config_file[256];
};

/* Global application context for signal handling */
static struct rx_app_context* g_app_ctx = NULL;

/* Signal handler */
static void rx_app_sig_handler(int sig) {
  printf("\nReceived signal %d, stopping...\n", sig);
  if (g_app_ctx) {
    g_app_ctx->exit = true;
  }
}

/* Parse command line arguments */
static int parse_args(struct rx_app_context* app, int argc, char* argv[]) {
  /* Set default values */
  strcpy(app->port, "0000:af:00.0");
  strcpy(app->sip_addr_str, "192.168.1.101");
  strcpy(app->dip_addr_str, "239.168.1.101");
  app->udp_port = 20000;
  app->width = 1920;
  app->height = 1080;
  app->fps = ST_FPS_P59_94;
  app->fmt = ST_FRAME_FMT_YUV422PLANAR10LE;
  app->st20p_sessions = 1;
  app->st30p_sessions = 0;
  app->test_time_s = 10;
  app->enable_display = true;
  app->enable_save = false;
  app->config_file[0] = '\0';

  /* Simple argument parsing - could be enhanced with getopt */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printf("Usage: %s [options]\n", argv[0]);
      printf("Options:\n");
      printf("  --help, -h          Show this help\n");
      printf("  --port <port>       Network port (default: %s)\n", app->port);
      printf("  --sip <ip>          Source IP (default: %s)\n", app->sip_addr_str);
      printf("  --dip <ip>          Destination IP (default: %s)\n", app->dip_addr_str);
      printf("  --udp-port <port>   UDP port (default: %d)\n", app->udp_port);
      printf("  --width <width>     Video width (default: %d)\n", app->width);
      printf("  --height <height>   Video height (default: %d)\n", app->height);
      printf("  --fps <fps>         Frame rate (default: %d)\n", app->fps);
      printf("  --format <fmt>      Frame format (default: %d)\n", app->fmt);
      printf("  --st20p <count>     ST20P sessions (default: %d)\n", app->st20p_sessions);
      printf("  --st30p <count>     ST30P sessions (default: %d)\n", app->st30p_sessions);
      printf("  --time <seconds>    Test duration (default: %d)\n", app->test_time_s);
      printf("  --no-display       Disable display\n");
      printf("  --save              Enable frame saving\n");
      printf("  --config <file>     JSON config file\n");
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

  /* Main loop */
  if (app.test_time_s > 0) {
    printf("Running for %d seconds...\n", app.test_time_s);
    sleep(app.test_time_s);
    app.exit = true;
  } else {
    while (!app.exit) {
      sleep(1);
    }
  }

  printf("Stopping application...\n");

cleanup:
  /* Cleanup session manager */
  session_manager_cleanup(&session_manager);

  printf("Application stopped\n");
  return ret;
}
