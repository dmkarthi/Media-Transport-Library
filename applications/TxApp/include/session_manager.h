#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "mtl_api.h"
#include "st_pipeline_api.h"
#include "st30_pipeline_api.h"

// Forward declarations
struct tx_app_context;

/* ST20P TX session context */
struct st20p_tx_ctx {
  int idx;
  st20p_tx_handle handle;
  pthread_t thread;
  struct tx_app_context* app;
  char session_name[32];

  /* Video source management */
  FILE* source_file;
  uint8_t* source_buffer;
  size_t source_size;
  size_t current_pos;
  bool loop_playback;

  /* Frame statistics */
  uint32_t frames_sent;
  size_t frame_size;
};

/* ST30P TX session context */
struct st30p_tx_ctx {
  int idx;
  st30p_tx_handle handle;
  pthread_t thread;
  struct tx_app_context* app;
  char session_name[32];

  /* Audio source management */
  FILE* source_file;
  uint8_t* source_buffer;
  size_t source_size;
  size_t current_pos;
  bool loop_playback;

  /* Frame statistics */
  uint32_t frames_sent;
  size_t frame_size;
};

/* TX session manager */
typedef struct {
  /* ST20P sessions */
  struct st20p_tx_ctx* st20p_sessions;
  int st20p_count;

  /* ST30P sessions */
  struct st30p_tx_ctx* st30p_sessions;
  int st30p_count;

  /* MTL handle */
  mtl_handle mtl;

  /* Control flags */
  bool running;
} session_manager_t;

/* Session manager functions */
int session_manager_init(session_manager_t* manager, struct tx_app_context* app);
int session_manager_start(session_manager_t* manager);
int session_manager_stop(session_manager_t* manager);
void session_manager_cleanup(session_manager_t* manager);
bool session_manager_is_running(const session_manager_t* manager);

/* Session creation functions */
int create_st20p_tx_session(session_manager_t* manager, struct tx_app_context* app, int session_idx);
int create_st30p_tx_session(session_manager_t* manager, struct tx_app_context* app, int session_idx);

/* Video/Audio source loading functions */
int load_video_source(struct st20p_tx_ctx* ctx, const char* filename);
int load_audio_source(struct st30p_tx_ctx* ctx, const char* filename);