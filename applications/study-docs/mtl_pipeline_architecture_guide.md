# Media Transport Library: Pipeline Architecture Guide

**Date:** February 11, 2026  
**Repository:** OpenVisualCloud/Media-Transport-Library  
**Documentation:** Pipeline Mode vs Raw Mode - Complete Internal Architecture

---

## Table of Contents

1. [Overview](#overview)
2. [Pipeline Mode vs Raw Mode](#pipeline-mode-vs-raw-mode)
3. [Pipeline Mode Internal Architecture](#pipeline-mode-internal-architecture)
4. [Internal Components Deep Dive](#internal-components-deep-dive)
5. [Data Flow Examples](#data-flow-examples)
6. [Plugin System](#plugin-system)
7. [Code References](#code-references)

---

## Overview

The Media Transport Library (MTL) is a software-based solution for high-throughput, low-latency transmission and reception of ST2110 compliant media data. It provides two distinct approaches for handling video streams:

1. **Pipeline Mode** (Recommended) - High-level abstraction with plugin support
2. **Raw/Codestream Mode** - Low-level direct control

---

## Pipeline Mode vs Raw Mode

### MTL Modes Available

The library discovered in the FFmpeg plugin (`mtl_st22p_tx.c`) demonstrates two modes for ST22:

#### **Mode 1: mtl_st22p** - ST22 Pipeline Mode
- **Purpose:** Handles uncompressed pixel formats with automatic compression
- **Input Formats:**
  - `AV_PIX_FMT_YUV422P10LE` → `ST_FRAME_FMT_YUV422PLANAR10LE`
  - `AV_PIX_FMT_RGB24` → `ST_FRAME_FMT_RGB8`
  - `AV_PIX_FMT_YUV420P` → `ST_FRAME_FMT_YUV420PLANAR8`
- **Codecs:** JPEGXS (default), configurable via `st22_codec` parameter
- **Operation:** Performs pixel format conversion and compression automatically

#### **Mode 2: mtl_st22** - ST22 Raw Codestream Mode
- **Purpose:** Handles pre-encoded/compressed video streams
- **Input Formats:**
  - `AV_CODEC_ID_H264` → `ST_FRAME_FMT_H264_CODESTREAM`
  - `AV_CODEC_ID_H265` → `ST_FRAME_FMT_H265_CODESTREAM`
  - `jpegxs` → `ST_FRAME_FMT_JPEGXS_CODESTREAM`
- **Operation:** Direct transport of compressed codestreams without re-encoding

### Comparison Table

| Feature | Pipeline Mode | Raw/Codestream Mode |
|---------|--------------|---------------------|
| **API Prefix** | `st20p_`, `st22p_`, `st30p_` | `st20_`, `st22_`, `st30_` |
| **Complexity** | Low (recommended) | High |
| **Codec Handling** | Automatic via plugins | Manual by application |
| **Frame Format** | Raw pixels (YUV, RGB) | Pre-encoded streams |
| **Plugin Support** | Yes (CPU/GPU/FPGA) | No |
| **Control Level** | High-level abstraction | Low-level control |
| **Use Case** | Standard video workflows | Custom codec implementations |
| **Session Types** | Frame-level with plugins | Frame-level or RTP-level |

### Common Features

Both modes support:
- `ST22_PACK_CODESTREAM` packing type
- Same device arguments and port configurations
- Blocking frame get mode (`ST22P_TX_FLAG_BLOCK_GET`)
- Frame buffer count: 3-8 frames (default 3)
- Codec thread count: 0-64 threads (default 0 = auto)

---

## Pipeline Mode Internal Architecture

### High-Level Architecture

```
┌────────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                            │
│  (Works with RAW pixels - YUV/RGB frames only)                     │
│  • st20p_tx_get_frame() / st22p_tx_get_frame()                     │
│  • Fill with raw pixel data                                         │
│  • st20p_tx_put_frame() / st22p_tx_put_frame()                     │
└──────────────────────┬─────────────────────────────────────────────┘
                       │
                       ▼
┌────────────────────────────────────────────────────────────────────┐
│                    PIPELINE API LAYER                               │
│    st20p_tx/rx, st22p_tx/rx, st30p_tx/rx handles                   │
│    • Frame buffer management (circular buffers)                     │
│    • State machine control (FREE→READY→ENCODING→TRANSMITTED)       │
│    • Blocking/non-blocking get/put operations                       │
│    • Thread synchronization (mutexes, condition variables)          │
└──────────────────────┬─────────────────────────────────────────────┘
                       │
         ┌─────────────┴─────────────┐
         ▼                           ▼
┌──────────────────┐         ┌──────────────────┐
│  PLUGIN LAYER    │         │  INTERNAL        │
│  (External)      │         │  CONVERTER       │
│  • ST22 Encoder  │         │  (Built-in)      │
│  • ST22 Decoder  │         │  • SIMD CSC      │
│  • ST20 CSC      │         │  • Color Space   │
│  CPU/GPU/FPGA    │         │  • Format Conv   │
└────────┬─────────┘         └────────┬─────────┘
         │                            │
         └────────────┬───────────────┘
                      ▼
┌────────────────────────────────────────────────────────────────────┐
│                  TRANSPORT LAYER (ST20/ST22 Raw)                    │
│    • RTP packetization/depacketization                              │
│    • Network transmission (DPDK PMD / Kernel / AF_XDP)             │
│    • Pacing control (RL hardware offload or TSC software)           │
│    • 2022-7 redundancy support                                      │
└────────────────────────────────────────────────────────────────────┘
```

### Available Pipeline APIs

#### **ST20P** - ST2110-20 Uncompressed Video Pipeline
- Application works with **raw pixel formats** (YUV422, RGB8, etc.)
- MTL handles color space conversion via converter plugins
- Defined in `include/st_pipeline_api.h`

#### **ST22P** - ST2110-22 Compressed Video Pipeline
- Application works with **raw pixel frames**
- MTL handles encoding/decoding via codec plugins
- Supports: JPEGXS, H.264, H.265
- MTL internally compresses/decompresses using registered plugins

#### **ST30P** - ST2110-30 Audio Pipeline
- Application works with PCM audio frames
- MTL handles audio pacing and RTP transport

### Workflow Example: ST22P TX

```c
// Create session with codec parameters
st22p_tx_handle handle = st22p_tx_create(mtl_handle, &ops);

while (active) {
    // Get frame buffer from pipeline
    struct st_frame* frame = st22p_tx_get_frame(handle);
    if (!frame) continue;
    
    // Fill with raw pixel data (YUV, RGB, etc.)
    memcpy(frame->addr[0], raw_pixels, frame_size);
    
    // Put frame back - MTL handles encoding & transmission automatically
    st22p_tx_put_frame(handle, frame);
}

// Clean up
st22p_tx_free(handle);
```

### Workflow Example: ST22 Raw TX (Comparison)

```c
// Create raw session
st22_tx_handle handle = st22_tx_create(mtl_handle, &ops);

while (active) {
    // Application must encode raw video to H.264/H.265/JPEGXS
    encode_frame(raw_yuv, codestream_buffer, &codestream_size);
    
    // Get frame buffer
    struct st_frame* frame = st22_tx_get_framebuffer(handle);
    if (!frame) continue;
    
    // Copy pre-encoded codestream
    memcpy(frame->addr[0], codestream_buffer, codestream_size);
    frame->data_size = codestream_size;
    
    // Put framebuffer - MTL transmits pre-encoded stream
    st22_tx_put_framebuffer(handle, frame);
}

st22_tx_free(handle);
```

---

## Internal Components Deep Dive

### 1. Pipeline Context Structures

#### ST20P TX Context (`st20p_tx_ctx`)
**Location:** `lib/src/st2110/pipeline/st20_pipeline_tx.h`

```c
struct st20p_tx_ctx {
    struct mtl_main_impl* impl;        // Main MTL instance
    int idx;                           // Session index
    int socket_id;                     // NUMA socket ID
    enum mt_handle_type type;          // Handle type validation
    
    char ops_name[ST_MAX_NAME_LEN];    // Session name
    struct st20p_tx_ops ops;           // User configuration
    st20_tx_handle transport;          // Underlying ST20 transport handle
    
    // Frame buffer management (circular buffer)
    uint16_t framebuff_cnt;            // Total frame buffers (3-8)
    uint16_t framebuff_producer_idx;   // Producer index (app writes here)
    uint16_t framebuff_convert_idx;    // Converter index
    uint16_t framebuff_consumer_idx;   // Consumer index (transport reads)
    struct st20p_tx_frame* framebuffs; // Frame buffer array
    pthread_mutex_t lock;              // Thread protection for state changes
    
    // Converter session (for color space conversion)
    struct st20_convert_session_impl* convert_impl;  // Plugin converter
    struct st_frame_converter* internal_converter;   // Built-in SIMD converter
    
    // Blocking mode support
    bool block_get;                    // Blocking mode enabled
    pthread_cond_t block_wake_cond;    // Condition variable for blocking
    pthread_mutex_t block_wake_mutex;  // Mutex for condition variable
    uint64_t block_timeout_ns;         // Timeout in nanoseconds (default 1s)
    
    // Operational flags
    bool ready;                        // Pipeline ready state
    bool derive;                       // No conversion needed (direct pass)
    bool dynamic_ext_frame;            // External frame dynamically queried
    int usdt_frame_cnt;                // USDT tracepoint counter
    
    size_t src_size;                   // Source frame size
    
    // Statistics
    rte_atomic32_t stat_convert_fail;  // Conversion failure counter
    int stat_get_frame_try;            // Get frame attempts
    int stat_get_frame_succ;           // Successful gets
    int stat_put_frame;                // Put frame count
};
```

#### ST22P TX Context (`st22p_tx_ctx`)
**Location:** `lib/src/st2110/pipeline/st22_pipeline_tx.h`

```c
struct st22p_tx_ctx {
    struct mtl_main_impl* impl;
    int idx;
    int socket_id;
    enum mt_handle_type type;
    enum st_frame_fmt codestream_fmt;   // Output codec format (JPEGXS, H264, etc.)
    
    char ops_name[ST_MAX_NAME_LEN];
    struct st22p_tx_ops ops;            // User configuration
    st22_tx_handle transport;           // Underlying ST22 transport handle
    
    // Frame buffer management with 3 indices (producer→encoder→consumer)
    uint16_t framebuff_cnt;
    uint16_t framebuff_producer_idx;    // App writes raw pixels here
    uint16_t framebuff_encode_idx;      // Encoder processes from here
    uint16_t framebuff_consumer_idx;    // Transport reads codestream from here
    struct st22p_tx_frame* framebuffs;
    pthread_mutex_t lock;               // Protect framebuffs state
    
    // Application blocking mode
    bool block_get;
    pthread_cond_t block_wake_cond;
    pthread_mutex_t block_wake_mutex;
    uint64_t block_timeout_ns;
    
    // Encoder session (from plugin)
    struct st22_encode_session_impl* encode_impl;
    
    // Encoder blocking mode (separate from app blocking)
    bool encode_block_get;
    pthread_cond_t encode_block_wake_cond;
    pthread_mutex_t encode_block_wake_mutex;
    uint64_t encode_block_timeout_ns;
    
    // Operational flags
    bool ready;                         // Pipeline ready
    bool derive;                        // Input fmt == transport fmt (no encoding)
    bool ext_frame;                     // External frame mode
    bool second_field;                  // For interlaced video
    int usdt_frame_cnt;
    
    size_t src_size;
    
    // Statistics
    rte_atomic32_t stat_encode_fail;    // Encoding failure counter
    int stat_get_frame_try;
    int stat_get_frame_succ;
    int stat_put_frame;
    int stat_encode_get_frame_try;      // Encoder get attempts
    int stat_encode_get_frame_succ;
    int stat_encode_put_frame;
};
```

#### ST20P RX Context (`st20p_rx_ctx`)
**Location:** `lib/src/st2110/pipeline/st20_pipeline_rx.h`

```c
struct st20p_rx_ctx {
    struct mtl_main_impl* impl;
    int idx;
    int socket_id;
    enum mt_handle_type type;
    
    char ops_name[ST_MAX_NAME_LEN];
    struct st20p_rx_ops ops;
    
    st20_rx_handle transport;           // Underlying ST20 RX transport
    uint16_t framebuff_cnt;
    uint16_t framebuff_producer_idx;    // Transport writes received frames
    uint16_t framebuff_convert_idx;     // Converter processes
    uint16_t framebuff_consumer_idx;    // App reads converted frames
    struct st20p_rx_frame* framebuffs;
    pthread_mutex_t lock;
    int usdt_frame_cnt;
    
    // Blocking get support
    bool block_get;
    pthread_cond_t block_wake_cond;
    pthread_mutex_t block_wake_mutex;
    uint64_t block_timeout_ns;
    
    // Converter
    struct st20_convert_session_impl* convert_impl;
    struct st_frame_converter* internal_converter;
    
    bool ready;
    bool derive;                        // No conversion needed
    bool dynamic_ext_frame;             // Query external frame dynamically
    
    size_t dst_size;
    
    // Statistics
    rte_atomic32_t stat_convert_fail;
    rte_atomic32_t stat_busy;           // Busy drop counter
    int stat_get_frame_try;
    int stat_get_frame_succ;
    int stat_put_frame;
};
```

### 2. Frame Buffer State Machines

#### ST20P TX Frame States (Conversion Pipeline)

```
FREE → IN_USER → READY → IN_CONVERTING → CONVERTED → IN_TRANSMITTING → FREE
  ↑      │         │           │              │              │           │
  │      └─────────┴───────────┴──────────────┴──────────────┘           │
  │                                                                       │
  └───────────────────────────────────────────────────────────────────────┘

State Descriptions:
• FREE: Buffer available for application to acquire
• IN_USER: Application is filling with raw pixels
• READY: Frame ready, waiting for color space conversion
• IN_CONVERTING: Color space conversion in progress (plugin or SIMD)
• CONVERTED: Converted to transport format, ready for transmission
• IN_TRANSMITTING: Being transmitted over network via RTP packets
```

#### ST22P TX Frame States (Encoding Pipeline)

```
FREE → IN_USER → READY → IN_ENCODING → ENCODED → IN_TRANSMITTING → FREE
  ↑      │         │          │            │             │           │
  │      └─────────┴──────────┴────────────┴─────────────┘           │
  │                                                                   │
  └───────────────────────────────────────────────────────────────────┘

State Descriptions:
• FREE: Buffer available for application to acquire
• IN_USER: Application is filling with raw pixels
• READY: Frame ready, waiting for codec encoding
• IN_ENCODING: Codec encoding in progress (via plugin: JPEGXS/H264/H265)
• ENCODED: Compressed codestream ready, with data_size set
• IN_TRANSMITTING: Codestream being transmitted over network
```

#### ST20P/ST22P RX Frame States

```
FREE → READY → IN_CONVERTING → CONVERTED → IN_USER → FREE
  ↑      │          │               │          │      │
  │      └──────────┴───────────────┴──────────┘      │
  │                                                    │
  └────────────────────────────────────────────────────┘

State Descriptions:
• FREE: Buffer available for receiving new frame
• READY: Transport frame received from network
• IN_CONVERTING: Color space conversion/decoding in progress
• CONVERTED: Ready for application consumption
• IN_USER: Application is processing the frame
```

**State Enum Definitions:**

```c
// From st20_pipeline_tx.h
enum st20p_tx_frame_status {
    ST20P_TX_FRAME_FREE = 0,
    ST20P_TX_FRAME_READY,
    ST20P_TX_FRAME_IN_CONVERTING,
    ST20P_TX_FRAME_CONVERTED,
    ST20P_TX_FRAME_IN_USER,
    ST20P_TX_FRAME_IN_TRANSMITTING,
    ST20P_TX_FRAME_STATUS_MAX,
};

// From st22_pipeline_tx.h
enum st22p_tx_frame_status {
    ST22P_TX_FRAME_FREE = 0,
    ST22P_TX_FRAME_IN_USER,
    ST22P_TX_FRAME_READY,
    ST22P_TX_FRAME_IN_ENCODING,
    ST22P_TX_FRAME_ENCODED,
    ST22P_TX_FRAME_IN_TRANSMITTING,
    ST22P_TX_FRAME_STATUS_MAX,
};

// From st20_pipeline_rx.h
enum st20p_rx_frame_status {
    ST20P_RX_FRAME_FREE = 0,
    ST20P_RX_FRAME_READY,
    ST20P_RX_FRAME_IN_CONVERTING,
    ST20P_RX_FRAME_CONVERTED,
    ST20P_RX_FRAME_IN_USER,
    ST20P_RX_FRAME_STATUS_MAX,
};
```

### 3. Frame Buffer Structures

#### ST20P Frame Buffer

```c
struct st20p_tx_frame {
    enum st20p_tx_frame_status stat;    // Current state
    struct st_frame src;                // Source frame (raw pixels from app)
    struct st_frame dst;                // Destination (RFC4175 transport format)
    struct st20_convert_frame_meta convert_frame;  // Meta for converter plugin
    uint16_t idx;                       // Frame index
    void* user_meta;                    // User metadata buffer
    size_t user_meta_buffer_size;
    size_t user_meta_data_size;
};

struct st20p_rx_frame {
    enum st20p_rx_frame_status stat;
    struct st_frame src;                // Before converting (RFC4175)
    struct st_frame dst;                // Converted (raw pixels)
    struct st20_convert_frame_meta convert_frame;
    uint16_t idx;
    void* user_meta;
    size_t user_meta_buffer_size;
    size_t user_meta_data_size;
    struct st20_rx_tp_meta tp[MTL_SESSION_PORT_MAX];  // Timing parser metadata
};
```

#### ST22P Frame Buffer

```c
struct st22p_tx_frame {
    enum st22p_tx_frame_status stat;    // Current state
    struct st_frame src;                // Source frame (raw pixels from app)
    struct st_frame dst;                // Destination (compressed codestream)
    struct st22_encode_frame_meta encode_frame;  // Meta for encoder plugin
    uint16_t idx;                       // Frame index
};

struct st22p_rx_frame {
    enum st22p_rx_frame_status stat;
    struct st_frame src;                // Source (compressed codestream)
    struct st_frame dst;                // Destination (raw pixels)
    struct st22_decode_frame_meta decode_frame;  // Meta for decoder plugin
    uint16_t idx;
};
```

#### Universal Frame Structure (`st_frame`)

```c
struct st_frame {
    // Buffer addresses
    void* addr[ST_MAX_PLANES];          // Virtual addresses (max 4 planes)
    mtl_iova_t iova[ST_MAX_PLANES];     // IOVA addresses for DMA
    size_t linesize[ST_MAX_PLANES];     // Bytes per line for each plane
    
    // Format information
    enum st_frame_fmt fmt;              // Frame format (YUV422P10LE, RGB8, etc.)
    bool interlaced;                    // Interlaced or progressive
    bool second_field;                  // Second field indicator for interlaced
    
    // Size information
    size_t buffer_size;                 // Total buffer size (all planes)
    size_t data_size;                   // Valid data size (may be < buffer_size)
    uint32_t width;                     // Frame width in pixels
    uint32_t height;                    // Frame height in pixels
    
    // Timing information
    enum st10_timestamp_fmt tfmt;       // Timestamp format
    uint64_t timestamp;                 // PTP timestamp
    uint64_t epoch;                     // Epoch number
    uint32_t rtp_timestamp;             // RTP timestamp
    
    // Status and metadata
    uint32_t flags;                     // ST_FRAME_FLAG_*
    enum st_frame_status status;        // Frame completeness status
    const void* user_meta;              // User metadata pointer
    size_t user_meta_size;              // User metadata size
    
    // Reception statistics (RX only)
    uint32_t pkts_total;                // Total packets in frame
    uint32_t pkts_recv[MTL_SESSION_PORT_MAX];  // Packets received per port
    
    // Private data
    void* priv;                         // MTL internal use
    void* opaque;                       // Application private data
    struct st20_rx_tp_meta* tp[MTL_SESSION_PORT_MAX];  // Timing parser meta
};
```

### 4. Circular Buffer Management

The pipeline uses a **triple-index circular buffer** pattern:

```
For ST22P TX (3 indices):

    framebuff_producer_idx  →  Where app gets free frames
    framebuff_encode_idx    →  Where encoder gets ready frames  
    framebuff_consumer_idx  →  Where transport gets encoded frames

Frame Buffer Array (circular):
┌─────┬─────┬─────┬─────┬─────┬─────┐
│  0  │  1  │  2  │  3  │  4  │  5  │
└─────┴─────┴─────┴─────┴─────┴─────┘
   ↑           ↑           ↑
   │           │           └─ consumer (transport reads)
   │           └───────────── encode (encoder processes)
   └─────────────────────────producer (app writes)
```

**Index Management Functions:**

```c
// From st22_pipeline_tx.c
static uint16_t tx_st22p_next_idx(struct st22p_tx_ctx* ctx, uint16_t idx) {
    uint16_t next_idx = idx;
    next_idx++;
    if (next_idx >= ctx->framebuff_cnt) next_idx = 0;  // Wrap around
    return next_idx;
}

// Find next frame with desired state
static struct st22p_tx_frame* tx_st22p_next_available(
    struct st22p_tx_ctx* ctx, uint16_t idx_start, 
    enum st22p_tx_frame_status desired) {
    
    uint16_t idx = idx_start;
    struct st22p_tx_frame* framebuff;
    
    // Search circular buffer starting from idx_start
    while (1) {
        framebuff = &ctx->framebuffs[idx];
        if (desired == framebuff->stat) {
            return framebuff;  // Found matching state
        }
        idx = tx_st22p_next_idx(ctx, idx);
        if (idx == idx_start) {
            break;  // Completed full circle
        }
    }
    
    return NULL;  // No frame in desired state
}
```

### 5. Thread Synchronization

#### Mutex Protection
All state changes are protected by mutexes:

```c
mt_pthread_mutex_lock(&ctx->lock);
framebuff->stat = ST22P_TX_FRAME_READY;
ctx->framebuff_producer_idx = next_idx;
mt_pthread_mutex_unlock(&ctx->lock);
```

#### Blocking Mode with Condition Variables

```c
// Blocking get implementation
static int tx_st22p_get_block_wait(struct st22p_tx_ctx* ctx) {
    mt_pthread_mutex_lock(&ctx->block_wake_mutex);
    mt_pthread_cond_timedwait_ns(&ctx->block_wake_cond, 
                                 &ctx->block_wake_mutex,
                                 ctx->block_timeout_ns);  // Default 1 second
    mt_pthread_mutex_unlock(&ctx->block_wake_mutex);
    return 0;
}

// Wake blocked threads
static void tx_st22p_block_wake(struct st22p_tx_ctx* ctx) {
    mt_pthread_mutex_lock(&ctx->block_wake_mutex);
    mt_pthread_cond_signal(&ctx->block_wake_cond);
    mt_pthread_mutex_unlock(&ctx->block_wake_mutex);
}

// Called when frame becomes available
static void tx_st22p_notify_frame_available(struct st22p_tx_ctx* ctx) {
    if (ctx->ops.notify_frame_available) {
        ctx->ops.notify_frame_available(ctx->ops.priv);  // Notify callback
    }
    if (ctx->block_get) {
        tx_st22p_block_wake(ctx);  // Wake blocked thread
    }
}
```

---

## Data Flow Examples

### ST22P TX Complete Data Flow

**Location:** `lib/src/st2110/pipeline/st22_pipeline_tx.c`

```
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 1: Application Requests Frame                                  │
│  API Call: st22p_tx_get_frame(handle)                                │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Pipeline: tx_st22p_get_frame()                                      │
│                                                                       │
│  1. Lock mutex                                                        │
│  2. Search: framebuff = next_available(producer_idx, FREE)           │
│  3. If blocking and no frame: wait on condition variable             │
│  4. Update: framebuff->stat = IN_USER                                │
│  5. Increment: producer_idx = next_idx(producer_idx)                 │
│  6. Unlock mutex                                                      │
│  7. Return: &framebuff->src (raw pixel buffer address)               │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 2: Application Fills Frame                                     │
│  memcpy(frame->addr[0], raw_yuv_pixels, frame_size)                  │
│  frame->data_size = frame_size                                        │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 3: Application Returns Frame                                   │
│  API Call: st22p_tx_put_frame(handle, frame)                         │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Pipeline: tx_st22p_put_frame()                                      │
│                                                                       │
│  1. Lock mutex                                                        │
│  2. Validate: framebuff->stat == IN_USER                             │
│  3. Update: framebuff->stat = READY                                  │
│  4. Unlock mutex                                                      │
│  5. Notify encoder: encode_notify_frame_ready()                      │
│     - Calls plugin's notify_frame_available()                        │
│     - Wakes encoder thread if blocking                               │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 4: Encoder Plugin Requests Frame                               │
│  Plugin calls: encode_get_frame() callback                           │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Pipeline: tx_st22p_encode_get_frame()                               │
│                                                                       │
│  1. Lock mutex                                                        │
│  2. Search: framebuff = next_available(encode_idx, READY)            │
│  3. If blocking and no frame: wait on encoder condition variable     │
│  4. Update: framebuff->stat = IN_ENCODING                            │
│  5. Increment: encode_idx = next_idx(encode_idx)                     │
│  6. Unlock mutex                                                      │
│  7. Setup: encode_frame.src = &framebuff->src (raw pixels)           │
│            encode_frame.dst = &framebuff->dst (codestream buffer)    │
│  8. Return: &framebuff->encode_frame                                 │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 5: Plugin Encodes Frame                                        │
│  Plugin's encode thread:                                              │
│  • Reads raw pixels from encode_frame->src                           │
│  • Encodes to JPEGXS/H264/H265                                        │
│  • Writes codestream to encode_frame->dst                            │
│  • Sets encode_frame->dst->data_size = compressed_size               │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 6: Plugin Returns Encoded Frame                                │
│  Plugin calls: encode_put_frame(encode_frame, result)                │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Pipeline: tx_st22p_encode_put_frame()                               │
│                                                                       │
│  1. Validate: framebuff->stat == IN_ENCODING                         │
│  2. Validate: data_size > MIN_SIZE && data_size <= max_size          │
│  3. If valid:                                                         │
│     - Update: framebuff->stat = ENCODED                              │
│  4. If invalid:                                                       │
│     - Update: framebuff->stat = FREE (recycle)                       │
│     - Increment: stat_encode_fail                                    │
│     - Notify app: frame available again                              │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 7: Transport Layer Requests Frame                              │
│  Transport callback: tx_st22p_next_frame()                           │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Pipeline: tx_st22p_next_frame()                                     │
│                                                                       │
│  1. Lock mutex                                                        │
│  2. Search: framebuff = next_available(consumer_idx, ENCODED)        │
│  3. If no frame: return -EBUSY (transport will retry)                │
│  4. Update: framebuff->stat = IN_TRANSMITTING                        │
│  5. Set: *next_frame_idx = framebuff->idx                            │
│  6. Copy metadata: meta->second_field, timestamp, codestream_size    │
│  7. Increment: consumer_idx = next_idx(consumer_idx)                 │
│  8. Unlock mutex                                                      │
│  9. Return: 0 (success)                                              │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 8: Transport Layer Transmits                                   │
│  • Packetizes codestream into RTP packets                            │
│  • Applies pacing (RL hardware or TSC software)                      │
│  • Transmits via DPDK PMD / Kernel / AF_XDP                          │
│  • Handles 2022-7 redundancy if configured                           │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  STEP 9: Transport Signals Completion                                │
│  Transport callback: tx_st22p_frame_done(frame_idx)                  │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Pipeline: tx_st22p_frame_done()                                     │
│                                                                       │
│  1. Lock mutex                                                        │
│  2. Validate: framebuff[frame_idx]->stat == IN_TRANSMITTING          │
│  3. Update: framebuff->stat = FREE (return to pool)                  │
│  4. Copy timing: framebuff->src/dst.timestamp, rtp_timestamp         │
│  5. Unlock mutex                                                      │
│  6. Notify app callback: ops.notify_frame_done(frame)                │
│  7. Notify app: frame available for reuse                            │
│  8. Wake any blocked get_frame() calls                               │
└──────────────────────────────────────────────────────────────────────┘

                         │
                         ▼
                    ┌────────┐
                    │  LOOP  │
                    └────────┘
```

### ST20P TX Data Flow (Conversion Instead of Encoding)

Similar to ST22P but with color space conversion instead of encoding:

```
Application → get_frame() → Fill raw pixels → put_frame()
    ↓
Ready for conversion
    ↓
Converter plugin (or internal SIMD) converts:
    YUV422P10LE → RFC4175 YUV422 10-bit Big Endian
    ↓
Converted frame ready
    ↓
Transport packetizes and transmits
    ↓
frame_done() → FREE
```

### ST22P RX Data Flow (Reverse)

```
Network packets arrive → Transport depacketizes → Complete frame
    ↓
Ready for decoding
    ↓
Decoder plugin decodes: H264/H265/JPEGXS → Raw YUV
    ↓
Converted frame ready
    ↓
Application: get_frame() → Process → put_frame()
    ↓
FREE → Ready for next frame
```

---

## Plugin System

### Plugin Manager Architecture

**Location:** `lib/src/st2110/pipeline/st_plugin.c`, `st_plugin.h`

```
┌─────────────────────────────────────────────────────────────────────┐
│                   st_plugin_mgr (Plugin Manager)                     │
├─────────────────────────────────────────────────────────────────────┤
│  • Initialized during mtl_init()                                     │
│  • Loads plugins from JSON config (kahawai.json)                    │
│  • Manages device and session lifecycle                             │
├─────────────────────────────────────────────────────────────────────┤
│  Dynamic Plugins:                                                    │
│    plugins[ST_MAX_DL_PLUGINS]  (Loaded .so files)                   │
├─────────────────────────────────────────────────────────────────────┤
│  Registered Devices:                                                 │
│    encode_devs[ST_MAX_ENCODER_DEV]      (ST22 encoders)             │
│    decode_devs[ST_MAX_DECODER_DEV]      (ST22 decoders)             │
│    convert_devs[ST_MAX_CONVERTER_DEV]   (ST20 converters)           │
├─────────────────────────────────────────────────────────────────────┤
│  Thread Safety:                                                      │
│    pthread_mutex_t lock               (Device access)               │
│    pthread_mutex_t plugins_lock       (Plugin loading)              │
└─────────────────────────────────────────────────────────────────────┘
            │
            ├──────────────┬──────────────┬──────────────┐
            ▼              ▼              ▼              ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │   Encoder    │ │   Decoder    │ │  Converter   │ │     More     │
    │   Device     │ │   Device     │ │   Device     │ │   Plugins    │
    ├──────────────┤ ├──────────────┤ ├──────────────┤ ├──────────────┤
    │ • name       │ │ • name       │ │ • name       │ │              │
    │ • priv       │ │ • priv       │ │ • priv       │ │              │
    │ • device     │ │ • device     │ │ • device     │ │              │
    │   (CPU/GPU)  │ │   (CPU/GPU)  │ │   (CPU/GPU)  │ │              │
    │ • caps       │ │ • caps       │ │ • caps       │ │              │
    ├──────────────┤ ├──────────────┤ ├──────────────┤ ├──────────────┤
    │  Sessions:   │ │  Sessions:   │ │  Sessions:   │ │  Sessions:   │
    │  [0..N]      │ │  [0..N]      │ │  [0..N]      │ │  [0..N]      │
    └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘
```

### Plugin Device Structure

#### ST22 Encoder Device

```c
struct st22_encoder_dev {
    const char* name;                      // Device name (e.g., "jpegxs_cpu")
    void* priv;                            // Private data for plugin
    enum st_plugin_device target_device;   // CPU/GPU/FPGA/AUTO
    
    // Capability bitmasks
    uint64_t input_fmt_caps;   // Supported input formats (ST_FMT_CAP_*)
    uint64_t output_fmt_caps;  // Supported output formats (ST_FMT_CAP_*)
    
    // Operations
    st22_encode_priv (*create_session)(
        void* priv,
        st22p_encode_session session_p,
        struct st22_encoder_create_req* req);
        
    int (*notify_frame_available)(st22_encode_priv encode_priv);
    int (*free_session)(void* priv, st22_encode_priv encode_priv);
};
```

#### ST22 Decoder Device

```c
struct st22_decoder_dev {
    const char* name;
    void* priv;
    enum st_plugin_device target_device;
    
    uint64_t input_fmt_caps;   // Codestream formats
    uint64_t output_fmt_caps;  // Raw pixel formats
    
    st22_decode_priv (*create_session)(
        void* priv,
        st22p_decode_session session_p,
        struct st22_decoder_create_req* req);
        
    int (*notify_frame_available)(st22_decode_priv decode_priv);
    int (*free_session)(void* priv, st22_decode_priv decode_priv);
};
```

#### ST20 Converter Device

```c
struct st20_converter_dev {
    const char* name;
    void* priv;
    enum st_plugin_device target_device;
    
    uint64_t input_fmt_caps;   // Input pixel formats
    uint64_t output_fmt_caps;  // Output pixel formats
    
    st20_convert_priv (*create_session)(
        void* priv,
        st20p_convert_session session_p,
        struct st20_converter_create_req* req);
        
    int (*notify_frame_available)(st20_convert_priv convert_priv);
    int (*free_session)(void* priv, st20_convert_priv convert_priv);
};
```

### Plugin Session Allocation

**From:** `lib/src/st2110/pipeline/st_plugin.c`

```c
// Called when creating st22p_tx session
struct st22_encode_session_impl* st22_get_encoder(
    struct mtl_main_impl* impl,
    struct st22_get_encoder_request* req) {
    
    struct st_plugin_mgr* mgr = st_get_plugins_mgr(impl);
    
    mt_pthread_mutex_lock(&mgr->lock);
    
    // Search all registered encoder devices
    for (int i = 0; i < ST_MAX_ENCODER_DEV; i++) {
        dev_impl = mgr->encode_devs[i];
        if (!dev_impl) continue;
        
        dev = &dev_impl->dev;
        
        // Check capability match
        if (!st22_encoder_is_capable(dev, req)) {
            continue;  // Device doesn't support required formats
        }
        
        // Try to allocate session from this device
        session_impl = st22_get_encoder_session(dev_impl, req);
        if (session_impl) {
            rte_atomic32_inc(&dev_impl->ref_cnt);
            mt_pthread_mutex_unlock(&mgr->lock);
            return session_impl;  // Success
        }
    }
    
    mt_pthread_mutex_unlock(&mgr->lock);
    return NULL;  // No capable device found
}

// Capability check
static bool st22_encoder_is_capable(
    struct st22_encoder_dev* dev,
    struct st22_get_encoder_request* req) {
    
    enum st_plugin_device plugin_dev = req->device;
    
    // Check device type match (CPU/GPU/FPGA/AUTO)
    if ((plugin_dev != ST_PLUGIN_DEVICE_AUTO) && 
        (plugin_dev != dev->target_device))
        return false;
    
    // Check input format compatibility
    if (!(MTL_BIT64(req->req.input_fmt) & dev->input_fmt_caps))
        return false;
    
    // Check output format compatibility
    if (!(MTL_BIT64(req->req.output_fmt) & dev->output_fmt_caps))
        return false;
    
    return true;  // Device is capable
}
```

### Plugin Interface Functions

Every plugin must implement these three functions:

```c
// 1. Get plugin metadata
int st_plugin_get_meta(struct st_plugin_meta* meta) {
    meta->version = ST_PLUGIN_VERSION_V1;
    meta->magic = ST_PLUGIN_VERSION_V1_MAGIC;
    return 0;
}

// 2. Create plugin instance (called during mtl_init)
st_plugin_priv st_plugin_create(mtl_handle mt) {
    // Allocate plugin context
    struct my_plugin_ctx* ctx = malloc(sizeof(*ctx));
    
    // Register devices
    struct st22_encoder_dev encoder_dev = {
        .name = "my_encoder",
        .target_device = ST_PLUGIN_DEVICE_CPU,
        .input_fmt_caps = ST_FMT_CAP_YUV422PLANAR10LE,
        .output_fmt_caps = ST_FMT_CAP_JPEGXS_CODESTREAM,
        .create_session = my_encoder_create,
        .notify_frame_available = my_encoder_notify,
        .free_session = my_encoder_free,
    };
    
    ctx->encoder_handle = st22_encoder_register(mt, &encoder_dev);
    
    return (st_plugin_priv)ctx;
}

// 3. Free plugin instance (called during mtl_uninit)
int st_plugin_free(st_plugin_priv handle) {
    struct my_plugin_ctx* ctx = handle;
    
    // Unregister devices
    st22_encoder_unregister(ctx->encoder_handle);
    
    free(ctx);
    return 0;
}
```

### Plugin Loading from JSON

**Configuration:** `kahawai.json` (or path in `KAHAWAI_CFG_PATH` env var)

```json
{
  "plugins": [
    {
      "path": "/usr/local/lib/libst_plugin_jpegxs.so",
      "enabled": true
    },
    {
      "path": "/usr/local/lib/libst_plugin_h264.so",
      "enabled": true
    }
  ]
}
```

**Loading Process:**
1. `mtl_init()` calls `st_plugins_init()`
2. Read JSON configuration file
3. For each plugin path:
   - `dlopen()` the .so file
   - `dlsym()` to get `st_plugin_get_meta`
   - Validate version and magic
   - `dlsym()` to get `st_plugin_create` and `st_plugin_free`
   - Call `st_plugin_create()` to initialize plugin
   - Plugin registers its devices (encoders/decoders/converters)
4. Devices are now available for session allocation

### Format Capability Bitmasks

**From:** `include/st_pipeline_api.h`

```c
// Format capabilities used in plugin caps
#define ST_FMT_CAP_YUV422PLANAR10LE    (MTL_BIT64(ST_FRAME_FMT_YUV422PLANAR10LE))
#define ST_FMT_CAP_V210                (MTL_BIT64(ST_FRAME_FMT_V210))
#define ST_FMT_CAP_YUV422PLANAR8       (MTL_BIT64(ST_FRAME_FMT_YUV422PLANAR8))
#define ST_FMT_CAP_YUV420PLANAR8       (MTL_BIT64(ST_FRAME_FMT_YUV420PLANAR8))
#define ST_FMT_CAP_RGB8                (MTL_BIT64(ST_FRAME_FMT_RGB8))

// Codestream format capabilities
#define ST_FMT_CAP_JPEGXS_CODESTREAM   (MTL_BIT64(ST_FRAME_FMT_JPEGXS_CODESTREAM))
#define ST_FMT_CAP_H264_CODESTREAM     (MTL_BIT64(ST_FRAME_FMT_H264_CODESTREAM))
#define ST_FMT_CAP_H265_CODESTREAM     (MTL_BIT64(ST_FRAME_FMT_H265_CODESTREAM))

// Example: JPEGXS encoder capabilities
input_fmt_caps = ST_FMT_CAP_YUV422PLANAR10LE | ST_FMT_CAP_RGB8;
output_fmt_caps = ST_FMT_CAP_JPEGXS_CODESTREAM;
```

### Example Plugins in Repository

1. **Sample Plugin:** `plugins/sample/st22_plugin_sample.c`
   - Test/reference implementation
   - Demonstrates plugin API usage
   
2. **JPEGXS Plugin:** Not in repo (external)
   - Encoder/decoder for JPEG-XS codec
   
3. **AVCodec Plugin:** `plugins/st22_avcodec/`
   - Uses FFmpeg libavcodec for H.264/H.265
   - Shows real-world plugin implementation
   
4. **Converter Plugin:** `plugins/sample/convert_plugin_sample.c`
   - Color space conversion example

---

## Key Internal Mechanisms

### 1. Derive Mode (Zero-Copy Optimization)

When input format matches transport format, conversion/encoding is bypassed:

```c
// From st22_pipeline_tx.c
if (ctx->derive) {
    // No encoding needed - direct pointer assignment
    framebuff->dst = framebuff->src;
    framebuff->stat = ST22P_TX_FRAME_ENCODED;  // Skip IN_ENCODING state
}

// Example: When sending pre-compressed JPEGXS stream
ops.input_fmt = ST_FRAME_FMT_JPEGXS_CODESTREAM;  // Already compressed
ops.codec = ST22_CODEC_JPEGXS;
// Result: ctx->derive = true, no encoding performed
```

### 2. External Frame Mode (User-Managed Buffers)

Allows zero-copy with application-owned buffers:

```c
// Application provides buffers
struct st_ext_frame ext_frames[FRAME_COUNT];
for (int i = 0; i < FRAME_COUNT; i++) {
    ext_frames[i].addr[0] = my_buffer_pool[i];
    ext_frames[i].iova[0] = get_iova(my_buffer_pool[i]);
    ext_frames[i].size = buffer_size;
}

// Configure pipeline
ops.flags |= ST20P_RX_FLAG_EXT_FRAME;
ops.ext_frames = ext_frames;

// Pipeline uses provided buffers instead of allocating
```

**Dynamic External Frame** (Query per frame):
```c
// Callback to query buffer for each frame
int query_ext_frame_cb(void* priv, struct st_ext_frame* ext_frame,
                       struct st20_rx_frame_meta* meta) {
    // Get buffer from custom pool based on metadata
    void* buf = my_custom_allocator(meta->width, meta->height);
    ext_frame->addr[0] = buf;
    ext_frame->iova[0] = get_iova(buf);
    ext_frame->size = calculate_size(meta);
    ext_frame->opaque = buf;  // For later free
    return 0;
}

ops.flags |= ST20P_RX_FLAG_EXT_FRAME;
ops.query_ext_frame = query_ext_frame_cb;
```

### 3. Packet-Level Conversion (ST20P Optimization)

Convert directly from RTP packets to avoid intermediate buffer:

```c
ops.flags |= ST20P_RX_FLAG_PKT_CONVERT;

// Callback per packet
int pkt_convert_cb(void* priv, void* frame,
                   struct st20_rx_uframe_pg_meta* meta) {
    // Convert single pixel group directly from packet
    struct st20_rfc4175_422_10_pg2_be* pg = meta->payload;
    uint8_t* dst_y = dst_base + offset_y;
    uint8_t* dst_u = dst_base + offset_u;
    uint8_t* dst_v = dst_base + offset_v;
    
    st20_rfc4175_422be10_to_yuv422p10le(pg, dst_y, dst_u, dst_v,
                                        meta->pg_cnt, row_stride);
    return 0;
}

ops.notify_rtp_ready = pkt_convert_cb;
```

### 4. Blocking vs Non-Blocking Operation

#### Non-Blocking (Default)
```c
// Returns immediately if no frame available
struct st_frame* frame = st22p_tx_get_frame(handle);
if (!frame) {
    // Do other work or retry later
    return;
}
```

#### Blocking Mode
```c
ops.flags |= ST22P_TX_FLAG_BLOCK_GET;
st22p_tx_handle handle = st22p_tx_create(mtl, &ops);

// Blocks up to 1 second waiting for frame
struct st_frame* frame = st22p_tx_get_frame(handle);
// frame is guaranteed non-NULL (unless error or timeout)
```

#### Custom Timeout
```c
// Set custom timeout (e.g., 500ms)
uint64_t timeout_ns = 500 * NS_PER_MS;
st22p_tx_set_block_timeout(handle, timeout_ns);
```

#### Wake Blocked Thread
```c
// From another thread, wake blocked get_frame() call
st22p_tx_wake_block(handle);
```

### 5. Statistics and Monitoring

Each context maintains counters accessed via internal dump:

```c
// From st22_pipeline_tx.c
static int tx_st22p_dump(void* priv) {
    struct st22p_tx_ctx* ctx = priv;
    
    notice("TX_st22p(%s): producer(%d:%s) encoder(%d:%s) consumer(%d:%s)\n",
           ctx->ops_name,
           ctx->framebuff_producer_idx, 
           tx_st22p_stat_name(ctx->framebuffs[ctx->framebuff_producer_idx].stat),
           ctx->framebuff_encode_idx,
           tx_st22p_stat_name(ctx->framebuffs[ctx->framebuff_encode_idx].stat),
           ctx->framebuff_consumer_idx,
           tx_st22p_stat_name(ctx->framebuffs[ctx->framebuff_consumer_idx].stat));
    
    int encode_fail = rte_atomic32_read(&ctx->stat_encode_fail);
    if (encode_fail) {
        notice("TX_st22p(%s): encode fail %d\n", ctx->ops_name, encode_fail);
    }
    
    if (ctx->stat_get_frame_try) {
        notice("TX_st22p(%s): get_frame try %d succ %d (%.2f%%)\n",
               ctx->ops_name, ctx->stat_get_frame_try, ctx->stat_get_frame_succ,
               100.0 * ctx->stat_get_frame_succ / ctx->stat_get_frame_try);
    }
    
    return 0;
}
```

### 6. USDT Tracepoints (User-Level Statically Defined Tracing)

For low-overhead performance analysis:

```c
// Tracepoints in st22_pipeline_tx.c
MT_USDT_ST22P_TX_FRAME_GET(idx, frame_idx, addr);
MT_USDT_ST22P_TX_FRAME_PUT(idx, frame_idx, addr);
MT_USDT_ST22P_TX_ENCODE_GET(idx, frame_idx, src_addr, dst_addr);
MT_USDT_ST22P_TX_ENCODE_PUT(idx, frame_idx, src_addr, dst_addr, result, size);
MT_USDT_ST22P_TX_FRAME_NEXT(idx, frame_idx);
MT_USDT_ST22P_TX_FRAME_DONE(idx, frame_idx, rtp_timestamp);
```

**Usage with SystemTap/BPF:**
```bash
# List available probes
sudo bpftrace -l 'usdt:/path/to/app:*st22p*'

# Trace encoder performance
sudo bpftrace -e '
    usdt:/path/to/app:st22p_tx_encode_get { @start[arg1] = nsecs; }
    usdt:/path/to/app:st22p_tx_encode_put { 
        @encode_latency = hist(nsecs - @start[arg1]);
        delete(@start[arg1]);
    }
'
```

### 7. Thread Safety Features

#### Mutex Protection
- `ctx->lock`: Protects frame buffer state transitions
- `ctx->block_wake_mutex`: Protects condition variable operations
- Plugin manager lock: Protects device registration/allocation

#### Lock-Free Searching
- `next_available()` searches without lock (reads only)
- Lock acquired only for state modification
- Minimizes contention in multi-threaded scenarios

#### Reference Counting
```c
// Device reference counting for safe cleanup
rte_atomic32_t ref_cnt;  // In device impl

// Increment when session allocated
rte_atomic32_inc(&dev_impl->ref_cnt);

// Decrement when session freed
rte_atomic32_dec(&dev_impl->ref_cnt);

// Device not freed until ref_cnt == 0
```

---

## Code References

### Core Pipeline Implementation Files

#### ST20 Pipeline (Uncompressed Video)
- **TX:** `lib/src/st2110/pipeline/st20_pipeline_tx.c` / `.h`
- **RX:** `lib/src/st2110/pipeline/st20_pipeline_rx.c` / `.h`
  - Frame state management
  - Color space conversion integration
  - Circular buffer management
  - Blocking/non-blocking operations

#### ST22 Pipeline (Compressed Video)
- **TX:** `lib/src/st2110/pipeline/st22_pipeline_tx.c` / `.h`
- **RX:** `lib/src/st2110/pipeline/st22_pipeline_rx.c` / `.h`
  - Encoder/decoder plugin integration
  - Codestream handling
  - Triple-index buffer management (producer→encoder→consumer)

#### ST30 Pipeline (Audio)
- **TX:** `lib/src/st2110/pipeline/st30_pipeline_tx.c` / `.h`
- **RX:** `lib/src/st2110/pipeline/st30_pipeline_rx.c` / `.h`
  - PCM audio frame handling

#### Plugin Manager
- **Core:** `lib/src/st2110/pipeline/st_plugin.c` / `.h`
  - Plugin loading (dlopen)
  - Device registration
  - Session allocation
  - Capability matching

### API Headers

#### Pipeline API
- **Main:** `include/st_pipeline_api.h`
  - st20p_*, st22p_*, st30p_* function declarations
  - Frame format definitions
  - Plugin device structures
  - Encoder/decoder interfaces

#### Raw API
- **Main:** `include/st20_api.h`
  - st20_*, st22_*, st30_* function declarations
  - RTP-level interfaces
  - Frame-level structures

#### Main API
- **Entry:** `include/st_api.h`
  - mtl_init(), mtl_uninit()
  - Device configuration
  - Port management

### Sample Applications

#### Pipeline Mode Samples
- **ST20P TX:** `app/sample/tx_st20_pipeline_sample.c`
- **ST20P RX:** `app/sample/rx_st20_pipeline_sample.c`
- **ST22P TX:** `app/sample/tx_st22_pipeline_sample.c`
- **ST22P RX:** `app/sample/rx_st22_pipeline_sample.c`
- **ST30P TX:** `app/sample/tx_st30_pipeline_sample.c`
- **ST30P RX:** `app/sample/rx_st30_pipeline_sample.c`

#### Raw Mode Samples (Legacy)
- **ST20 TX:** `app/sample/legacy/tx_video_sample.c`
- **ST20 RX:** `app/sample/legacy/rx_video_sample.c`
- **ST22 TX:** `app/sample/legacy/tx_st22_video_sample.c`
- **ST22 RX:** `app/sample/legacy/rx_st22_video_sample.c`

### FFmpeg Plugin Integration

#### FFmpeg MTL Plugin
- **ST20P Muxer:** `ecosystem/ffmpeg_plugin/mtl_st20p_tx.c`
- **ST20P Demuxer:** `ecosystem/ffmpeg_plugin/mtl_st20p_rx.c`
- **ST22P Muxer:** `ecosystem/ffmpeg_plugin/mtl_st22p_tx.c`
- **ST22P Demuxer:** `ecosystem/ffmpeg_plugin/mtl_st22p_rx.c`
- **ST22 Muxer:** `ecosystem/ffmpeg_plugin/mtl_st22p_tx.c` (raw mode)
- **ST22 Demuxer:** `ecosystem/ffmpeg_plugin/mtl_st22p_rx.c` (raw mode)
- **Common:** `ecosystem/ffmpeg_plugin/mtl_common.c` / `.h`
- **README:** `ecosystem/ffmpeg_plugin/README.md`

### Plugin Examples

#### Sample Plugins
- **ST22 Encoder/Decoder:** `plugins/sample/st22_plugin_sample.c` / `.h`
- **ST20 Converter:** `plugins/sample/convert_plugin_sample.c` / `.h`
- **AVCodec Plugin:** `plugins/st22_avcodec/st22_avcodec_plugin.c` / `.h`
  - H.264/H.265 using FFmpeg libavcodec
  - Real-world plugin implementation

### Documentation

#### Design Documentation
- **Design Guide:** `doc/design.md`
  - Section 6.3: ST20 Pipeline
  - Section 6.4: ST22 Pipeline and Raw modes
  - Architecture overview

#### Plugin Documentation
- **Plugin Guide:** `doc/plugin.md`
  - Plugin interface
  - Registration process
  - Configuration

#### Other Documentation
- **Build Guide:** `doc/build.md`
- **Run Guide:** `doc/run.md`
- **Performance Guide:** `doc/performance.md`
- **External Frame Guide:** `doc/external_frame.md`

### Configuration

#### JSON Configuration
- **Example Config:** `kahawai.json`
  - Plugin registration
  - Device settings

### Testing

#### Unit Tests
- **ST20P Tests:** `tests/unittest/st20p_test.cpp`
- **ST22P Tests:** `tests/unittest/st22p_test.cpp`
  - Frame buffer state machine tests
  - Blocking mode tests
  - Error handling tests

---

## Benefits of Pipeline Architecture

### 1. Separation of Concerns
- **Application:** Handles pixel/audio data processing
- **Pipeline:** Manages buffers, state, synchronization
- **Plugins:** Handle codec/conversion specifics
- **Transport:** Manages network transmission

### 2. Plugin Extensibility
- Add new codecs without changing MTL core
- Support heterogeneous devices (CPU/GPU/FPGA)
- Third-party integration without API changes
- Easy A/B testing of different codec implementations

### 3. Efficient Memory Management
- **Circular buffers:** Predictable memory footprint
- **Zero-copy paths:** External frame mode, derive mode
- **NUMA awareness:** Allocate on socket matching NIC
- **DMA support:** IOVA addresses for hardware acceleration

### 4. Thread Safety
- **Mutex protection:** All state changes are atomic
- **Lock-free reads:** Minimize contention
- **Condition variables:** Efficient blocking without polling
- **Reference counting:** Safe device lifecycle

### 5. Performance Optimization
- **SIMD operations:** Built-in color space converters
- **DMA offload:** Hardware-accelerated memory copy
- **Packet-level conversion:** Eliminate intermediate buffers
- **Pipeline parallelism:** Encode while transmitting previous frame

### 6. Flexibility
- **Blocking/Non-blocking:** Application choice
- **External frames:** Integrate with existing allocators
- **Derive mode:** Bypass unnecessary operations
- **Custom plugins:** Implement proprietary codecs

### 7. Observability
- **Statistics:** Frame rates, failures, busy events
- **USDT tracepoints:** Low-overhead performance analysis
- **Dump functions:** Runtime state inspection
- **Timing metadata:** Per-frame timing information

### 8. Robustness
- **Error handling:** Graceful degradation on encode/convert failures
- **State validation:** Detect state machine violations
- **Frame recycling:** Failed frames returned to pool
- **Timeout protection:** Prevent infinite waits

---

## Summary

The Media Transport Library's **Pipeline Mode** provides a sophisticated yet user-friendly architecture for ST2110 media transport. By abstracting away the complexities of codec management, color space conversion, and network transport, it enables applications to focus on content processing while maintaining high performance and flexibility.

Key architectural components:
- **Multi-index circular buffers** for efficient frame management
- **Plugin system** for extensible codec/converter support
- **State machines** for robust frame lifecycle tracking
- **Thread synchronization** for safe concurrent operation
- **Zero-copy optimizations** for maximum performance

The architecture strikes a balance between:
- **Simplicity** for common use cases (pipeline mode)
- **Control** for advanced requirements (raw mode, external frames)
- **Performance** through optimizations (SIMD, DMA, zero-copy)
- **Extensibility** via the plugin interface

This design has enabled MTL to achieve both high throughput and low latency while maintaining a clean separation between application logic, media processing, and network transport.

---

**Document Version:** 1.0  
**Last Updated:** February 11, 2026  
**Author:** Generated from Media Transport Library source code analysis

