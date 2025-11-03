# Media Transport Library: Pipeline API Reference Guide

**Date:** February 11, 2026  
**Repository:** OpenVisualCloud/Media-Transport-Library  
**Branch:** initial-tx_rx_app  
**Header Files:** `st_pipeline_api.h`, `st30_pipeline_api.h`

---

## Table of Contents

1. [Overview](#overview)
2. [ST20P API - Uncompressed Video Pipeline](#st20p-api---uncompressed-video-pipeline)
3. [ST22P API - Compressed Video Pipeline](#st22p-api---compressed-video-pipeline)
4. [ST30P API - Audio Pipeline](#st30p-api---audio-pipeline)
5. [Common Data Structures](#common-data-structures)
6. [Usage Examples](#usage-examples)
7. [Error Handling](#error-handling)
8. [Best Practices](#best-practices)

---

## Overview

### What are Pipeline APIs?

The Pipeline APIs provide a **high-level abstraction** for ST2110 media transport, automatically handling:
- **Format conversions** (YUV/RGB color space conversions)
- **Codec operations** (JPEGXS, H.264, H.265 encoding/decoding)
- **Frame buffer management** (circular buffer with state machines)
- **Plugin management** (CPU/GPU/FPGA codec loading)

### API Categories

| API Family | Standard | Purpose | Work with |
|------------|----------|---------|-----------|
| **ST20P** | ST2110-20 | Uncompressed video transport | Raw pixels (YUV/RGB) |
| **ST22P** | ST2110-22 | Compressed video transport | Raw pixels → Auto-encode |
| **ST30P** | ST2110-30 | PCM audio transport | PCM audio samples |

### Key Design Principles

1. **Blocking/Non-blocking Operations:** All `get_frame()` APIs support optional blocking mode via flags
2. **Circular Buffer Pattern:** get_frame() → Fill/Process → put_frame()
3. **Plugin Architecture:** Transparent hardware acceleration (CPU/GPU/FPGA)
4. **Zero-Copy Options:** External frame buffers for minimal overhead
5. **Thread Safety:** All APIs are thread-safe

---

## ST20P API - Uncompressed Video Pipeline

### Overview

ST20P APIs handle **ST2110-20 uncompressed video** transport with automatic color space conversion between application pixel formats and transport formats.

**Typical Use Case:**
- Application works with YUV422 planar (easy to process)
- Transport requires RFC4175 packed format (network efficient)
- ST20P automatically converts between formats via plugins

### API Functions

#### Session Management

```c
/**
 * Create one tx st2110-20 pipeline session
 * 
 * @param mt       Media transport device handle
 * @param ops      Session configuration parameters
 * @return         Session handle or NULL on error
 */
st20p_tx_handle st20p_tx_create(mtl_handle mt, struct st20p_tx_ops* ops);

/**
 * Free the tx st2110-20 pipeline session
 * 
 * @param handle   Session handle
 * @return         0 on success, <0 on error
 */
int st20p_tx_free(st20p_tx_handle handle);

/**
 * Create one rx st2110-20 pipeline session
 * 
 * @param mt       Media transport device handle
 * @param ops      Session configuration parameters
 * @return         Session handle or NULL on error
 */
st20p_rx_handle st20p_rx_create(mtl_handle mt, struct st20p_rx_ops* ops);

/**
 * Free the rx st2110-20 pipeline session
 * 
 * @param handle   Session handle
 * @return         0 on success, <0 on error
 */
int st20p_rx_free(st20p_rx_handle handle);
```

#### Frame Operations - TX

```c
/**
 * Get one tx frame from the pipeline session
 * Call st20p_tx_put_frame() or st20p_tx_put_ext_frame() to return the frame
 * 
 * @param handle   Session handle
 * @return         Frame pointer or NULL if no frame available
 */
struct st_frame* st20p_tx_get_frame(st20p_tx_handle handle);

/**
 * Put back the frame to the tx pipeline session
 * 
 * @param handle   Session handle
 * @param frame    Frame pointer from st20p_tx_get_frame()
 * @return         0 on success, <0 on error
 */
int st20p_tx_put_frame(st20p_tx_handle handle, struct st_frame* frame);

/**
 * Put back the frame with external framebuffer (zero-copy mode)
 * Only valid if ST20P_TX_FLAG_EXT_FRAME is set
 * 
 * @param handle      Session handle
 * @param frame       Frame pointer from st20p_tx_get_frame()
 * @param ext_frame   External framebuffer descriptor
 * @return            0 on success, <0 on error
 */
int st20p_tx_put_ext_frame(st20p_tx_handle handle, struct st_frame* frame,
                           struct st_ext_frame* ext_frame);
```

#### Frame Operations - RX

```c
/**
 * Get one rx frame from the pipeline session
 * Call st20p_rx_put_frame() to return the frame
 * 
 * @param handle   Session handle
 * @return         Frame pointer or NULL if no frame available
 */
struct st_frame* st20p_rx_get_frame(st20p_rx_handle handle);

/**
 * Put back the frame to the rx pipeline session
 * 
 * @param handle   Session handle
 * @param frame    Frame pointer from st20p_rx_get_frame()
 * @return         0 on success, <0 on error
 */
int st20p_rx_put_frame(st20p_rx_handle handle, struct st_frame* frame);
```

#### Utility Functions

```c
/**
 * Get framebuffer pointer by index
 * 
 * @param handle   Session handle
 * @param idx      Buffer index [0, framebuff_cnt-1]
 * @return         Framebuffer address or NULL on error
 */
void* st20p_tx_get_fb_addr(st20p_tx_handle handle, uint16_t idx);
void* st20p_rx_get_fb_addr(st20p_rx_handle handle, uint16_t idx);

/**
 * Get framebuffer size in bytes
 * 
 * @param handle   Session handle
 * @return         Frame size in bytes
 */
size_t st20p_tx_frame_size(st20p_tx_handle handle);
size_t st20p_rx_frame_size(st20p_rx_handle handle);

/**
 * Online update destination info (IP/MAC/port)
 * 
 * @param handle   Session handle
 * @param dst      New destination info
 * @return         0 on success, <0 on error
 */
int st20p_tx_update_destination(st20p_tx_handle handle, struct st_tx_dest_info* dst);

/**
 * Online update source info (IP filtering)
 * 
 * @param handle   Session handle
 * @param src      New source info
 * @return         0 on success, <0 on error
 */
int st20p_rx_update_source(st20p_rx_handle handle, struct st_rx_source_info* src);
```

#### Blocking Mode Control

```c
/**
 * Wake up blocked get_frame() call
 * Only effective if ST20P_TX_FLAG_BLOCK_GET is set
 * 
 * @param handle   Session handle
 * @return         0 on success, <0 on error
 */
int st20p_tx_wake_block(st20p_tx_handle handle);
int st20p_rx_wake_block(st20p_rx_handle handle);

/**
 * Set timeout for blocking get_frame() call
 * Default: 1 second
 * 
 * @param handle         Session handle
 * @param timedwait_ns   Timeout in nanoseconds
 * @return               0 on success, <0 on error
 */
int st20p_tx_set_block_timeout(st20p_tx_handle handle, uint64_t timedwait_ns);
int st20p_rx_set_block_timeout(st20p_rx_handle handle, uint64_t timedwait_ns);
```

#### Statistics and Debug

```c
/**
 * Get scheduler index for the session
 * 
 * @param handle   Session handle
 * @return         Scheduler index (>=0) or error code (<0)
 */
int st20p_tx_get_sch_idx(st20p_tx_handle handle);

/**
 * Get port statistics
 * 
 * @param handle   Session handle
 * @param port     Port index (MTL_SESSION_PORT_P or MTL_SESSION_PORT_R)
 * @param stats    Statistics structure to fill
 * @return         0 on success, <0 on error
 */
int st20p_tx_get_port_stats(st20p_tx_handle handle, enum mtl_session_port port,
                            struct st20_tx_port_status* stats);

/**
 * Reset port statistics
 * 
 * @param handle   Session handle
 * @param port     Port index
 * @return         0 on success, <0 on error
 */
int st20p_tx_reset_port_stats(st20p_tx_handle handle, enum mtl_session_port port);

/**
 * Dump packets to pcapng file (RX only)
 * 
 * @param handle             Session handle
 * @param max_dump_packets   Maximum packets to dump
 * @param sync               Synchronous (true) or asynchronous (false)
 * @param meta               Dump metadata (optional, can be NULL)
 * @return                   0 on success, <0 on error
 */
int st20p_rx_pcapng_dump(st20p_rx_handle handle, uint32_t max_dump_packets, 
                         bool sync, struct st_pcap_dump_meta* meta);

/**
 * Get queue metadata (for DATA_PATH_ONLY mode)
 * 
 * @param handle   Session handle
 * @param meta     Queue metadata to fill
 * @return         0 on success, <0 on error
 */
int st20p_rx_get_queue_meta(st20p_rx_handle handle, struct st_queue_meta* meta);
```

### Configuration Structures

#### ST20P TX Operations Structure

```c
struct st20p_tx_ops {
  /* ===== MANDATORY FIELDS ===== */
  struct st_tx_port port;              // Port configuration (IP, MAC, port number)
  
  uint32_t width;                      // Frame width in pixels
  uint32_t height;                     // Frame height in pixels
  enum st_fps fps;                     // Frame rate (ST_FPS_P59_94, ST_FPS_P60, etc.)
  bool interlaced;                     // true = interlaced, false = progressive
  
  enum st_frame_fmt input_fmt;         // Application pixel format
                                       // ST_FRAME_FMT_YUV422PLANAR10LE
                                       // ST_FRAME_FMT_RGB8, ST_FRAME_FMT_V210, etc.
  
  enum st21_pacing transport_pacing;   // ST21_PACING_NARROW, ST21_PACING_NARROW_LINEAR, etc.
  enum st20_packing transport_packing; // ST20_PACKING_BPM or ST20_PACKING_GPM
  enum st20_fmt transport_fmt;         // Network format (ST20_FMT_YUV_422_10BIT, etc.)
  
  enum st_plugin_device device;        // ST_PLUGIN_DEVICE_AUTO, _CPU, _GPU, _FPGA
  uint16_t framebuff_cnt;              // Frame buffer count [2, ST20_FB_MAX_COUNT]
  
  /* ===== OPTIONAL FIELDS ===== */
  const char* name;                    // Session name for logging
  void* priv;                          // Private data passed to callbacks
  uint32_t flags;                      // See ST20P_TX_FLAG_* below
  
  // Callbacks (run from lcore tasklet - use non-blocking calls only)
  int (*notify_frame_available)(void* priv);
  int (*notify_frame_done)(void* priv, struct st_frame* frame);
  int (*notify_event)(void* priv, enum st_event event, void* args);
  
  size_t transport_linesize;           // Manual linesize (non-convert mode only)
  struct st_tx_rtcp_ops rtcp;          // RTCP configuration
  uint8_t tx_dst_mac[MTL_SESSION_PORT_MAX][MTL_MAC_ADDR_LEN];
  
  uint16_t start_vrx;                  // VRX buffer start value (auto if 0)
  uint16_t pad_interval;               // Padding interval for RL pacing (auto if 0)
  int32_t rtp_timestamp_delta_us;      // RTP timestamp offset (microseconds)
  uint32_t tx_hang_detect_ms;          // TX hang detection timeout (default 1000ms)
  
  int socket_id;                       // NUMA socket (if FORCE_NUMA flag set)
};
```

#### ST20P TX Flags

```c
enum st20p_tx_flag {
  ST20P_TX_FLAG_USER_P_MAC         = MTL_BIT32(0),  // User-supplied primary MAC
  ST20P_TX_FLAG_USER_R_MAC         = MTL_BIT32(1),  // User-supplied redundant MAC
  ST20P_TX_FLAG_EXT_FRAME          = MTL_BIT32(2),  // External frame mode (zero-copy)
  ST20P_TX_FLAG_DISABLE_MIGRATE    = MTL_BIT32(4),  // Disable TX migrate
  ST20P_TX_FLAG_DEDICATE_QUEUE     = MTL_BIT32(5),  // Dedicated TX queue
  ST20P_TX_FLAG_ENABLE_RTCP        = MTL_BIT32(7),  // Enable RTCP
  ST20P_TX_FLAG_RTP_TIMESTAMP_FIRST_PKT = MTL_BIT32(8),  // RTP TS at first packet
  ST20P_TX_FLAG_RTP_TIMESTAMP_EPOCH     = MTL_BIT32(9),  // RTP TS at epoch
  ST20P_TX_FLAG_DISABLE_BULK       = MTL_BIT32(10), // Disable bulk ring operations
  ST20P_TX_FLAG_FORCE_NUMA         = MTL_BIT32(11), // Force NUMA socket
  ST20P_TX_FLAG_BLOCK_GET          = MTL_BIT32(15), // Enable blocking get_frame()
};
```

#### ST20P RX Operations Structure

```c
struct st20p_rx_ops {
  /* ===== MANDATORY FIELDS ===== */
  struct st_rx_port port;              // Port configuration (multicast IP, port, etc.)
  
  uint32_t width;                      // Frame width in pixels
  uint32_t height;                     // Frame height in pixels
  enum st_fps fps;                     // Frame rate
  bool interlaced;                     // Interlaced flag
  
  enum st20_fmt transport_fmt;         // Network format
  enum st_frame_fmt output_fmt;        // Application output format
  enum st_plugin_device device;        // Converter device selection
  uint16_t framebuff_cnt;              // Frame buffer count
  
  /* ===== OPTIONAL FIELDS ===== */
  const char* name;
  void* priv;
  uint32_t flags;                      // See ST20P_RX_FLAG_* below
  uint16_t rx_burst_size;              // RX burst size (0 = auto)
  
  int (*notify_frame_available)(void* priv);
  int (*notify_event)(void* priv, enum st_event event, void* args);
  
  size_t transport_linesize;
  struct st_ext_frame* ext_frames;     // External frames array (if EXT_FRAME flag)
  struct st_rx_rtcp_ops rtcp;
  
  // External frame callback (for dynamic external frame mode)
  int (*query_ext_frame)(void* priv, struct st_ext_frame* ext_frame,
                         struct st20_rx_frame_meta* meta);
  
  // Auto-detection callback (if AUTO_DETECT flag)
  int (*notify_detected)(void* priv, const struct st20_detect_meta* meta,
                         struct st20_detect_reply* reply);
  
  int socket_id;
  void* gpu_context;                   // GPU context for GPU direct mode
};
```

#### ST20P RX Flags

```c
enum st20p_rx_flag {
  ST20P_RX_FLAG_DATA_PATH_ONLY     = MTL_BIT32(0),  // App manages flow/multicast
  ST20P_RX_FLAG_ENABLE_VSYNC       = MTL_BIT32(1),  // Pass VSYNC events
  ST20P_RX_FLAG_EXT_FRAME          = MTL_BIT32(2),  // External frame mode
  ST20P_RX_FLAG_PKT_CONVERT        = MTL_BIT32(3),  // Per-packet conversion
  ST20P_RX_FLAG_ENABLE_RTCP        = MTL_BIT32(4),  // Enable RTCP
  ST20P_RX_FLAG_SIMULATE_PKT_LOSS  = MTL_BIT32(5),  // Test: simulate packet loss
  ST20P_RX_FLAG_FORCE_NUMA         = MTL_BIT32(6),  // Force NUMA socket
  ST20P_RX_FLAG_BLOCK_GET          = MTL_BIT32(15), // Enable blocking get_frame()
  ST20P_RX_FLAG_RECEIVE_INCOMPLETE_FRAME = MTL_BIT32(16), // Pass incomplete frames
  ST20P_RX_FLAG_DMA_OFFLOAD        = MTL_BIT32(17), // Use DMA for memory copy
  ST20P_RX_FLAG_AUTO_DETECT        = MTL_BIT32(18), // Auto-detect video format
  ST20P_RX_FLAG_HDR_SPLIT          = MTL_BIT32(19), // Header split offload
  ST20P_RX_FLAG_DISABLE_MIGRATE    = MTL_BIT32(20), // Disable RX migrate
  ST20P_RX_FLAG_TIMING_PARSER_STAT = MTL_BIT32(21), // Enable timing stats
  ST20P_RX_FLAG_TIMING_PARSER_META = MTL_BIT32(22), // Return timing in frame meta
  ST20P_RX_FLAG_USE_MULTI_THREADS  = MTL_BIT32(23), // Multi-threaded RX processing
  ST20P_RX_FLAG_USE_GPU_DIRECT_FRAMEBUFFERS = MTL_BIT32(24), // GPU VRAM buffers
};
```

---

## ST22P API - Compressed Video Pipeline

### Overview

ST22P APIs handle **ST2110-22 compressed video** transport with automatic encoding/decoding via plugins.

**Key Features:**
- Application works with **raw pixels** (YUV/RGB)
- MTL automatically **encodes** (TX) or **decodes** (RX) using plugins
- Supports: **JPEGXS**, **H.264**, **H.265** codecs
- **CPU/GPU/FPGA** plugin support

**Typical Use Case:**
- TX: Application provides YUV422 pixels → MTL encodes to JPEGXS → Network transport
- RX: Network receives JPEGXS → MTL decodes to RGB8 → Application displays

### API Functions

#### Session Management

```c
/**
 * Create one tx st2110-22 pipeline session
 * 
 * @param mt       Media transport device handle
 * @param ops      Session configuration parameters
 * @return         Session handle or NULL on error
 */
st22p_tx_handle st22p_tx_create(mtl_handle mt, struct st22p_tx_ops* ops);

/**
 * Free the tx st2110-22 pipeline session
 * 
 * @param handle   Session handle
 * @return         0 on success, <0 on error
 */
int st22p_tx_free(st22p_tx_handle handle);

/**
 * Create one rx st2110-22 pipeline session
 * 
 * @param mt       Media transport device handle
 * @param ops      Session configuration parameters
 * @return         Session handle or NULL on error
 */
st22p_rx_handle st22p_rx_create(mtl_handle mt, struct st22p_rx_ops* ops);

/**
 * Free the rx st2110-22 pipeline session
 * 
 * @param handle   Session handle
 * @return         0 on success, <0 on error
 */
int st22p_rx_free(st22p_rx_handle handle);
```

#### Frame Operations - TX

```c
/**
 * Get one tx frame from the pipeline session
 * Call st22p_tx_put_frame() or st22p_tx_put_ext_frame() to return the frame
 * 
 * @param handle   Session handle
 * @return         Frame pointer or NULL if no frame available
 */
struct st_frame* st22p_tx_get_frame(st22p_tx_handle handle);

/**
 * Put back the frame to the tx pipeline session
 * Frame will be automatically encoded and transmitted
 * 
 * @param handle   Session handle
 * @param frame    Frame pointer from st22p_tx_get_frame()
 * @return         0 on success, <0 on error
 */
int st22p_tx_put_frame(st22p_tx_handle handle, struct st_frame* frame);

/**
 * Put back the frame with external framebuffer
 * Only valid if ST22P_TX_FLAG_EXT_FRAME is set
 * 
 * @param handle      Session handle
 * @param frame       Frame pointer from st22p_tx_get_frame()
 * @param ext_frame   External framebuffer descriptor
 * @return            0 on success, <0 on error
 */
int st22p_tx_put_ext_frame(st22p_tx_handle handle, struct st_frame* frame,
                           struct st_ext_frame* ext_frame);
```

#### Frame Operations - RX

```c
/**
 * Get one rx frame from the pipeline session
 * Frame is already decoded to pixel format
 * Call st22p_rx_put_frame() to return the frame
 * 
 * @param handle   Session handle
 * @return         Frame pointer or NULL if no frame available
 */
struct st_frame* st22p_rx_get_frame(st22p_rx_handle handle);

/**
 * Put back the frame to the rx pipeline session
 * 
 * @param handle   Session handle
 * @param frame    Frame pointer from st22p_rx_get_frame()
 * @return         0 on success, <0 on error
 */
int st22p_rx_put_frame(st22p_rx_handle handle, struct st_frame* frame);
```

#### Utility Functions

```c
/**
 * Get framebuffer pointer by index
 */
void* st22p_tx_get_fb_addr(st22p_tx_handle handle, uint16_t idx);
void* st22p_rx_get_fb_addr(st22p_rx_handle handle, uint16_t idx);

/**
 * Get framebuffer size in bytes
 */
size_t st22p_tx_frame_size(st22p_tx_handle handle);
size_t st22p_rx_frame_size(st22p_rx_handle handle);

/**
 * Online update destination/source info
 */
int st22p_tx_update_destination(st22p_tx_handle handle, struct st_tx_dest_info* dst);
int st22p_rx_update_source(st22p_rx_handle handle, struct st_rx_source_info* src);

/**
 * Blocking mode control
 */
int st22p_tx_wake_block(st22p_tx_handle handle);
int st22p_rx_wake_block(st22p_rx_handle handle);
int st22p_tx_set_block_timeout(st22p_tx_handle handle, uint64_t timedwait_ns);
int st22p_rx_set_block_timeout(st22p_rx_handle handle, uint64_t timedwait_ns);

/**
 * Debug: Dump packets to pcapng file
 */
int st22p_rx_pcapng_dump(st22p_rx_handle handle, uint32_t max_dump_packets,
                         bool sync, struct st_pcap_dump_meta* meta);

/**
 * Get queue metadata (for DATA_PATH_ONLY mode)
 */
int st22p_rx_get_queue_meta(st22p_rx_handle handle, struct st_queue_meta* meta);
```

### Configuration Structures

#### ST22P TX Operations Structure

```c
struct st22p_tx_ops {
  /* ===== MANDATORY FIELDS ===== */
  struct st_tx_port port;              // Port configuration
  
  uint32_t width;                      // Frame width
  uint32_t height;                     // Frame height
  enum st_fps fps;                     // Frame rate
  bool interlaced;                     // Interlaced flag
  
  enum st_frame_fmt input_fmt;         // Application input format (raw pixels)
  enum st22_pack_type pack_type;       // ST22_PACK_CODESTREAM
  enum st22_codec codec;               // ST22_CODEC_JPEGXS, H264, H265
  enum st_plugin_device device;        // Encoder device (AUTO/CPU/GPU/FPGA)
  enum st22_quality_mode quality;      // ST22_QUALITY_MODE_SPEED or QUALITY
  
  size_t codestream_size;              // Expected encoded size (compression ratio)
  uint16_t framebuff_cnt;              // Frame buffer count [2, ST22_FB_MAX_COUNT]
  
  /* ===== OPTIONAL FIELDS ===== */
  const char* name;
  void* priv;
  uint32_t flags;                      // See ST22P_TX_FLAG_* below
  uint32_t codec_thread_cnt;           // Encoder thread count (0 = auto)
  
  int (*notify_frame_available)(void* priv);
  int (*notify_frame_done)(void* priv, struct st_frame* frame);
  int (*notify_event)(void* priv, enum st_event event, void* args);
  
  struct st_tx_rtcp_ops rtcp;
  uint8_t tx_dst_mac[MTL_SESSION_PORT_MAX][MTL_MAC_ADDR_LEN];
  int socket_id;
};
```

#### ST22P TX Flags

```c
enum st22p_tx_flag {
  ST22P_TX_FLAG_USER_P_MAC         = MTL_BIT32(0),  // User-supplied primary MAC
  ST22P_TX_FLAG_USER_R_MAC         = MTL_BIT32(1),  // User-supplied redundant MAC
  ST22P_TX_FLAG_EXT_FRAME          = MTL_BIT32(2),  // External frame mode
  ST22P_TX_FLAG_DEDICATE_QUEUE     = MTL_BIT32(3),  // Dedicated TX queue
  ST22P_TX_FLAG_ENABLE_VSYNC       = MTL_BIT32(4),  // Pass VSYNC events
  ST22P_TX_FLAG_ENABLE_RTCP        = MTL_BIT32(5),  // Enable RTCP
  ST22P_TX_FLAG_FORCE_NUMA         = MTL_BIT32(6),  // Force NUMA socket
  ST22P_TX_FLAG_BLOCK_GET          = MTL_BIT32(15), // Enable blocking get_frame()
};
```

#### ST22P RX Operations Structure

```c
struct st22p_rx_ops {
  /* ===== MANDATORY FIELDS ===== */
  struct st_rx_port port;              // Port configuration
  
  uint32_t width;                      // Frame width
  uint32_t height;                     // Frame height
  enum st_fps fps;                     // Frame rate
  bool interlaced;                     // Interlaced flag
  
  enum st_frame_fmt output_fmt;        // Application output format (raw pixels)
  enum st22_pack_type pack_type;       // ST22_PACK_CODESTREAM
  enum st22_codec codec;               // Codec type
  enum st_plugin_device device;        // Decoder device (AUTO/CPU/GPU/FPGA)
  uint16_t framebuff_cnt;              // Frame buffer count
  
  /* ===== OPTIONAL FIELDS ===== */
  const char* name;
  void* priv;
  uint32_t flags;                      // See ST22P_RX_FLAG_* below
  uint32_t codec_thread_cnt;           // Decoder thread count (0 = auto)
  size_t max_codestream_size;          // Max encoded size (uses output size if 0)
  
  int (*notify_frame_available)(void* priv);
  int (*notify_event)(void* priv, enum st_event event, void* args);
  
  int (*query_ext_frame)(void* priv, struct st_ext_frame* ext_frame,
                         struct st22_rx_frame_meta* meta);
  
  struct st_rx_rtcp_ops rtcp;
  int socket_id;
};
```

#### ST22P RX Flags

```c
enum st22p_rx_flag {
  ST22P_RX_FLAG_DATA_PATH_ONLY     = MTL_BIT32(0),  // App manages flow/multicast
  ST22P_RX_FLAG_ENABLE_VSYNC       = MTL_BIT32(1),  // Pass VSYNC events
  ST22P_RX_FLAG_ENABLE_RTCP        = MTL_BIT32(2),  // Enable RTCP
  ST22P_RX_FLAG_SIMULATE_PKT_LOSS  = MTL_BIT32(3),  // Test: simulate packet loss
  ST22P_RX_FLAG_EXT_FRAME          = MTL_BIT32(4),  // External frame mode
  ST22P_RX_FLAG_FORCE_NUMA         = MTL_BIT32(5),  // Force NUMA socket
  ST22P_RX_FLAG_BLOCK_GET          = MTL_BIT32(15), // Enable blocking get_frame()
  ST22P_RX_FLAG_RECEIVE_INCOMPLETE_FRAME = MTL_BIT32(16), // Pass incomplete frames
};
```

---

## ST30P API - Audio Pipeline

### Overview

ST30P APIs handle **ST2110-30 PCM audio** transport with automatic pacing and frame management.

**Key Features:**
- Application works with **PCM audio samples**
- MTL handles **RTP packetization** and **timing** automatically
- Supports multiple **channels** and **sampling rates**
- Configurable **packet time** (PTIME)

### API Functions

#### Session Management

```c
/**
 * Create one tx st2110-30 pipeline session
 * 
 * @param mt       Media transport device handle
 * @param ops      Session configuration parameters
 * @return         Session handle or NULL on error
 */
st30p_tx_handle st30p_tx_create(mtl_handle mt, struct st30p_tx_ops* ops);

/**
 * Free the tx st2110-30 pipeline session
 * 
 * @param handle   Session handle
 * @return         0 on success, <0 on error
 */
int st30p_tx_free(st30p_tx_handle handle);

/**
 * Create one rx st2110-30 pipeline session
 * 
 * @param mt       Media transport device handle
 * @param ops      Session configuration parameters
 * @return         Session handle or NULL on error
 */
st30p_rx_handle st30p_rx_create(mtl_handle mt, struct st30p_rx_ops* ops);

/**
 * Free the rx st2110-30 pipeline session
 * 
 * @param handle   Session handle
 * @return         0 on success, <0 on error
 */
int st30p_rx_free(st30p_rx_handle handle);
```

#### Frame Operations

```c
/**
 * Get one tx frame from the pipeline session
 * Call st30p_tx_put_frame() to return the frame
 * 
 * @param handle   Session handle
 * @return         Frame pointer or NULL if no frame available
 */
struct st30_frame* st30p_tx_get_frame(st30p_tx_handle handle);

/**
 * Put back the frame to the tx pipeline session
 * 
 * @param handle   Session handle
 * @param frame    Frame pointer from st30p_tx_get_frame()
 * @return         0 on success, <0 on error
 */
int st30p_tx_put_frame(st30p_tx_handle handle, struct st30_frame* frame);

/**
 * Get one rx frame from the pipeline session
 * Call st30p_rx_put_frame() to return the frame
 * 
 * @param handle   Session handle
 * @return         Frame pointer or NULL if no frame available
 */
struct st30_frame* st30p_rx_get_frame(st30p_rx_handle handle);

/**
 * Put back the frame to the rx pipeline session
 * 
 * @param handle   Session handle
 * @param frame    Frame pointer from st30p_rx_get_frame()
 * @return         0 on success, <0 on error
 */
int st30p_rx_put_frame(st30p_rx_handle handle, struct st30_frame* frame);
```

#### Utility Functions

```c
/**
 * Get framebuffer pointer by index
 */
void* st30p_tx_get_fb_addr(st30p_tx_handle handle, uint16_t idx);

/**
 * Get framebuffer size in bytes
 */
size_t st30p_tx_frame_size(st30p_tx_handle handle);
size_t st30p_rx_frame_size(st30p_rx_handle handle);

/**
 * Online update destination info
 */
int st30p_tx_update_destination(st30p_tx_handle handle, struct st_tx_dest_info* dst);

/**
 * Blocking mode control
 */
int st30p_tx_wake_block(st30p_tx_handle handle);
int st30p_rx_wake_block(st30p_rx_handle handle);
int st30p_tx_set_block_timeout(st30p_tx_handle handle, uint64_t timedwait_ns);
int st30p_rx_set_block_timeout(st30p_rx_handle handle, uint64_t timedwait_ns);
```

### Configuration Structures

#### ST30P TX Operations Structure

```c
struct st30p_tx_ops {
  /* ===== MANDATORY FIELDS ===== */
  struct st_tx_port port;              // Port configuration
  
  enum st30_fmt fmt;                   // Audio format
                                       // ST30_FMT_PCM24, ST30_FMT_PCM16, ST30_FMT_PCM8
  uint16_t channel;                    // Number of audio channels (1, 2, 4, 8, ...)
  enum st30_sampling sampling;         // Sampling rate
                                       // ST30_SAMPLING_48K, ST30_SAMPLING_96K, etc.
  enum st30_ptime ptime;               // Packet time
                                       // ST30_PTIME_1MS, ST30_PTIME_125US, etc.
  
  uint16_t framebuff_cnt;              // Frame buffer count
  uint32_t framebuff_size;             // Frame size (must be multiple of packet size)
  
  /* ===== OPTIONAL FIELDS ===== */
  const char* name;
  void* priv;
  uint32_t flags;                      // See ST30P_TX_FLAG_* below
  enum st30_tx_pacing_way pacing_way;  // Pacing method (auto if not set)
  
  int (*notify_frame_available)(void* priv);
  int (*notify_frame_done)(void* priv, struct st30_frame* frame);
  
  uint16_t fifo_size;                  // FIFO ring size (0 = auto)
  uint8_t tx_dst_mac[MTL_SESSION_PORT_MAX][MTL_MAC_ADDR_LEN];
  
  uint32_t rl_accuracy_ns;             // RL pacing accuracy (optional)
  int32_t rl_offset_ns;                // RL pacing offset (optional)
  int socket_id;
};
```

#### ST30P TX Flags

```c
enum st30p_tx_flag {
  ST30P_TX_FLAG_USER_P_MAC         = MTL_BIT32(0),  // User-supplied primary MAC
  ST30P_TX_FLAG_USER_R_MAC         = MTL_BIT32(1),  // User-supplied redundant MAC
  ST30P_TX_FLAG_DEDICATE_QUEUE     = MTL_BIT32(7),  // Dedicated TX queue
  ST30P_TX_FLAG_FORCE_NUMA         = MTL_BIT32(8),  // Force NUMA socket
  ST30P_TX_FLAG_BLOCK_GET          = MTL_BIT32(15), // Enable blocking get_frame()
};
```

#### ST30P RX Operations Structure

```c
struct st30p_rx_ops {
  /* ===== MANDATORY FIELDS ===== */
  struct st_rx_port port;              // Port configuration
  
  enum st30_fmt fmt;                   // Audio format
  uint16_t channel;                    // Number of audio channels
  enum st30_sampling sampling;         // Sampling rate
  enum st30_ptime ptime;               // Packet time
  
  uint16_t framebuff_cnt;              // Frame buffer count
  uint32_t framebuff_size;             // Frame size (multiple of packet size)
  
  /* ===== OPTIONAL FIELDS ===== */
  const char* name;
  void* priv;
  uint32_t flags;                      // See ST30P_RX_FLAG_* below
  
  int (*notify_frame_available)(void* priv);
  
  int socket_id;
};
```

#### ST30P RX Flags

```c
enum st30p_rx_flag {
  ST30P_RX_FLAG_DATA_PATH_ONLY     = MTL_BIT32(0),  // App manages flow/multicast
  ST30P_RX_FLAG_FORCE_NUMA         = MTL_BIT32(2),  // Force NUMA socket
  ST30P_RX_FLAG_BLOCK_GET          = MTL_BIT32(15), // Enable blocking get_frame()
};
```

---

## Common Data Structures

### Frame Structures

#### st_frame (Video Frames - ST20P/ST22P)

```c
struct st_frame {
  void* addr[3];                       // Frame buffer addresses (planar: Y, U, V)
  uint8_t planes;                      // Number of planes (1=packed, 3=planar)
  size_t buffer_size;                  // Total buffer size in bytes
  size_t data_size;                    // Valid data size (may be < buffer_size)
  
  uint32_t width;                      // Frame width in pixels
  uint32_t height;                     // Frame height in pixels
  enum st_frame_fmt fmt;               // Pixel format
  bool interlaced;                     // Interlaced flag
  bool second_field;                   // Second field (for interlaced)
  
  enum st10_timestamp_fmt tfmt;        // Timestamp format (RTP/PTP/etc.)
  uint64_t timestamp;                  // Frame timestamp
  uint64_t epoch;                      // Epoch number
  uint32_t rtp_timestamp;              // RTP timestamp from header
  
  // RX specific
  enum st_frame_status status;         // Frame status (complete/incomplete)
  uint32_t pkts_total;                 // Total expected packets
  uint32_t pkts_recv[MTL_SESSION_PORT_MAX]; // Packets received per port
  
  // TX specific
  uint32_t flags;                      // Frame flags
  uint16_t user_meta_buffer_size;      // User metadata size
  void* user_meta;                     // User metadata pointer
  
  void* priv;                          // Private (DO NOT TOUCH)
};
```

#### st30_frame (Audio Frames - ST30P)

```c
struct st30_frame {
  void* addr;                          // Frame buffer address
  enum st30_fmt fmt;                   // Audio format
  uint16_t channel;                    // Number of channels
  enum st30_sampling sampling;         // Sampling rate
  enum st30_ptime ptime;               // Packet time
  
  size_t buffer_size;                  // Total buffer size
  size_t data_size;                    // Valid data size
  
  enum st10_timestamp_fmt tfmt;        // Timestamp format
  uint64_t timestamp;                  // Frame timestamp
  uint64_t epoch;                      // Epoch number
  uint32_t rtp_timestamp;              // RTP timestamp
  
  // RX specific
  uint32_t pkts_total;                 // Total expected packets
  uint32_t pkts_recv[MTL_SESSION_PORT_MAX]; // Packets received per port
  
  void* priv;                          // Private (DO NOT TOUCH)
};
```

### Port Configuration Structures

#### st_tx_port (Transmit Port)

```c
struct st_tx_port {
  uint8_t dip_addr[MTL_SESSION_PORT_MAX][MTL_IP_ADDR_LEN]; // Destination IPs
  uint8_t num_port;                    // Number of ports (1 or 2 for redundancy)
  char port[MTL_SESSION_PORT_MAX][MTL_PORT_MAX_LEN]; // PCIe BDF (e.g., "0000:af:00.0")
  uint16_t udp_port[MTL_SESSION_PORT_MAX];           // UDP destination ports
  uint8_t payload_type;                // RTP payload type (96-127)
  uint32_t ssrc;                       // RTP SSRC (0 = auto-generate)
};
```

#### st_rx_port (Receive Port)

```c
struct st_rx_port {
  uint8_t ip_addr[MTL_SESSION_PORT_MAX][MTL_IP_ADDR_LEN]; // Multicast/sender IPs
  uint8_t num_port;                    // Number of ports
  char port[MTL_SESSION_PORT_MAX][MTL_PORT_MAX_LEN];      // PCIe BDF
  uint16_t udp_port[MTL_SESSION_PORT_MAX];                // UDP ports
  uint8_t payload_type;                // RTP payload type (0 = no check)
  uint32_t ssrc;                       // RTP SSRC (0 = no check)
  uint8_t mcast_sip_addr[MTL_SESSION_PORT_MAX][MTL_IP_ADDR_LEN]; // Source filter
};
```

### External Frame Structure

```c
struct st_ext_frame {
  void* addr[3];                       // External buffer addresses
  uint16_t linesize[3];                // Line size per plane (bytes per line)
  size_t size;                         // Total buffer size
  
  void* opaque;                        // User opaque data
  void (*free)(void* opaque);          // Callback to free external buffer
};
```

### Enumerations

#### Frame Formats (enum st_frame_fmt)

```c
/* YUV Formats */
ST_FRAME_FMT_YUV422PLANAR10LE        // YUV 4:2:2 planar 10-bit LE
ST_FRAME_FMT_YUV422PLANAR8           // YUV 4:2:2 planar 8-bit
ST_FRAME_FMT_YUV420PLANAR8           // YUV 4:2:0 planar 8-bit (I420)
ST_FRAME_FMT_V210                    // YUV 4:2:2 packed 10-bit
ST_FRAME_FMT_Y210                    // YUV 4:2:2 packed 16-bit
ST_FRAME_FMT_UYVY                    // YUV 4:2:2 packed 8-bit (UYVY)
ST_FRAME_FMT_YUV422RFC4175PG2BE10    // RFC4175 YUV422 10-bit BE
ST_FRAME_FMT_YUV444PLANAR10LE        // YUV 4:4:4 planar 10-bit LE

/* RGB Formats */
ST_FRAME_FMT_RGB8                    // RGB 8-bit (24bpp)
ST_FRAME_FMT_ARGB                    // ARGB 8-bit (32bpp)
ST_FRAME_FMT_BGRA                    // BGRA 8-bit (32bpp)
ST_FRAME_FMT_GBRPLANAR10LE           // GBR planar 10-bit LE
ST_FRAME_FMT_RGBRFC4175PG4BE10       // RFC4175 RGB 10-bit BE

/* Codestream Formats (ST22P) */
ST_FRAME_FMT_JPEGXS_CODESTREAM       // JPEGXS encoded
ST_FRAME_FMT_H264_CODESTREAM         // H.264 encoded
ST_FRAME_FMT_H265_CODESTREAM         // H.265/HEVC encoded
```

#### Frame Rates (enum st_fps)

```c
ST_FPS_P23_98                        // 23.98 fps
ST_FPS_P24                           // 24 fps
ST_FPS_P25                           // 25 fps
ST_FPS_P29_97                        // 29.97 fps
ST_FPS_P30                           // 30 fps
ST_FPS_P50                           // 50 fps
ST_FPS_P59_94                        // 59.94 fps
ST_FPS_P60                           // 60 fps
ST_FPS_P100                          // 100 fps
ST_FPS_P119_88                       // 119.88 fps
ST_FPS_P120                          // 120 fps
```

#### ST22 Codecs (enum st22_codec)

```c
ST22_CODEC_JPEGXS                    // JPEG XS (default for ST22)
ST22_CODEC_H264                      // H.264/AVC
ST22_CODEC_H265                      // H.265/HEVC
```

#### Plugin Devices (enum st_plugin_device)

```c
ST_PLUGIN_DEVICE_AUTO                // Automatic selection
ST_PLUGIN_DEVICE_CPU                 // CPU-based plugin
ST_PLUGIN_DEVICE_GPU                 // GPU-accelerated plugin
ST_PLUGIN_DEVICE_FPGA                // FPGA-based plugin
```

#### Quality Modes (enum st22_quality_mode)

```c
ST22_QUALITY_MODE_QUALITY            // Optimize for quality
ST22_QUALITY_MODE_SPEED              // Optimize for speed
```

#### Audio Formats (enum st30_fmt)

```c
ST30_FMT_PCM8                        // 8-bit PCM
ST30_FMT_PCM16                       // 16-bit PCM
ST30_FMT_PCM24                       // 24-bit PCM
```

#### Audio Sampling Rates (enum st30_sampling)

```c
ST30_SAMPLING_48K                    // 48 kHz
ST30_SAMPLING_96K                    // 96 kHz
ST30_SAMPLING_192K                   // 192 kHz
```

#### Audio Packet Time (enum st30_ptime)

```c
ST30_PTIME_1MS                       // 1 millisecond
ST30_PTIME_125US                     // 125 microseconds
ST30_PTIME_250US                     // 250 microseconds
ST30_PTIME_333US                     // 333 microseconds
ST30_PTIME_1000US                    // 1000 microseconds
```

---

## Usage Examples

### Example 1: ST20P TX - Basic Uncompressed Video Transmit

```c
#include <mtl/st_pipeline_api.h>

void* tx_thread(void* arg) {
    mtl_handle mtl = (mtl_handle)arg;
    
    // Configure TX session
    struct st20p_tx_ops ops = {0};
    
    // Mandatory: Port configuration
    ops.port.num_port = 1;
    snprintf(ops.port.port[0], MTL_PORT_MAX_LEN, "0000:af:00.0");
    inet_pton(AF_INET, "239.168.85.20", ops.port.dip_addr[0]);
    ops.port.udp_port[0] = 20000;
    ops.port.payload_type = 112;
    
    // Mandatory: Video parameters
    ops.width = 1920;
    ops.height = 1080;
    ops.fps = ST_FPS_P59_94;
    ops.interlaced = false;
    
    // Mandatory: Format configuration
    ops.input_fmt = ST_FRAME_FMT_YUV422PLANAR10LE;  // App provides planar
    ops.transport_fmt = ST20_FMT_YUV_422_10BIT;      // Network uses RFC4175
    ops.transport_packing = ST20_PACKING_BPM;
    ops.transport_pacing = ST21_PACING_NARROW;
    ops.device = ST_PLUGIN_DEVICE_AUTO;              // Auto-select converter
    
    // Mandatory: Buffer count
    ops.framebuff_cnt = 3;
    
    // Optional: Session name and flags
    ops.name = "st20p_tx_demo";
    ops.flags = ST20P_TX_FLAG_BLOCK_GET;             // Enable blocking mode
    
    // Create session
    st20p_tx_handle handle = st20p_tx_create(mtl, &ops);
    if (!handle) {
        printf("Failed to create ST20P TX session\n");
        return NULL;
    }
    
    printf("ST20P TX session created, frame size: %zu bytes\n",
           st20p_tx_frame_size(handle));
    
    // Transmission loop
    while (running) {
        // Get frame buffer (blocks if ST20P_TX_FLAG_BLOCK_GET is set)
        struct st_frame* frame = st20p_tx_get_frame(handle);
        if (!frame) {
            usleep(1000);
            continue;
        }
        
        // Fill with video data (planar YUV422)
        // frame->addr[0] = Y plane
        // frame->addr[1] = U plane
        // frame->addr[2] = V plane
        generate_yuv_frame(frame->addr, frame->width, frame->height);
        
        // Set frame metadata
        frame->data_size = frame->buffer_size;
        
        // Return frame - auto converts and transmits
        st20p_tx_put_frame(handle, frame);
        
        frame_count++;
    }
    
    // Cleanup
    st20p_tx_free(handle);
    return NULL;
}
```

### Example 2: ST20P RX - Basic Uncompressed Video Receive

```c
#include <mtl/st_pipeline_api.h>

void* rx_thread(void* arg) {
    mtl_handle mtl = (mtl_handle)arg;
    
    // Configure RX session
    struct st20p_rx_ops ops = {0};
    
    // Mandatory: Port configuration
    ops.port.num_port = 1;
    snprintf(ops.port.port[0], MTL_PORT_MAX_LEN, "0000:af:00.0");
    inet_pton(AF_INET, "239.168.85.20", ops.port.ip_addr[0]);
    ops.port.udp_port[0] = 20000;
    ops.port.payload_type = 112;
    
    // Mandatory: Video parameters
    ops.width = 1920;
    ops.height = 1080;
    ops.fps = ST_FPS_P59_94;
    ops.interlaced = false;
    
    // Mandatory: Format configuration
    ops.transport_fmt = ST20_FMT_YUV_422_10BIT;      // Network format
    ops.output_fmt = ST_FRAME_FMT_RGB8;              // App wants RGB8
    ops.device = ST_PLUGIN_DEVICE_AUTO;              // Auto-select converter
    ops.framebuff_cnt = 4;
    
    // Optional
    ops.name = "st20p_rx_demo";
    ops.flags = ST20P_RX_FLAG_BLOCK_GET;
    
    // Create session
    st20p_rx_handle handle = st20p_rx_create(mtl, &ops);
    if (!handle) {
        printf("Failed to create ST20P RX session\n");
        return NULL;
    }
    
    printf("ST20P RX session created\n");
    
    // Reception loop
    while (running) {
        // Get received frame (blocks if ST20P_RX_FLAG_BLOCK_GET is set)
        struct st_frame* frame = st20p_rx_get_frame(handle);
        if (!frame) {
            usleep(1000);
            continue;
        }
        
        // Check frame status
        if (frame->status == ST_FRAME_STATUS_COMPLETE) {
            // Process frame (already converted to RGB8)
            display_rgb_frame(frame->addr[0], frame->width, frame->height);
        } else {
            printf("Warning: Incomplete frame, pkts %u/%u\n",
                   frame->pkts_recv[0], frame->pkts_total);
        }
        
        // Return frame to pipeline
        st20p_rx_put_frame(handle, frame);
    }
    
    // Cleanup
    st20p_rx_free(handle);
    return NULL;
}
```

### Example 3: ST22P TX - Compressed Video with JPEGXS

```c
#include <mtl/st_pipeline_api.h>

int st22p_tx_example(mtl_handle mtl) {
    // Configure ST22P TX session
    struct st22p_tx_ops ops = {0};
    
    // Port configuration
    ops.port.num_port = 1;
    snprintf(ops.port.port[0], MTL_PORT_MAX_LEN, "0000:af:00.0");
    inet_pton(AF_INET, "239.168.85.22", ops.port.dip_addr[0]);
    ops.port.udp_port[0] = 30000;
    ops.port.payload_type = 112;
    
    // Video parameters
    ops.width = 1920;
    ops.height = 1080;
    ops.fps = ST_FPS_P60;
    ops.interlaced = false;
    
    // Codec configuration
    ops.input_fmt = ST_FRAME_FMT_YUV422PLANAR10LE;   // App provides raw pixels
    ops.pack_type = ST22_PACK_CODESTREAM;
    ops.codec = ST22_CODEC_JPEGXS;                   // Use JPEGXS encoder
    ops.device = ST_PLUGIN_DEVICE_AUTO;              // Auto-select encoder plugin
    ops.quality = ST22_QUALITY_MODE_SPEED;
    
    // Compression ratio: ~10:1 for JPEGXS
    size_t raw_size = ops.width * ops.height * 2;    // YUV422 10-bit ~= 2 bytes/pixel
    ops.codestream_size = raw_size / 10;
    
    ops.framebuff_cnt = 4;
    ops.codec_thread_cnt = 0;                        // Auto thread count
    
    // Optional
    ops.name = "st22p_jpegxs_tx";
    ops.flags = ST22P_TX_FLAG_BLOCK_GET;
    
    // Create session
    st22p_tx_handle handle = st22p_tx_create(mtl, &ops);
    if (!handle) {
        printf("Failed to create ST22P TX session\n");
        return -1;
    }
    
    printf("ST22P TX created with JPEGXS encoder\n");
    
    // Transmission loop
    while (running) {
        struct st_frame* frame = st22p_tx_get_frame(handle);
        if (!frame) {
            usleep(1000);
            continue;
        }
        
        // Fill with raw YUV data
        // MTL will automatically encode to JPEGXS before transmission
        fill_yuv_frame(frame->addr, frame->width, frame->height);
        frame->data_size = frame->buffer_size;
        
        // Put frame - triggers encoding and transmission
        st22p_tx_put_frame(handle, frame);
    }
    
    st22p_tx_free(handle);
    return 0;
}
```

### Example 4: ST22P RX - Compressed Video Receive and Decode

```c
#include <mtl/st_pipeline_api.h>

int st22p_rx_example(mtl_handle mtl) {
    // Configure ST22P RX session
    struct st22p_rx_ops ops = {0};
    
    // Port configuration
    ops.port.num_port = 1;
    snprintf(ops.port.port[0], MTL_PORT_MAX_LEN, "0000:af:00.0");
    inet_pton(AF_INET, "239.168.85.22", ops.port.ip_addr[0]);
    ops.port.udp_port[0] = 30000;
    ops.port.payload_type = 112;
    
    // Video parameters
    ops.width = 1920;
    ops.height = 1080;
    ops.fps = ST_FPS_P60;
    ops.interlaced = false;
    
    // Codec configuration
    ops.output_fmt = ST_FRAME_FMT_RGB8;              // App wants RGB8 output
    ops.pack_type = ST22_PACK_CODESTREAM;
    ops.codec = ST22_CODEC_JPEGXS;                   // Incoming stream is JPEGXS
    ops.device = ST_PLUGIN_DEVICE_AUTO;              // Auto-select decoder plugin
    
    ops.framebuff_cnt = 4;
    ops.codec_thread_cnt = 0;                        // Auto thread count
    
    // Optional
    ops.name = "st22p_jpegxs_rx";
    ops.flags = ST22P_RX_FLAG_BLOCK_GET;
    
    // Create session
    st22p_rx_handle handle = st22p_rx_create(mtl, &ops);
    if (!handle) {
        printf("Failed to create ST22P RX session\n");
        return -1;
    }
    
    printf("ST22P RX created with JPEGXS decoder\n");
    
    // Reception loop
    while (running) {
        // Get decoded frame (already converted to RGB8)
        struct st_frame* frame = st22p_rx_get_frame(handle);
        if (!frame) {
            usleep(1000);
            continue;
        }
        
        if (frame->status == ST_FRAME_STATUS_COMPLETE) {
            // Frame is already decoded - use directly
            display_frame(frame->addr[0], frame->width, frame->height);
        }
        
        // Return frame
        st22p_rx_put_frame(handle, frame);
    }
    
    st22p_rx_free(handle);
    return 0;
}
```

### Example 5: ST30P Audio TX/RX

```c
#include <mtl/st30_pipeline_api.h>

// Audio TX
int st30p_tx_example(mtl_handle mtl) {
    struct st30p_tx_ops ops = {0};
    
    // Port configuration
    ops.port.num_port = 1;
    snprintf(ops.port.port[0], MTL_PORT_MAX_LEN, "0000:af:00.0");
    inet_pton(AF_INET, "239.168.85.30", ops.port.dip_addr[0]);
    ops.port.udp_port[0] = 40000;
    ops.port.payload_type = 111;
    
    // Audio parameters
    ops.fmt = ST30_FMT_PCM24;                        // 24-bit PCM
    ops.channel = 2;                                 // Stereo
    ops.sampling = ST30_SAMPLING_48K;                // 48 kHz
    ops.ptime = ST30_PTIME_1MS;                      // 1ms packet time
    
    // Calculate frame size: samples_per_ms * channels * bytes_per_sample
    uint32_t samples_per_ms = 48;                    // 48 samples @ 48kHz
    uint32_t bytes_per_sample = 3;                   // 24-bit = 3 bytes
    ops.framebuff_size = samples_per_ms * ops.channel * bytes_per_sample;
    ops.framebuff_cnt = 3;
    
    ops.name = "st30p_tx_demo";
    ops.flags = ST30P_TX_FLAG_BLOCK_GET;
    
    st30p_tx_handle handle = st30p_tx_create(mtl, &ops);
    if (!handle) return -1;
    
    while (running) {
        struct st30_frame* frame = st30p_tx_get_frame(handle);
        if (!frame) continue;
        
        // Fill with PCM audio data
        generate_audio_samples(frame->addr, frame->buffer_size);
        frame->data_size = frame->buffer_size;
        
        st30p_tx_put_frame(handle, frame);
    }
    
    st30p_tx_free(handle);
    return 0;
}

// Audio RX
int st30p_rx_example(mtl_handle mtl) {
    struct st30p_rx_ops ops = {0};
    
    // Port configuration
    ops.port.num_port = 1;
    snprintf(ops.port.port[0], MTL_PORT_MAX_LEN, "0000:af:00.0");
    inet_pton(AF_INET, "239.168.85.30", ops.port.ip_addr[0]);
    ops.port.udp_port[0] = 40000;
    ops.port.payload_type = 111;
    
    // Audio parameters (must match TX)
    ops.fmt = ST30_FMT_PCM24;
    ops.channel = 2;
    ops.sampling = ST30_SAMPLING_48K;
    ops.ptime = ST30_PTIME_1MS;
    
    ops.framebuff_size = 48 * 2 * 3;                 // Match TX
    ops.framebuff_cnt = 4;
    ops.name = "st30p_rx_demo";
    ops.flags = ST30P_RX_FLAG_BLOCK_GET;
    
    st30p_rx_handle handle = st30p_rx_create(mtl, &ops);
    if (!handle) return -1;
    
    while (running) {
        struct st30_frame* frame = st30p_rx_get_frame(handle);
        if (!frame) continue;
        
        // Process received audio
        play_audio_samples(frame->addr, frame->data_size);
        
        st30p_rx_put_frame(handle, frame);
    }
    
    st30p_rx_free(handle);
    return 0;
}
```

### Example 6: Non-Blocking Mode with Callbacks

```c
#include <mtl/st_pipeline_api.h>

// Callback when frame becomes available
static int frame_available_callback(void* priv) {
    // Wake up processing thread or set event flag
    sem_post((sem_t*)priv);
    return 0;
}

int st20p_non_blocking_example(mtl_handle mtl) {
    sem_t frame_ready_sem;
    sem_init(&frame_ready_sem, 0, 0);
    
    struct st20p_tx_ops ops = {0};
    // ... configure ops fields ...
    
    // Setup callback (runs from lcore tasklet)
    ops.priv = &frame_ready_sem;
    ops.notify_frame_available = frame_available_callback;
    ops.flags = 0;  // Non-blocking mode (no BLOCK_GET flag)
    
    st20p_tx_handle handle = st20p_tx_create(mtl, &ops);
    if (!handle) return -1;
    
    while (running) {
        // Wait for callback notification
        sem_wait(&frame_ready_sem);
        
        // Non-blocking get (returns NULL if no frame)
        struct st_frame* frame = st20p_tx_get_frame(handle);
        if (frame) {
            fill_frame(frame);
            st20p_tx_put_frame(handle, frame);
        }
    }
    
    st20p_tx_free(handle);
    sem_destroy(&frame_ready_sem);
    return 0;
}
```

### Example 7: External Frame Mode (Zero-Copy)

```c
#include <mtl/st_pipeline_api.h>

void external_frame_free(void* opaque) {
    // Free external buffer allocated by custom allocator
    custom_buffer_free(opaque);
}

int st20p_external_frame_example(mtl_handle mtl) {
    struct st20p_tx_ops ops = {0};
    // ... configure ops fields ...
    
    ops.flags = ST20P_TX_FLAG_EXT_FRAME;  // Enable external frame mode
    ops.framebuff_cnt = 3;
    
    st20p_tx_handle handle = st20p_tx_create(mtl, &ops);
    if (!handle) return -1;
    
    while (running) {
        struct st_frame* frame = st20p_tx_get_frame(handle);
        if (!frame) continue;
        
        // Allocate external buffer (e.g., GPU memory, DMA buffer)
        void* ext_buffer = custom_buffer_alloc(frame->buffer_size);
        
        // Fill external buffer
        fill_buffer(ext_buffer, frame->buffer_size);
        
        // Prepare external frame descriptor
        struct st_ext_frame ext_frame = {0};
        ext_frame.addr[0] = ext_buffer;
        ext_frame.size = frame->buffer_size;
        ext_frame.opaque = ext_buffer;
        ext_frame.free = external_frame_free;
        
        // Put frame with external buffer (zero-copy)
        st20p_tx_put_ext_frame(handle, frame, &ext_frame);
    }
    
    st20p_tx_free(handle);
    return 0;
}
```

---

## Error Handling

### Return Codes

All API functions follow consistent return code conventions:

| Return Value | Meaning |
|--------------|---------|
| **0** | Success |
| **NULL** | Failed (for handle creation) or no frame available (for get_frame) |
| **< 0** | Error code (negative integer) |

### Common Error Codes

```c
-ENOMEM          // Out of memory
-EINVAL          // Invalid argument
-EBUSY           // Resource busy
-ETIMEDOUT       // Operation timed out (blocking mode)
-ENODEV          // No device/plugin available
-ENOTSUP         // Operation not supported
-EIO             // I/O error
```

### Error Handling Pattern

```c
// Session creation
st20p_tx_handle handle = st20p_tx_create(mtl, &ops);
if (!handle) {
    fprintf(stderr, "Failed to create session: %s\n", strerror(errno));
    return -1;
}

// Frame operations
struct st_frame* frame = st20p_tx_get_frame(handle);
if (!frame) {
    // This is normal - no frame available yet
    // For blocking mode with timeout, check errno:
    if (errno == ETIMEDOUT) {
        fprintf(stderr, "get_frame timeout\n");
    }
}

// API function calls
int ret = st20p_tx_update_destination(handle, &new_dst);
if (ret < 0) {
    fprintf(stderr, "Failed to update destination: %s\n", strerror(-ret));
}

// Cleanup
ret = st20p_tx_free(handle);
if (ret < 0) {
    fprintf(stderr, "Failed to free session: %s\n", strerror(-ret));
}
```

### Frame Status Checking

```c
// RX: Check if frame is complete
struct st_frame* frame = st20p_rx_get_frame(handle);
if (frame) {
    switch (frame->status) {
        case ST_FRAME_STATUS_COMPLETE:
            // All packets received
            process_frame(frame);
            break;
        case ST_FRAME_STATUS_INCOMPLETE:
            // Some packets lost
            fprintf(stderr, "Incomplete frame: %u/%u packets\n",
                    frame->pkts_recv[0], frame->pkts_total);
            // Can still process if ST20P_RX_FLAG_RECEIVE_INCOMPLETE_FRAME is set
            break;
        case ST_FRAME_STATUS_CORRUPTED:
            // Frame data is corrupted
            fprintf(stderr, "Corrupted frame\n");
            break;
    }
    st20p_rx_put_frame(handle, frame);
}
```

---

## Best Practices

### 1. Session Configuration

#### Choose Appropriate Buffer Count

```c
// Minimum: 2 frames (producer-consumer)
ops.framebuff_cnt = 2;  // Not recommended - tight timing

// Recommended: 3-4 frames (allows jitter tolerance)
ops.framebuff_cnt = 3;  // Good for most cases
ops.framebuff_cnt = 4;  // Better for RX with network jitter

// Higher counts: Only if needed (uses more memory)
ops.framebuff_cnt = 6;  // For high-latency processing
```

#### Frame Format Selection

```c
// Prefer planar formats for processing (easier to work with)
ops.input_fmt = ST_FRAME_FMT_YUV422PLANAR10LE;  // Easy to manipulate

// Use packed formats only if required by hardware
ops.input_fmt = ST_FRAME_FMT_V210;  // For specific hardware codecs

// Let MTL handle transport format conversion
ops.transport_fmt = ST20_FMT_YUV_422_10BIT;  // RFC4175 for network
```

#### Plugin Device Selection

```c
// Auto-select: Best for portability
ops.device = ST_PLUGIN_DEVICE_AUTO;

// Explicit: When you know hardware is available
ops.device = ST_PLUGIN_DEVICE_GPU;  // Force GPU encoder

// CPU fallback: Always have CPU plugin as fallback in kahawai.json
```

### 2. Blocking vs Non-Blocking Mode

#### Use Blocking Mode for Simple Applications

```c
// Blocking mode: Simplifies application logic
ops.flags = ST20P_TX_FLAG_BLOCK_GET;
st20p_tx_set_block_timeout(handle, 2000000000);  // 2 second timeout

while (running) {
    struct st_frame* frame = st20p_tx_get_frame(handle);  // Blocks here
    if (frame) {
        fill_frame(frame);
        st20p_tx_put_frame(handle, frame);
    }
}
```

#### Use Non-Blocking Mode with Callbacks for Performance

```c
// Non-blocking: Better CPU efficiency
ops.flags = 0;  // No BLOCK_GET flag
ops.notify_frame_available = frame_ready_callback;
ops.priv = &app_context;

// Callback wakes up processing thread
static int frame_ready_callback(void* priv) {
    app_context_t* ctx = (app_context_t*)priv;
    sem_post(&ctx->frame_sem);  // Wake up worker thread
    return 0;
}
```

### 3. ST22P Codec Configuration

#### JPEGXS Compression Ratio

```c
// Calculate codestream size based on compression ratio
size_t raw_size = width * height * bytes_per_pixel;

// Low compression (high quality): 4:1 to 6:1
ops.codestream_size = raw_size / 5;

// Medium compression: 8:1 to 12:1
ops.codestream_size = raw_size / 10;

// High compression (testing only): 15:1+
ops.codestream_size = raw_size / 15;
```

#### Quality vs Speed Trade-off

```c
// For broadcast production (quality critical)
ops.quality = ST22_QUALITY_MODE_QUALITY;
ops.codec_thread_cnt = 8;  // More threads for multi-pass encoding

// For low-latency (speed critical)
ops.quality = ST22_QUALITY_MODE_SPEED;
ops.codec_thread_cnt = 0;  // Auto thread count
```

### 4. Memory and Performance Optimization

#### External Frame Mode for Zero-Copy

```c
// Use external frames to avoid memory copy
ops.flags = ST20P_TX_FLAG_EXT_FRAME;

// Application manages buffer lifecycle
struct st_ext_frame ext_frame;
ext_frame.addr[0] = gpu_buffer;  // GPU memory, DMA buffer, etc.
ext_frame.free = custom_free_func;
st20p_tx_put_ext_frame(handle, frame, &ext_frame);
```

#### NUMA Awareness

```c
// Force session to specific NUMA node for performance
ops.flags = ST20P_TX_FLAG_FORCE_NUMA;
ops.socket_id = 0;  // NUMA node 0

// Match NUMA node with NIC location for best performance
// Check NIC NUMA: cat /sys/class/net/eth0/device/numa_node
```

#### DMA Offload for RX

```c
// Enable DMA offload for large frames (RX only)
ops.flags = ST20P_RX_FLAG_DMA_OFFLOAD;

// MTL will use DMA devices if available, fallback to CPU if not
// DMA devices configured in mtl_init_params.dma_dev_port
```

### 5. Redundancy and Reliability

#### Enable Redundant Path (2022-7)

```c
// Configure primary and redundant ports
ops.port.num_port = 2;
snprintf(ops.port.port[0], MTL_PORT_MAX_LEN, "0000:af:00.0");  // Primary
snprintf(ops.port.port[1], MTL_PORT_MAX_LEN, "0000:af:00.1");  // Redundant
inet_pton(AF_INET, "239.168.85.20", ops.port.dip_addr[0]);
inet_pton(AF_INET, "239.168.86.20", ops.port.dip_addr[1]);  // Different network
ops.port.udp_port[0] = 20000;
ops.port.udp_port[1] = 20000;
```

#### Handle Incomplete Frames

```c
// RX: Optionally receive incomplete frames
ops.flags = ST20P_RX_FLAG_RECEIVE_INCOMPLETE_FRAME;

// Check frame status before processing
if (frame->status == ST_FRAME_STATUS_COMPLETE) {
    // Perfect frame
} else {
    // Handle packet loss
    float packet_loss = 1.0 - (float)frame->pkts_recv[0] / frame->pkts_total;
    if (packet_loss < 0.01) {  // < 1% loss
        // Still usable
    }
}
```

### 6. Debugging and Monitoring

#### Enable PCAP Dump for Analysis

```c
// Dump RX packets for analysis
struct st_pcap_dump_meta meta;
int ret = st20p_rx_pcapng_dump(handle, 1000, true, &meta);
if (ret == 0) {
    printf("Dumped %u packets to %s\n", meta.dumped_packets, meta.file_name);
}
```

#### Monitor Port Statistics

```c
// Get TX port statistics
struct st20_tx_port_status stats;
st20p_tx_get_port_stats(handle, MTL_SESSION_PORT_P, &stats);
printf("TX: %lu packets, %lu bytes\n", stats.pkts, stats.bytes);

// Reset statistics
st20p_tx_reset_port_stats(handle, MTL_SESSION_PORT_P);
```

#### Use Timing Parser for RX Analysis

```c
// Enable timing analysis
ops.flags = ST20P_RX_FLAG_TIMING_PARSER_STAT |   // Show in stats dump
            ST20P_RX_FLAG_TIMING_PARSER_META;    // Return in frame meta

// Access timing info in frame
struct st_frame* frame = st20p_rx_get_frame(handle);
// Timing data available in frame metadata
```

### 7. Resource Management

#### Session Lifecycle

```c
// 1. Initialize MTL
mtl_handle mtl = mtl_init(&init_params);

// 2. Create sessions
st20p_tx_handle tx = st20p_tx_create(mtl, &tx_ops);
st20p_rx_handle rx = st20p_rx_create(mtl, &rx_ops);

// 3. Use sessions
// ... transmission/reception loop ...

// 4. Free sessions (in reverse order of creation)
st20p_rx_free(rx);
st20p_tx_free(tx);

// 5. Uninitialize MTL
mtl_uninit(mtl);
```

#### Thread Management

```c
// Keep get_frame/put_frame in same thread for best performance
void* tx_thread(void* arg) {
    st20p_tx_handle handle = (st20p_tx_handle)arg;
    
    while (running) {
        struct st_frame* frame = st20p_tx_get_frame(handle);
        if (frame) {
            // Process in same thread
            fill_frame(frame);
            st20p_tx_put_frame(handle, frame);
        }
    }
    return NULL;
}
```

### 8. Common Pitfalls to Avoid

```c
// ❌ DON'T: Forget to put_frame
struct st_frame* frame = st20p_tx_get_frame(handle);
// ... process ...
// MISSING: st20p_tx_put_frame(handle, frame);  // Causes buffer leak!

// ✅ DO: Always pair get with put
struct st_frame* frame = st20p_tx_get_frame(handle);
if (frame) {
    process_frame(frame);
    st20p_tx_put_frame(handle, frame);  // Always return frame
}

// ❌ DON'T: Use blocking calls in callbacks
int callback(void* priv) {
    st20p_tx_get_frame(handle);  // Runs in lcore - can deadlock!
    return 0;
}

// ✅ DO: Signal other threads from callbacks
int callback(void* priv) {
    sem_post(&frame_sem);  // Non-blocking signal
    return 0;
}

// ❌ DON'T: Modify frame after put_frame
st20p_tx_put_frame(handle, frame);
memcpy(frame->addr[0], data, size);  // Frame is now owned by MTL!

// ✅ DO: Modify only between get and put
struct st_frame* frame = st20p_tx_get_frame(handle);
memcpy(frame->addr[0], data, size);  // Safe: we own the frame
st20p_tx_put_frame(handle, frame);   // Now MTL owns it

// ❌ DON'T: Mix blocking flag without timeout adjustment
ops.flags = ST20P_TX_FLAG_BLOCK_GET;
// Default timeout is 1 second - may be too short
st20p_tx_handle handle = st20p_tx_create(mtl, &ops);

// ✅ DO: Set appropriate timeout
st20p_tx_handle handle = st20p_tx_create(mtl, &ops);
st20p_tx_set_block_timeout(handle, 5000000000ULL);  // 5 seconds
```

---

## Quick Reference Summary

### API Naming Pattern

```
st<TYPE>p_<DIR>_<ACTION>

<TYPE> = 20, 22, 30 (standard number)
<DIR>  = tx, rx (direction)
<ACTION> = create, free, get_frame, put_frame, etc.

Examples:
- st20p_tx_create()      // ST2110-20 pipeline TX create
- st22p_rx_get_frame()   // ST2110-22 pipeline RX get frame
- st30p_tx_put_frame()   // ST2110-30 pipeline TX put frame
```

### Typical TX Workflow

```
1. Configure ops structure (st20p_tx_ops / st22p_tx_ops / st30p_tx_ops)
2. Create session: handle = st<X>p_tx_create(mtl, &ops)
3. Loop:
   a. Get frame: frame = st<X>p_tx_get_frame(handle)
   b. Fill frame data
   c. Put frame: st<X>p_tx_put_frame(handle, frame)
4. Free session: st<X>p_tx_free(handle)
```

### Typical RX Workflow

```
1. Configure ops structure (st20p_rx_ops / st22p_rx_ops / st30p_rx_ops)
2. Create session: handle = st<X>p_rx_create(mtl, &ops)
3. Loop:
   a. Get frame: frame = st<X>p_rx_get_frame(handle)
   b. Check frame->status
   c. Process frame data
   d. Put frame: st<X>p_rx_put_frame(handle, frame)
4. Free session: st<X>p_rx_free(handle)
```

### Format Conversion Summary

| Pipeline | Application Format | Transport Format | Conversion |
|----------|-------------------|------------------|------------|
| ST20P TX | Raw pixels (YUV/RGB) | RFC4175 packed | Plugin (CPU/GPU/FPGA) |
| ST20P RX | Raw pixels (YUV/RGB) | RFC4175 packed | Plugin (CPU/GPU/FPGA) |
| ST22P TX | Raw pixels (YUV/RGB) | Encoded (JPEGXS/H264/H265) | Encoder plugin |
| ST22P RX | Raw pixels (YUV/RGB) | Encoded (JPEGXS/H264/H265) | Decoder plugin |
| ST30P TX | PCM samples | RTP audio | None (built-in) |
| ST30P RX | PCM samples | RTP audio | None (built-in) |

---

## Additional Resources

### Source Code References

- **Pipeline API Header:** `include/st_pipeline_api.h`
- **Audio Pipeline Header:** `include/st30_pipeline_api.h`
- **ST20P TX Implementation:** `lib/src/st2110/pipeline/st20_pipeline_tx.c`
- **ST20P RX Implementation:** `lib/src/st2110/pipeline/st20_pipeline_rx.c`
- **ST22P TX Implementation:** `lib/src/st2110/pipeline/st22_pipeline_tx.c`
- **ST22P RX Implementation:** `lib/src/st2110/pipeline/st22_pipeline_rx.c`
- **ST30P TX Implementation:** `lib/src/st2110/pipeline/st30_pipeline_tx.c`
- **ST30P RX Implementation:** `lib/src/st2110/pipeline/st30_pipeline_rx.c`
- **Plugin Manager:** `lib/src/st2110/pipeline/st_plugin.c`

### Sample Applications

- **ST20P Sample:** `app/sample/rx_st20p_sample.c`, `app/sample/tx_st20p_sample.c`
- **ST22P Sample:** `app/sample/rx_st22p_sample.c`, `app/sample/tx_st22p_sample.c`
- **ST30P Sample:** `app/sample/rx_st30p_sample.c`, `app/sample/tx_st30p_sample.c`
- **FFmpeg Plugin:** `ecosystem/ffmpeg_plugin/mtl_st22p_tx.c` (demonstrates ST22P usage)

### Configuration Files

- **Plugin Configuration:** `kahawai.json` (defines available plugins and priority)

### Related Documentation

- **Architecture Guide:** `applications/study-docs/mtl_pipeline_architecture_guide.md`
- **Build Guide:** `doc/build.md`
- **Run Guide:** `doc/run.md`
- **Design Document:** `doc/design.md`

---

**Document Version:** 1.0  
**Last Updated:** February 11, 2026  
**Author:** Generated from Media Transport Library API headers and implementation  
**License:** BSD-3-Clause

