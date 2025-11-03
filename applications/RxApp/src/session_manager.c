#include "session_manager.h"
#include "sdl_handler.h"
#include "frame_converter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Forward declaration for rx_app_context (to avoid circular dependency)
struct rx_app_context {
  mtl_handle mtl;
  char port[64];
  char rx_url[256];
  char sip_addr_str[16];
  uint8_t sip_addr[4];
  char dip_addr_str[16];
  uint8_t dip_addr[4];
  uint16_t udp_port;
  uint32_t width;
  uint32_t height;
  int fps;
  int fmt;
  int st20p_sessions;
  int st30p_sessions;
  bool exit;
  int test_time_s;
  bool enable_display;
  bool enable_save;
  char config_file[256];
};

/* Frame comparison for change detection */
static bool frame_has_changed(struct st20p_rx_ctx* ctx, struct st_frame* frame) {
  /* Check if frame format is supported for comparison */
  if (!frame->addr[0] ||
      !(frame->fmt == ST_FRAME_FMT_YUV422PLANAR10LE ||
        frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE ||
        frame->fmt == ST_FRAME_FMT_YUV444PLANAR10LE ||
        frame->fmt == ST_FRAME_FMT_YUV444PLANAR12LE ||
        frame->fmt == ST_FRAME_FMT_GBRPLANAR10LE ||
        frame->fmt == ST_FRAME_FMT_GBRPLANAR12LE)) {
    return true; /* Save if we can't compare */
  }

  /* Use frame's actual dimensions */
  size_t height = ctx->app->height;
  size_t total_size = frame->data_size; /* Use actual frame data size */

  /* Allocate or reallocate previous frame buffer if needed */
  if (!ctx->prev_frame_data || ctx->prev_frame_size != total_size) {
    free(ctx->prev_frame_data);
    ctx->prev_frame_data = malloc(total_size);
    ctx->prev_frame_size = total_size;
    if (!ctx->prev_frame_data) {
      printf("Warning: Could not allocate frame comparison buffer\n");
      return true; /* Save if we can't compare */
    }
    /* First frame - always different, copy all plane data */
    size_t offset = 0;
    for (int plane = 0; plane < 3; plane++) {
      if (frame->addr[plane]) {
        size_t plane_size = frame->linesize[plane] * height;
        if (plane > 0 && (frame->fmt == ST_FRAME_FMT_YUV422PLANAR10LE ||
                          frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE)) {
          plane_size /= 2; /* YUV422 has half-width chroma */
        }
        memcpy(ctx->prev_frame_data + offset, frame->addr[plane], plane_size);
        offset += plane_size;
      }
    }
    return true;
  }

  /* Compare current frame with previous frame using simple memory comparison */
  uint8_t* current_data = malloc(total_size);
  if (!current_data) {
    return true; /* Save if we can't compare */
  }

  /* Copy current frame data for comparison */
  size_t offset = 0;
  for (int plane = 0; plane < 3; plane++) {
    if (frame->addr[plane]) {
      size_t plane_size = frame->linesize[plane] * height;
      if (plane > 0 && (frame->fmt == ST_FRAME_FMT_YUV422PLANAR10LE ||
                        frame->fmt == ST_FRAME_FMT_YUV422PLANAR12LE)) {
        plane_size /= 2; /* YUV422 has half-width chroma */
      }
      memcpy(current_data + offset, frame->addr[plane], plane_size);
      offset += plane_size;
    }
  }

  /* Compare with previous frame */
  bool changed = (memcmp(current_data, ctx->prev_frame_data, total_size) != 0);

  /* Update previous frame data for next comparison */
  memcpy(ctx->prev_frame_data, current_data, total_size);

  free(current_data);
  return changed;
}

/* ST20P RX frame processing thread */
static void* st20p_rx_thread(void* arg) {
  struct st20p_rx_ctx* ctx = (struct st20p_rx_ctx*)arg;
  struct st_frame* frame;

  printf("ST20P RX thread %d started - waiting for incoming frames\n", ctx->idx);

  while (!ctx->app->exit) {
    frame = st20p_rx_get_frame(ctx->handle);
    if (!frame) {
      usleep(1000); /* 1ms */
      continue;
    }

    ctx->frames_received++;

    /* Save to file if specified */
    if (ctx->output_file) {
      if (frame->addr[0]) {
        size_t written = fwrite(frame->addr[0], 1, frame->data_size, ctx->output_file);
        if (written != frame->data_size) {
          printf("Warning: Failed to write complete frame to file\n");
        }
      }
    }

    /* Save frame only if enabled and it has changed from previous frame */
    if (ctx->app->enable_save && frame_has_changed(ctx, frame)) {
      printf("Frame %d: Content changed - saving as PPM\n", ctx->frames_received);
      save_frame_as_ppm(frame, ctx->frames_received, "received", ctx->app->width, ctx->app->height);
    } else if (ctx->app->enable_save && ctx->frames_received <= 10) {
      printf("Frame %d: No change from previous frame - skipping save\n", ctx->frames_received);
    }

    /* Render frame only if display is enabled */
    if (ctx->app->enable_display && ctx->display.initialized) {
      display_frame_sdl(&ctx->display, frame);
    }

    /* Return frame to library */
    st20p_rx_put_frame(ctx->handle, frame);

    if (ctx->frames_received % 50 == 0) {
      if (ctx->app->enable_save) {
        printf("ST20P session %d: received %d frames (check /tmp/ for saved images)\n",
               ctx->idx, ctx->frames_received);
      } else {
        printf("ST20P session %d: received %d frames\n",
               ctx->idx, ctx->frames_received);
      }
    }
  }

  printf("ST20P RX thread %d stopped, received %d frames\n", ctx->idx, ctx->frames_received);
  return NULL;
}

/* ST30P RX packet processing thread */
static void* st30p_rx_thread(void* arg) {
  struct st30p_rx_ctx* ctx = (struct st30p_rx_ctx*)arg;
  void* packet_buf;

  printf("ST30P RX thread %d started\n", ctx->idx);

  while (!ctx->app->exit) {
    packet_buf = st30p_rx_get_frame(ctx->handle);
    if (!packet_buf) {
      usleep(1000); /* 1ms */
      continue;
    }

    ctx->packets_received++;

    /* Save to file if specified */
    if (ctx->output_file) {
      size_t packet_size = st30p_rx_frame_size(ctx->handle);
      if (packet_size > 0) {
        size_t written = fwrite(packet_buf, 1, packet_size, ctx->output_file);
        if (written != packet_size) {
          printf("Warning: Failed to write complete packet to file\n");
        }
      }
    }

    /* Return packet to library */
    st30p_rx_put_frame(ctx->handle, packet_buf);

    if (ctx->packets_received % 1000 == 0) {
      printf("ST30P session %d: received %d packets\n", ctx->idx, ctx->packets_received);
    }
  }

  printf("ST30P RX thread %d stopped, received %d packets\n", ctx->idx, ctx->packets_received);
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

/* Create ST20P session */
static int create_st20p_session(struct rx_app_context* app, struct st20p_rx_ctx* ctx, int session_idx) {
  struct st20p_rx_ops ops;

  memset(&ops, 0, sizeof(ops));

  /* Session identification */
  snprintf(ctx->session_name, sizeof(ctx->session_name), "st20p_rx_%d", session_idx);
  ops.name = ctx->session_name;
  ops.priv = ctx;

  /* Network configuration */
  ops.port.num_port = 1;
  memcpy(ops.port.ip_addr[MTL_SESSION_PORT_P], app->dip_addr, MTL_IP_ADDR_LEN);
  strncpy(ops.port.port[MTL_SESSION_PORT_P], app->port, MTL_PORT_MAX_LEN);
  ops.port.udp_port[MTL_SESSION_PORT_P] = app->udp_port + (session_idx * 2);
  ops.port.payload_type = 112; /* Standard RTP payload type for video */

  /* Video parameters */
  ops.width = app->width;
  ops.height = app->height;
  ops.fps = app->fps;
  ops.transport_fmt = get_transport_format(app->fmt);
  ops.output_fmt = app->fmt;

  /* Device and flags */
  ops.device = ST_PLUGIN_DEVICE_AUTO;
  ops.framebuff_cnt = 3;
  ops.flags = ST20P_RX_FLAG_BLOCK_GET;

  /* Create RX handle */
  ctx->handle = st20p_rx_create(app->mtl, &ops);
  if (!ctx->handle) {
    printf("Error: Failed to create ST20P RX session %d\n", session_idx);
    printf("       Possible causes: insufficient RX queues, network configuration, or DPDK setup\n");
    printf("       Check MTL logs above for detailed error information\n");
    return -1;
  }

  ctx->frame_size = st20p_rx_frame_size(ctx->handle);
  printf("ST20P RX session %d created, frame size: %zu bytes\n", session_idx, ctx->frame_size);

  /* Initialize display if enabled */
  if (app->enable_display) {
    if (init_sdl_display(&ctx->display, app->width, app->height, session_idx) < 0) {
      printf("Warning: Failed to initialize display for session %d\n", session_idx);
      /* Continue without display */
    }
  }

  return 0;
}

/* Create ST30P RX session */
static int create_st30p_session(struct rx_app_context* app, struct st30p_rx_ctx* ctx, int session_idx) {
  struct st30p_rx_ops ops;

  memset(&ops, 0, sizeof(ops));

  /* Session identification */
  snprintf(ctx->session_name, sizeof(ctx->session_name), "st30p_rx_%d", session_idx);
  ops.name = ctx->session_name;
  ops.priv = ctx;

  /* Network configuration */
  ops.port.num_port = 1;
  memcpy(ops.port.ip_addr[MTL_SESSION_PORT_P], app->dip_addr, MTL_IP_ADDR_LEN);
  strncpy(ops.port.port[MTL_SESSION_PORT_P], app->port, MTL_PORT_MAX_LEN);
  ops.port.udp_port[MTL_SESSION_PORT_P] = app->udp_port + 100 + (session_idx * 2);
  ops.port.payload_type = 111; /* Standard RTP payload type for audio */

  /* Audio parameters */
  ops.channel = 2;
  ops.sampling = ST30_SAMPLING_48K;
  ops.ptime = ST30_PTIME_1MS;
  ops.fmt = ST30_FMT_PCM24;

  /* Frame buffer and flags */
  ops.framebuff_cnt = 3;
  ops.flags = ST30P_RX_FLAG_BLOCK_GET;

  /* Create RX handle */
  ctx->handle = st30p_rx_create(app->mtl, &ops);
  if (!ctx->handle) {
    printf("Error: Failed to create ST30P RX session %d\n", session_idx);
    return -1;
  }

  printf("ST30P RX session %d created\n", session_idx);
  return 0;
}

/* Initialize MTL and create session manager */
int session_manager_init(session_manager_t* manager, struct rx_app_context* app) {
  struct mtl_init_params mtl_params;

  if (!manager || !app) {
    return -1;
  }

  memset(manager, 0, sizeof(*manager));

  /* Initialize MTL */
  memset(&mtl_params, 0, sizeof(mtl_params));
  mtl_params.num_ports = 1;
  strncpy(mtl_params.port[MTL_PORT_P], app->port, MTL_PORT_MAX_LEN);
  memcpy(mtl_params.sip_addr[MTL_PORT_P], app->sip_addr, MTL_IP_ADDR_LEN);
  mtl_params.log_level = MTL_LOG_LEVEL_INFO;
  mtl_params.flags |= MTL_FLAG_DEV_AUTO_START_STOP;

  /* Configure RX queues based on session count */
  uint16_t total_rx_sessions = app->st20p_sessions + app->st30p_sessions;
  uint16_t rx_queues_needed = total_rx_sessions + 2; /* Extra queues for system use */
  mtl_para_rx_queues_cnt_set(&mtl_params, MTL_PORT_P, rx_queues_needed);

  printf("Configuring MTL with %d RX queues for %d sessions\n",
         rx_queues_needed, total_rx_sessions);

  manager->mtl = mtl_init(&mtl_params);
  if (!manager->mtl) {
    printf("Error: MTL initialization failed\n");
    return -1;
  }

  app->mtl = manager->mtl; /* Store reference in app context */

  printf("MTL initialized successfully\n");
  printf("Listening on: %s (port %s)\n", app->sip_addr_str, app->port);
  printf("Multicast group: %s, UDP port: %d\n", app->dip_addr_str, app->udp_port);
  printf("Video format: %dx%d, FPS: %d\n", app->width, app->height,
         (app->fps == ST_FPS_P25) ? 25 : (app->fps == ST_FPS_P30) ? 30 :
         (app->fps == ST_FPS_P50) ? 50 : 60);
  printf("Sessions: ST20P=%d, ST30P=%d\n", app->st20p_sessions, app->st30p_sessions);
  printf("Display: %s\n", app->enable_display ? "Enabled" : "Disabled");

  manager->st20p_count = app->st20p_sessions;
  manager->st30p_count = app->st30p_sessions;

  return 0;
}

/* Start all sessions */
int session_manager_start(session_manager_t* manager, struct rx_app_context* app) {
  if (!manager || !app) {
    return -1;
  }

  /* Create ST20P sessions */
  if (manager->st20p_count > 0) {
    manager->st20p_sessions = calloc(manager->st20p_count, sizeof(struct st20p_rx_ctx));
    if (!manager->st20p_sessions) {
      printf("Error: Failed to allocate ST20P contexts\n");
      return -1;
    }

    for (int i = 0; i < manager->st20p_count; i++) {
      manager->st20p_sessions[i].app = app;
      manager->st20p_sessions[i].idx = i;

      /* Open output file if specified */
      if (app->rx_url[0] != '\0') {
        char filename[512];
        snprintf(filename, sizeof(filename), "%s_st20p_%d.yuv", app->rx_url, i);
        manager->st20p_sessions[i].output_file = fopen(filename, "wb");
        if (!manager->st20p_sessions[i].output_file) {
          printf("Warning: Failed to open output file %s\n", filename);
        } else {
          printf("Saving ST20P session %d to %s\n", i, filename);
        }
      }

      if (create_st20p_session(app, &manager->st20p_sessions[i], i) < 0) {
        return -1;
      }

      if (pthread_create(&manager->st20p_sessions[i].thread, NULL, st20p_rx_thread, &manager->st20p_sessions[i]) != 0) {
        printf("Error: Failed to create ST20P thread %d\n", i);
        return -1;
      }
    }
  }

  /* Create ST30P sessions */
  if (manager->st30p_count > 0) {
    manager->st30p_sessions = calloc(manager->st30p_count, sizeof(struct st30p_rx_ctx));
    if (!manager->st30p_sessions) {
      printf("Error: Failed to allocate ST30P contexts\n");
      return -1;
    }

    for (int i = 0; i < manager->st30p_count; i++) {
      manager->st30p_sessions[i].app = app;
      manager->st30p_sessions[i].idx = i;

      /* Open output file if specified */
      if (app->rx_url[0] != '\0') {
        char filename[512];
        snprintf(filename, sizeof(filename), "%s_st30p_%d.pcm", app->rx_url, i);
        manager->st30p_sessions[i].output_file = fopen(filename, "wb");
        if (!manager->st30p_sessions[i].output_file) {
          printf("Warning: Failed to open output file %s\n", filename);
        } else {
          printf("Saving ST30P session %d to %s\n", i, filename);
        }
      }

      if (create_st30p_session(app, &manager->st30p_sessions[i], i) < 0) {
        return -1;
      }

      if (pthread_create(&manager->st30p_sessions[i].thread, NULL, st30p_rx_thread, &manager->st30p_sessions[i]) != 0) {
        printf("Error: Failed to create ST30P thread %d\n", i);
        return -1;
      }
    }
  }

  manager->running = true;
  printf("All RX sessions started successfully\n");

  return 0;
}

/* Stop all sessions */
void session_manager_stop(session_manager_t* manager) {
  if (!manager || !manager->running) {
    return;
  }

  manager->running = false;

  /* Stop ST20P sessions */
  if (manager->st20p_sessions) {
    for (int i = 0; i < manager->st20p_count; i++) {
      if (manager->st20p_sessions[i].handle) {
        st20p_rx_wake_block(manager->st20p_sessions[i].handle);

        if (manager->st20p_sessions[i].thread) {
          pthread_join(manager->st20p_sessions[i].thread, NULL);
        }

        st20p_rx_free(manager->st20p_sessions[i].handle);

        if (manager->st20p_sessions[i].output_file) {
          fclose(manager->st20p_sessions[i].output_file);
        }

        /* Free frame comparison buffer */
        if (manager->st20p_sessions[i].prev_frame_data) {
          free(manager->st20p_sessions[i].prev_frame_data);
        }

        cleanup_sdl_display(&manager->st20p_sessions[i].display);
      }
    }
  }

  /* Stop ST30P sessions */
  if (manager->st30p_sessions) {
    for (int i = 0; i < manager->st30p_count; i++) {
      if (manager->st30p_sessions[i].handle) {
        st30p_rx_wake_block(manager->st30p_sessions[i].handle);

        if (manager->st30p_sessions[i].thread) {
          pthread_join(manager->st30p_sessions[i].thread, NULL);
        }

        st30p_rx_free(manager->st30p_sessions[i].handle);

        if (manager->st30p_sessions[i].output_file) {
          fclose(manager->st30p_sessions[i].output_file);
        }
      }
    }
  }
}

/* Cleanup session manager */
void session_manager_cleanup(session_manager_t* manager) {
  if (!manager) {
    return;
  }

  /* Stop sessions first */
  session_manager_stop(manager);

  /* Free session arrays */
  if (manager->st20p_sessions) {
    free(manager->st20p_sessions);
    manager->st20p_sessions = NULL;
  }

  if (manager->st30p_sessions) {
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