#include "session_manager.h"
#include "tx_app_context.h"
#include "config_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Global exit flag for threads */
static bool g_tx_app_exit = false;

/* ST20P TX thread function */
static void* st20p_tx_thread(void* arg) {
  struct st20p_tx_ctx* ctx = (struct st20p_tx_ctx*)arg;
  st20p_tx_handle handle = ctx->handle;
  struct st_frame* frame;

  printf("ST20P TX(%d): thread started\n", ctx->idx);

  while (!ctx->app->exit && !g_tx_app_exit) {
    frame = st20p_tx_get_frame(handle);
    if (!frame) {
      usleep(1000); /* 1ms */
      continue;
    }

    /* Fill frame with source data or test pattern */
    bool use_source = (ctx->source_buffer != NULL && ctx->source_size > 0);

    if (use_source) {
      /* Use loaded source file */
      size_t frame_data_size = ctx->frame_size;

      /* Check if we need to loop back */
      if (ctx->current_pos + frame_data_size > ctx->source_size) {
        ctx->current_pos = 0; /* Loop back to start */
        if (ctx->loop_playback) {
          printf("ST20P TX(%d): Looping back to start of source\n", ctx->idx);
        }
      }

      /* Copy frame data */
      if (frame->addr[0] && ctx->source_buffer) {
        size_t copy_size = (ctx->current_pos + frame_data_size <= ctx->source_size) ?
                          frame_data_size : (ctx->source_size - ctx->current_pos);

        memcpy(frame->addr[0], ctx->source_buffer + ctx->current_pos, copy_size);
        ctx->current_pos += copy_size;
      }
    } else {
      /* Generate test pattern */
      if (frame->addr[0]) {
        /* Simple test pattern - fill with frame count pattern */
        uint32_t pattern = (ctx->frames_sent % 256);

        /* For YUV formats, create a simple gradient pattern */
        if (frame->fmt == ST_FRAME_FMT_YUV422PLANAR10LE ||
            frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE) {

          /* Fill Y plane with gradient pattern */
          uint16_t* y_plane = (uint16_t*)frame->addr[0];
          size_t y_pixels = ctx->app->width * ctx->app->height;

          for (size_t i = 0; i < y_pixels; i++) {
            y_plane[i] = ((i + pattern) % 1024) << 6; /* 10-bit value shifted to 16-bit */
          }

          /* Fill U and V planes with constant values */
          if (frame->addr[1] && frame->addr[2]) {
            uint16_t* u_plane = (uint16_t*)frame->addr[1];
            uint16_t* v_plane = (uint16_t*)frame->addr[2];
            size_t uv_pixels = ctx->app->width * ctx->app->height / 2; /* 422 subsampling */

            for (size_t i = 0; i < uv_pixels; i++) {
              u_plane[i] = 512 << 6; /* Neutral chroma */
              v_plane[i] = 512 << 6; /* Neutral chroma */
            }
          }
        } else {
          /* For other formats, fill with simple pattern */
          memset(frame->addr[0], pattern, frame->data_size);
        }
      }
    }

    /* Update frame metadata */
    frame->tfmt = ST10_TIMESTAMP_FMT_MEDIA_CLK;
    frame->timestamp = ctx->frames_sent * 90000 / ctx->app->fps; /* 90kHz clock */

    /* Submit frame */
    st20p_tx_put_frame(handle, frame);
    ctx->frames_sent++;

    if (ctx->frames_sent % 100 == 0) {
      printf("ST20P TX(%d): sent %d frames\n", ctx->idx, ctx->frames_sent);
    }
  }

  printf("ST20P TX(%d): thread stopped, sent %d frames\n", ctx->idx, ctx->frames_sent);
  return NULL;
}

/* ST30P TX thread function */
static void* st30p_tx_thread(void* arg) {
  struct st30p_tx_ctx* ctx = (struct st30p_tx_ctx*)arg;
  st30p_tx_handle handle = ctx->handle;
  struct st30_frame* frame;

  printf("ST30P TX(%d): thread started\n", ctx->idx);

  while (!ctx->app->exit && !g_tx_app_exit) {
    frame = st30p_tx_get_frame(handle);
    if (!frame) {
      usleep(1000); /* 1ms */
      continue;
    }

    /* Fill frame with source data or test pattern */
    if (ctx->source_buffer && ctx->source_size > 0) {
      /* Use loaded source file */
      size_t frame_data_size = ctx->frame_size;

      if (ctx->current_pos + frame_data_size > ctx->source_size) {
        ctx->current_pos = 0; /* Loop back to start */
      }

      if (frame->addr && ctx->source_buffer) {
        size_t copy_size = (ctx->current_pos + frame_data_size <= ctx->source_size) ?
                          frame_data_size : (ctx->source_size - ctx->current_pos);

        memcpy(frame->addr, ctx->source_buffer + ctx->current_pos, copy_size);
        ctx->current_pos += copy_size;
      }
    } else {
      /* Generate test pattern - silence or tone */
      if (frame->addr) {
        memset(frame->addr, 0, frame->data_size); /* Silence */
      }
    }

    /* Submit frame */
    st30p_tx_put_frame(handle, frame);
    ctx->frames_sent++;

    if (ctx->frames_sent % 1000 == 0) {
      printf("ST30P TX(%d): sent %d frames\n", ctx->idx, ctx->frames_sent);
    }
  }

  printf("ST30P TX(%d): thread stopped, sent %d frames\n", ctx->idx, ctx->frames_sent);
  return NULL;
}

/* Map frame format to transport format */
static enum st20_fmt get_transport_format(enum st_frame_fmt frame_fmt) {
  switch (frame_fmt) {
    case ST_FRAME_FMT_YUV422PLANAR10LE:
      return ST20_FMT_YUV_422_10BIT;
    case ST_FRAME_FMT_YUV420PLANAR8:
      return ST20_FMT_YUV_420_8BIT;
    case ST_FRAME_FMT_YUV422PLANAR12LE:
      return ST20_FMT_YUV_422_12BIT;
    case ST_FRAME_FMT_YUV444PLANAR10LE:
      return ST20_FMT_YUV_444_10BIT;
    case ST_FRAME_FMT_YUV444PLANAR12LE:
      return ST20_FMT_YUV_444_12BIT;
    case ST_FRAME_FMT_GBRPLANAR10LE:
      return ST20_FMT_RGB_10BIT;
    case ST_FRAME_FMT_GBRPLANAR12LE:
      return ST20_FMT_RGB_12BIT;
    default:
      printf("Warning: Unknown frame format %d, using YUV_422_10BIT\n", frame_fmt);
      return ST20_FMT_YUV_422_10BIT;
  }
}

/* Load video source from file */
int load_video_source(struct st20p_tx_ctx* ctx, const char* filename) {
  if (!filename || strlen(filename) == 0) {
    printf("ST20P TX(%d): No source file specified, will use test pattern\n", ctx->idx);
    return 0; /* Not an error, will use test pattern */
  }

  FILE* file = fopen(filename, "rb");
  if (!file) {
    printf("ST20P TX(%d): Warning: Cannot open source file %s, will use test pattern\n",
           ctx->idx, filename);
    return 0; /* Not fatal, will use test pattern */
  }

  /* Get file size */
  fseek(file, 0, SEEK_END);
  ctx->source_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (ctx->source_size == 0) {
    printf("ST20P TX(%d): Warning: Source file %s is empty\n", ctx->idx, filename);
    fclose(file);
    return 0;
  }

  /* Allocate buffer */
  ctx->source_buffer = malloc(ctx->source_size);
  if (!ctx->source_buffer) {
    printf("ST20P TX(%d): Error: Cannot allocate %zu bytes for source buffer\n",
           ctx->idx, ctx->source_size);
    fclose(file);
    return -1;
  }

  /* Load file */
  size_t read_size = fread(ctx->source_buffer, 1, ctx->source_size, file);
  fclose(file);

  if (read_size != ctx->source_size) {
    printf("ST20P TX(%d): Warning: Only read %zu of %zu bytes from source file\n",
           ctx->idx, read_size, ctx->source_size);
    ctx->source_size = read_size;
  }

  ctx->current_pos = 0;
  ctx->loop_playback = true;

  printf("ST20P TX(%d): Loaded %zu bytes from source file %s\n",
         ctx->idx, ctx->source_size, filename);

  return 0;
}

/* Load audio source from file */
int load_audio_source(struct st30p_tx_ctx* ctx, const char* filename) {
  if (!filename || strlen(filename) == 0) {
    printf("ST30P TX(%d): No source file specified, will use silence\n", ctx->idx);
    return 0;
  }

  FILE* file = fopen(filename, "rb");
  if (!file) {
    printf("ST30P TX(%d): Warning: Cannot open source file %s, will use silence\n",
           ctx->idx, filename);
    return 0;
  }

  /* Get file size */
  fseek(file, 0, SEEK_END);
  ctx->source_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (ctx->source_size == 0) {
    printf("ST30P TX(%d): Warning: Source file %s is empty\n", ctx->idx, filename);
    fclose(file);
    return 0;
  }

  /* Allocate buffer */
  ctx->source_buffer = malloc(ctx->source_size);
  if (!ctx->source_buffer) {
    printf("ST30P TX(%d): Error: Cannot allocate %zu bytes for source buffer\n",
           ctx->idx, ctx->source_size);
    fclose(file);
    return -1;
  }

  /* Load file */
  size_t read_size = fread(ctx->source_buffer, 1, ctx->source_size, file);
  fclose(file);

  if (read_size != ctx->source_size) {
    printf("ST30P TX(%d): Warning: Only read %zu of %zu bytes from source file\n",
           ctx->idx, read_size, ctx->source_size);
    ctx->source_size = read_size;
  }

  ctx->current_pos = 0;
  ctx->loop_playback = true;

  printf("ST30P TX(%d): Loaded %zu bytes from source file %s\n",
         ctx->idx, ctx->source_size, filename);

  return 0;
}

/* Create ST20P TX session */
int create_st20p_tx_session(session_manager_t* manager, struct tx_app_context* app, int session_idx) {
  struct st20p_tx_ctx* ctx = &manager->st20p_sessions[session_idx];
  struct st20p_tx_ops ops;

  memset(ctx, 0, sizeof(*ctx));
  memset(&ops, 0, sizeof(ops));

  ctx->idx = session_idx;
  ctx->app = app;

  /* Session identification */
  snprintf(ctx->session_name, sizeof(ctx->session_name), "st20p_tx_%d", session_idx);
  ops.name = ctx->session_name;
  ops.priv = ctx;

  /* Network configuration */
  ops.port.num_port = 1;
  memcpy(ops.port.dip_addr[MTL_SESSION_PORT_P], app->dip_addr, MTL_IP_ADDR_LEN);
  strncpy(ops.port.port[MTL_SESSION_PORT_P], app->port, MTL_PORT_MAX_LEN);
  ops.port.udp_port[MTL_SESSION_PORT_P] = app->udp_port + (session_idx * 2);
  ops.port.payload_type = 112; /* Standard RTP payload type for video */

  /* Video parameters */
  ops.width = app->width;
  ops.height = app->height;
  ops.fps = app->fps;
  ops.transport_fmt = get_transport_format(app->fmt);
  ops.input_fmt = app->fmt;

  /* Device and flags */
  ops.device = ST_PLUGIN_DEVICE_AUTO;
  ops.framebuff_cnt = 3;
  ops.flags = ST20P_TX_FLAG_BLOCK_GET;

  /* Create TX handle */
  ctx->handle = st20p_tx_create(manager->mtl, &ops);
  if (!ctx->handle) {
    printf("Error: Failed to create ST20P TX session %d\n", session_idx);
    printf("       Check network configuration and MTL setup\n");
    return -1;
  }

  ctx->frame_size = st20p_tx_frame_size(ctx->handle);
  printf("ST20P TX session %d created, frame size: %zu bytes\n", session_idx, ctx->frame_size);

  /* Load video source if specified */
  if (strlen(app->tx_url) > 0) {
    if (load_video_source(ctx, app->tx_url) < 0) {
      printf("Warning: Failed to load video source, will use test pattern\n");
    }
  }

  return 0;
}

/* Create ST30P TX session */
int create_st30p_tx_session(session_manager_t* manager, struct tx_app_context* app, int session_idx) {
  struct st30p_tx_ctx* ctx = &manager->st30p_sessions[session_idx];
  struct st30p_tx_ops ops;

  memset(ctx, 0, sizeof(*ctx));
  memset(&ops, 0, sizeof(ops));

  ctx->idx = session_idx;
  ctx->app = app;

  /* Session identification */
  snprintf(ctx->session_name, sizeof(ctx->session_name), "st30p_tx_%d", session_idx);
  ops.name = ctx->session_name;
  ops.priv = ctx;

  /* Network configuration */
  ops.port.num_port = 1;
  memcpy(ops.port.dip_addr[MTL_SESSION_PORT_P], app->dip_addr, MTL_IP_ADDR_LEN);
  strncpy(ops.port.port[MTL_SESSION_PORT_P], app->port, MTL_PORT_MAX_LEN);
  ops.port.udp_port[MTL_SESSION_PORT_P] = app->udp_port + 1 + (session_idx * 2);
  ops.port.payload_type = 111; /* Standard RTP payload type for audio */

  /* Audio parameters */
  ops.channel = 2; /* Stereo */
  ops.sampling = ST30_SAMPLING_48K;
  ops.fmt = ST30_FMT_PCM24;
  ops.ptime = ST30_PTIME_1MS;

  /* Device and flags */
  ops.framebuff_cnt = 3;
  ops.flags = ST30P_TX_FLAG_BLOCK_GET;

  /* Create TX handle */
  ctx->handle = st30p_tx_create(manager->mtl, &ops);
  if (!ctx->handle) {
    printf("Error: Failed to create ST30P TX session %d\n", session_idx);
    printf("       Check network configuration and MTL setup\n");
    return -1;
  }

  ctx->frame_size = st30p_tx_frame_size(ctx->handle);
  printf("ST30P TX session %d created, frame size: %zu bytes\n", session_idx, ctx->frame_size);

  return 0;
}

/* Initialize session manager */
int session_manager_init(session_manager_t* manager, struct tx_app_context* app) {
  memset(manager, 0, sizeof(*manager));

  /* Initialize MTL */
  struct mtl_init_params mtl_params;
  memset(&mtl_params, 0, sizeof(mtl_params));

  mtl_params.flags = MTL_FLAG_BIND_NUMA | MTL_FLAG_DEV_AUTO_START_STOP;
  mtl_params.num_ports = 1;
  strncpy(mtl_params.port[MTL_PORT_P], app->port, MTL_PORT_MAX_LEN);
  memcpy(mtl_params.sip_addr[MTL_PORT_P], app->sip_addr, MTL_IP_ADDR_LEN);
  mtl_params.pmd[MTL_PORT_P] = MTL_PMD_DPDK_USER;

  manager->mtl = mtl_init(&mtl_params);
  if (!manager->mtl) {
    printf("Error: Failed to initialize MTL\n");
    return -1;
  }

  /* Store MTL handle in app context for compatibility */
  app->mtl = manager->mtl;

  /* Allocate ST20P sessions */
  if (app->st20p_sessions > 0) {
    manager->st20p_sessions = calloc(app->st20p_sessions, sizeof(struct st20p_tx_ctx));
    if (!manager->st20p_sessions) {
      printf("Error: Failed to allocate ST20P sessions\n");
      mtl_uninit(manager->mtl);
      return -1;
    }
    manager->st20p_count = app->st20p_sessions;

    /* Create ST20P sessions */
    for (int i = 0; i < app->st20p_sessions; i++) {
      if (create_st20p_tx_session(manager, app, i) < 0) {
        printf("Error: Failed to create ST20P TX session %d\n", i);
        session_manager_cleanup(manager);
        return -1;
      }
    }
  }

  /* Allocate ST30P sessions */
  if (app->st30p_sessions > 0) {
    manager->st30p_sessions = calloc(app->st30p_sessions, sizeof(struct st30p_tx_ctx));
    if (!manager->st30p_sessions) {
      printf("Error: Failed to allocate ST30P sessions\n");
      session_manager_cleanup(manager);
      return -1;
    }
    manager->st30p_count = app->st30p_sessions;

    /* Create ST30P sessions */
    for (int i = 0; i < app->st30p_sessions; i++) {
      if (create_st30p_tx_session(manager, app, i) < 0) {
        printf("Error: Failed to create ST30P TX session %d\n", i);
        session_manager_cleanup(manager);
        return -1;
      }
    }
  }

  printf("TX Session Manager initialized with %d ST20P and %d ST30P sessions\n",
         manager->st20p_count, manager->st30p_count);

  return 0;
}

/* Start session manager */
int session_manager_start(session_manager_t* manager) {
  g_tx_app_exit = false;

  /* Start ST20P threads */
  for (int i = 0; i < manager->st20p_count; i++) {
    struct st20p_tx_ctx* ctx = &manager->st20p_sessions[i];

    if (pthread_create(&ctx->thread, NULL, st20p_tx_thread, ctx) != 0) {
      printf("Error: Failed to create ST20P TX thread %d\n", i);
      return -1;
    }
  }

  /* Start ST30P threads */
  for (int i = 0; i < manager->st30p_count; i++) {
    struct st30p_tx_ctx* ctx = &manager->st30p_sessions[i];

    if (pthread_create(&ctx->thread, NULL, st30p_tx_thread, ctx) != 0) {
      printf("Error: Failed to create ST30P TX thread %d\n", i);
      return -1;
    }
  }

  manager->running = true;
  printf("TX Session Manager started\n");

  return 0;
}

/* Stop session manager */
int session_manager_stop(session_manager_t* manager) {
  g_tx_app_exit = true;
  manager->running = false;

  /* Wait for ST20P threads */
  for (int i = 0; i < manager->st20p_count; i++) {
    struct st20p_tx_ctx* ctx = &manager->st20p_sessions[i];

    if (ctx->thread) {
      /* Wake up blocked thread */
      st20p_tx_wake_block(ctx->handle);

      /* Wait for thread completion */
      pthread_join(ctx->thread, NULL);
      ctx->thread = 0;
    }
  }

  /* Wait for ST30P threads */
  for (int i = 0; i < manager->st30p_count; i++) {
    struct st30p_tx_ctx* ctx = &manager->st30p_sessions[i];

    if (ctx->thread) {
      /* Wake up blocked thread */
      st30p_tx_wake_block(ctx->handle);

      /* Wait for thread completion */
      pthread_join(ctx->thread, NULL);
      ctx->thread = 0;
    }
  }

  printf("TX Session Manager stopped\n");
  return 0;
}

/* Cleanup session manager */
void session_manager_cleanup(session_manager_t* manager) {
  /* Stop sessions if still running */
  if (manager->running) {
    session_manager_stop(manager);
  }

  /* Clean up ST20P sessions */
  if (manager->st20p_sessions) {
    for (int i = 0; i < manager->st20p_count; i++) {
      struct st20p_tx_ctx* ctx = &manager->st20p_sessions[i];

      if (ctx->handle) {
        st20p_tx_free(ctx->handle);
        ctx->handle = NULL;
      }

      if (ctx->source_buffer) {
        free(ctx->source_buffer);
        ctx->source_buffer = NULL;
      }
    }
    free(manager->st20p_sessions);
    manager->st20p_sessions = NULL;
  }

  /* Clean up ST30P sessions */
  if (manager->st30p_sessions) {
    for (int i = 0; i < manager->st30p_count; i++) {
      struct st30p_tx_ctx* ctx = &manager->st30p_sessions[i];

      if (ctx->handle) {
        st30p_tx_free(ctx->handle);
        ctx->handle = NULL;
      }

      if (ctx->source_buffer) {
        free(ctx->source_buffer);
        ctx->source_buffer = NULL;
      }
    }
    free(manager->st30p_sessions);
    manager->st30p_sessions = NULL;
  }

  /* Cleanup MTL */
  if (manager->mtl) {
    mtl_uninit(manager->mtl);
    manager->mtl = NULL;
  }

  manager->st20p_count = 0;
  manager->st30p_count = 0;
  manager->running = false;
}

/* Check if sessions are running */
bool session_manager_is_running(const session_manager_t* manager) {
  return manager && manager->running;
}