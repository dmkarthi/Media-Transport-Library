#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "mtl_api.h"
#include "st_pipeline_api.h"
#include "st30_pipeline_api.h"
#include "sdl_handler.h"

// Forward declarations
struct rx_app_context;

/* ST20P RX session context */
struct st20p_rx_ctx {
  struct rx_app_context* app;
  st20p_rx_handle handle;
  pthread_t thread;
  int idx;
  size_t frame_size;
  char session_name[32];

  /* File output */
  FILE* output_file;

  /* Frame statistics */
  int frames_received;

  /* Frame comparison for change detection */
  uint8_t* prev_frame_data;
  size_t prev_frame_size;

  /* Display */
  struct sdl_display_ctx display;
};

/* ST30P RX session context */
struct st30p_rx_ctx {
  struct rx_app_context* app;
  st30p_rx_handle handle;
  pthread_t thread;
  int idx;
  char session_name[32];

  /* File output */
  FILE* output_file;

  /* Packet statistics */
  int packets_received;
};

/* Session Manager Interface */
typedef struct {
  mtl_handle mtl;
  struct st20p_rx_ctx* st20p_sessions;
  struct st30p_rx_ctx* st30p_sessions;
  int st20p_count;
  int st30p_count;
  bool running;
} session_manager_t;

/* Initialize MTL and create session manager */
int session_manager_init(session_manager_t* manager, struct rx_app_context* app);

/* Start all sessions */
int session_manager_start(session_manager_t* manager, struct rx_app_context* app);

/* Stop all sessions */
void session_manager_stop(session_manager_t* manager);

/* Cleanup session manager */
void session_manager_cleanup(session_manager_t* manager);

/* Check if sessions are running */
bool session_manager_is_running(const session_manager_t* manager);