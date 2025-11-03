# FFmpeg with MTL Pipeline Mode: Complete Usage Guide

**Date:** February 16, 2026  
**Repository:** OpenVisualCloud/Media-Transport-Library  
**Purpose:** Comprehensive guide for using FFmpeg plugins with MTL to transmit video over ST2110

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture and Data Flow](#architecture-and-data-flow)
3. [FFmpeg and MTL API Integration](#ffmpeg-and-mtl-api-integration)
4. [Installation and Build](#installation-and-build)
5. [ST20P Uncompressed Video Workflow](#st20p-uncompressed-video-workflow)
6. [ST22P Compressed Video Workflow](#st22p-compressed-video-workflow)
7. [Complete Examples](#complete-examples)
8. [Command Line Parameters](#command-line-parameters)
9. [Advanced Usage](#advanced-usage)
10. [Performance Optimization](#performance-optimization)
11. [Troubleshooting](#troubleshooting)

---

## Overview

### What is FFmpeg with MTL?

The **FFmpeg MTL plugin** integrates Media Transport Library (MTL) with FFmpeg, enabling professional video workflows using ST2110 standards. This allows you to:

- **Read video files** (MP4, MKV, AVI, MOV, etc.) and transmit them as ST2110 streams
- **Receive ST2110 streams** and save them as video files or process in real-time
- Support both **uncompressed (ST2110-20)** and **compressed (ST2110-22)** video transport
- Leverage **FFmpeg's codec ecosystem** (H.264, H.265, JPEGXS, etc.)

### Key Benefits

✅ **File-to-Stream:** Convert any video file format to ST2110 network streams  
✅ **Stream-to-File:** Capture ST2110 streams to standard video files  
✅ **Format Conversion:** Automatic pixel format conversion (RGB ↔ YUV)  
✅ **Codec Integration:** Use any FFmpeg-supported codec with ST2110 transport  
✅ **Pipeline Mode:** Automatic compression/decompression via MTL plugins  
✅ **Production Ready:** Supports professional broadcast workflows

---

## Architecture and Data Flow

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           FFMPEG PROCESS                                 │
│                                                                          │
│  ┌────────────┐    ┌──────────┐    ┌──────────────┐    ┌──────────────┐│
│  │ Input File │───▶│  Demuxer │───▶│   Decoder    │───▶│ Pixel Format ││
│  │  (MP4/MKV) │    │          │    │   (H.264/5)  │    │  Conversion  ││
│  └────────────┘    └──────────┘    └──────────────┘    └──────┬───────┘│
│                                                                 │        │
│                                    Raw YUV/RGB Frames           │        │
│                                           │                     │        │
│                                           ▼                     │        │
│                                  ┌─────────────────┐            │        │
│                                  │  MTL Plugin     │◀───────────┘        │
│                                  │  (ST20P/ST22P)  │                     │
│                                  └────────┬────────┘                     │
└───────────────────────────────────────────┼──────────────────────────────┘
                                            │
                                            ▼
                              ┌──────────────────────────┐
                              │  Media Transport Library │
                              │     (MTL ST2110 Stack)   │
                              └─────────────┬────────────┘
                                            │
                                            ▼
                              ┌──────────────────────────┐
                              │   Network Interface      │
                              │   (Intel E810/XXV710)    │
                              └─────────────┬────────────┘
                                            │
                                            ▼
                              ┌──────────────────────────┐
                              │   ST2110 RTP Packets     │
                              │   239.x.x.x:port         │
                              └──────────────────────────┘
```

### Data Flow: File to ST2110 Stream

#### Path 1: ST20P Uncompressed Video (Recommended for Quality)

```
Input File (test.mp4)
    │
    ├─ Container: MP4/MKV
    ├─ Video Codec: H.264/H.265
    └─ Audio: AAC/PCM
    │
    ▼
FFmpeg Demuxer
    │ Extracts video stream
    ▼
FFmpeg Decoder (libx264/libx265)
    │ Decodes to raw pixels
    ▼
Raw Frames (YUV422P10LE / RGB8)
    │ Uncompressed pixel data
    │ Example: 1920x1080x10-bit = ~5MB/frame
    ▼
MTL ST20P Plugin
    │ get_frame() ← MTL provides buffer
    │ copy raw pixels to buffer
    │ put_frame() → MTL takes ownership
    ▼
MTL Internal Processing
    │ Format conversion (if needed)
    │ RGB → YUV or planar → packed
    ▼
ST2110-20 RTP Packetization
    │ RFC4175 packing
    │ RTP timestamp
    │ Packet pacing
    ▼
Network Transmission
    │ Multicast IP: 239.168.85.20
    │ UDP Port: 20000
    │ Payload Type: 112
    │ Bandwidth: ~3 Gbps (1080p60)
    ▼
Receiver (MTL ST20P or FFmpeg)
```

**Key Point:** This path maintains **maximum quality** as the video is transmitted uncompressed over the network.

---

#### Path 2: ST22P Compressed Video (Bandwidth Efficient)

```
Input File (test.mp4)
    │
    ▼
FFmpeg Demuxer + Decoder
    │ Same as Path 1
    ▼
Raw Frames (YUV422P10LE)
    │
    ▼
MTL ST22P Plugin
    │ get_frame() ← MTL provides buffer
    │ copy raw pixels to buffer
    │ put_frame() → MTL takes ownership
    ▼
MTL Internal Encoding (via plugin)
    │ JPEGXS Encoder (10:1 compression)
    │ OR H.264/H.265 Encoder
    │ Runs in plugin (CPU/GPU/FPGA)
    ▼
Compressed Codestream
    │ Example: 1080p60 @ 10:1 = ~500KB/frame
    ▼
ST2110-22 RTP Packetization
    │ Codestream packing
    │ RTP timestamp
    │ Packet pacing
    ▼
Network Transmission
    │ Multicast IP: 239.168.85.22
    │ UDP Port: 30000
    │ Payload Type: 112
    │ Bandwidth: ~0.3 Gbps (1080p60 @ 10:1)
    ▼
Receiver (MTL ST22P or FFmpeg)
    │
    ▼
MTL Internal Decoding
    │ JPEGXS Decoder
    ▼
Raw Frames (YUV422P10LE)
```

**Key Point:** This path uses **10x less bandwidth** but adds compression latency (~1-2 frames).

---

#### Path 3: ST22 with FFmpeg Encoding (More Control)

```
Input File (test.mp4)
    │
    ▼
FFmpeg Demuxer + Decoder
    │
    ▼
Raw Frames (YUV420P / YUV422P)
    │
    ▼
FFmpeg Encoder (libopenh264 / libsvt_jpegxs)
    │ Encode within FFmpeg process
    │ More codec control/tuning
    ▼
Compressed Codestream (H.264/JPEGXS)
    │
    ▼
MTL ST22 Plugin (Raw Codestream Mode)
    │ get_framebuffer() ← MTL provides buffer
    │ copy encoded stream to buffer
    │ put_framebuffer() → MTL transmits
    ▼
ST2110-22 RTP Packetization
    │ Direct codestream transmission
    ▼
Network Transmission
    │ Bandwidth depends on encoder settings
    ▼
Receiver
    │
    ▼
FFmpeg Decoder
    │ Decode codestream to raw pixels
```

**Key Point:** This path gives you **full control** over encoder settings but doesn't use MTL's plugin system.

---

## FFmpeg and MTL API Integration

### Understanding the Integration

The FFmpeg MTL plugin acts as a **bridge** between FFmpeg's media processing APIs and MTL's ST2110 network transport APIs. This section explains how these two systems work together to send video frames over the network.

### Data Flow Architecture

```
FFmpeg Input → FFmpeg Decode → MTL Frame Buffer → MTL Transmit → Network (ST2110)
(AVPacket)     (Raw Frames)     (st_frame)         (RTP/UDP)      (Multicast)
```

---

### Step 1: Initialization (FFmpeg → MTL)

The `mtl_st20p_write_header()` function bridges FFmpeg format context to MTL:

```c
// Extract video parameters from FFmpeg
ops_tx.width = ctx->streams[0]->codecpar->width;        // FFmpeg API
ops_tx.height = ctx->streams[0]->codecpar->height;      // FFmpeg API
ops_tx.fps = framerate_to_st_fps(ctx->streams[0]->avg_frame_rate); // Convert

// Map FFmpeg pixel format to MTL format
switch (ctx->streams[0]->codecpar->format) {  // FFmpeg pixel format
    case AV_PIX_FMT_YUV422P10LE:
        ops_tx.input_fmt = ST_FRAME_FMT_YUV422PLANAR10LE;  // MTL format
        ops_tx.transport_fmt = ST20_FMT_YUV_422_10BIT;     // Network format
        break;
    case AV_PIX_FMT_RGB24:
        ops_tx.input_fmt = ST_FRAME_FMT_RGB8;
        ops_tx.transport_fmt = ST20_FMT_RGB_8BIT;
        break;
}

// Create MTL device and session
s->dev_handle = mtl_dev_get(ctx, &s->devArgs, &s->idx);  // MTL API
s->tx_handle = st20p_tx_create(s->dev_handle, &ops_tx);  // MTL API
```

**What's happening:**
- FFmpeg `AVFormatContext` provides video parameters (resolution, fps, pixel format)
- Plugin converts FFmpeg enums to MTL enums
- MTL device and TX session are created with these parameters

---

### Step 2: Frame Transmission (FFmpeg AVPacket → MTL st_frame)

The `mtl_st20p_write_packet()` function handles each video frame:

```c
static int mtl_st20p_write_packet(AVFormatContext* ctx, AVPacket* pkt) {
    mtlSt20pMuxerContext* s = ctx->priv_data;
    struct st_frame* frame;
    
    // Step 1: Get empty frame buffer from MTL (BLOCKING)
    frame = st20p_tx_get_frame(s->tx_handle);  // MTL API - waits for available buffer
    if (!frame) {
        return AVERROR(EIO);  // Timeout or error
    }
    
    // Step 2: Copy FFmpeg packet data to MTL frame buffer
    mtl_memcpy(frame->addr[0],    // MTL frame buffer address
               pkt->data,          // FFmpeg packet data (from AVPacket)
               s->frame_size);     // Size in bytes
    
    // Step 3: Submit frame to MTL for transmission
    st20p_tx_put_frame(s->tx_handle, frame);  // MTL API - queues for TX
    
    s->frame_counter++;
    return 0;
}
```

**Key Operations:**
1. **Get Frame Buffer:** `st20p_tx_get_frame()` blocks until MTL has an available frame buffer
2. **Copy Data:** FFmpeg's `AVPacket->data` is copied to MTL's `st_frame->addr[0]`
3. **Submit Frame:** `st20p_tx_put_frame()` queues the frame for network transmission

---

### Step 3: Cleanup (Release Resources)

```c
static int mtl_st20p_write_close(AVFormatContext* ctx) {
    mtlSt20pMuxerContext* s = ctx->priv_data;
    
    // Destroy MTL TX session
    if (s->tx_handle) {
        st20p_tx_free(s->tx_handle);  // MTL API
        s->tx_handle = NULL;
    }
    
    // Destroy MTL device instance
    if (s->dev_handle) {
        mtl_instance_put(ctx, s->dev_handle);  // MTL API
        s->dev_handle = NULL;
    }
    
    return 0;
}
```

---

### API Integration Points

| FFmpeg Component | MTL Component | Purpose |
|-----------------|---------------|------|
| `AVFormatContext` | `mtl_handle` | Device/session management |
| `AVPacket->data` | `st_frame->addr[0]` | Frame buffer data |
| `AVPacket->size` | `st_frame->data_size` | Frame size in bytes |
| `AVPixelFormat` | `st_frame_fmt` | Pixel format mapping |
| `AVRational framerate` | `st_fps` | Frame rate mapping |
| `write_header()` | `st20p_tx_create()` | Initialize TX session |
| `write_packet()` | `st20p_tx_get/put_frame()` | Transmit frames |
| `write_trailer()` | `st20p_tx_free()` | Cleanup session |

---

### Example 1: Using FFmpeg Command Line with MTL Output

```bash
# Send MP4 file to ST2110 network
ffmpeg -re -i input.mp4 \
  -pix_fmt yuv422p10le \
  -f mtl_st20p \
  -p_port 0000:31:00.0 \
  -p_sip 192.168.1.100 \
  -p_tx_ip 239.168.1.1 \
  -udp_port 20000 \
  -fb_cnt 3 \
  mtl:

# Parameters explained:
# -pix_fmt yuv422p10le  → FFmpeg pixel format (must match MTL support)
# -f mtl_st20p          → Use MTL ST20P muxer (this plugin)
# -p_port               → PCIe device (NIC)
# -p_sip                → Source IP (local NIC IP)
# -p_tx_ip              → Destination multicast IP
# -udp_port             → UDP port number
# -fb_cnt               → Frame buffer count (3-8)
```

---

### Example 2: Using FFmpeg APIs in C Code

```c
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int send_video_via_mtl(const char* input_file) {
    AVFormatContext *in_ctx = NULL, *out_ctx = NULL;
    AVPacket *packet = NULL;
    int ret;
    
    // 1. Open input file (FFmpeg API)
    ret = avformat_open_input(&in_ctx, input_file, NULL, NULL);
    if (ret < 0) return ret;
    
    avformat_find_stream_info(in_ctx, NULL);
    
    // 2. Create MTL output context (FFmpeg API)
    avformat_alloc_output_context2(&out_ctx, NULL, "mtl_st20p", NULL);
    
    // 3. Set MTL options (FFmpeg API with MTL parameters)
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "p_port", "0000:31:00.0", 0);      // NIC device
    av_dict_set(&opts, "p_sip", "192.168.1.100", 0);      // Local IP
    av_dict_set(&opts, "p_tx_ip", "239.168.1.1", 0);      // Multicast IP
    av_dict_set(&opts, "udp_port", "20000", 0);           // UDP port
    av_dict_set(&opts, "fb_cnt", "3", 0);                 // Frame buffers
    
    // 4. Create output stream
    AVStream *out_stream = avformat_new_stream(out_ctx, NULL);
    avcodec_parameters_copy(out_stream->codecpar, in_ctx->streams[0]->codecpar);
    out_stream->codecpar->format = AV_PIX_FMT_YUV422P10LE; // MTL-compatible format
    
    // 5. Write header → triggers MTL initialization
    ret = avformat_write_header(out_ctx, &opts);  // Calls mtl_st20p_write_header()
    if (ret < 0) goto cleanup;
    
    // 6. Read and transmit frames
    packet = av_packet_alloc();
    while (av_read_frame(in_ctx, packet) >= 0) {
        if (packet->stream_index == 0) {
            // Write packet → triggers MTL transmission
            ret = av_interleaved_write_frame(out_ctx, packet);  // Calls mtl_st20p_write_packet()
            if (ret < 0) break;
        }
        av_packet_unref(packet);
    }
    
    // 7. Write trailer → triggers MTL cleanup
    av_write_trailer(out_ctx);  // Calls mtl_st20p_write_close()
    
cleanup:
    av_packet_free(&packet);
    avformat_close_input(&in_ctx);
    avformat_free_context(out_ctx);
    av_dict_free(&opts);
    
    return ret;
}
```

**Build and run:**
```bash
# Compile
gcc -o mtl_sender example.c \
    -lavformat -lavcodec -lavutil \
    -lmtl

# Execute
./mtl_sender input.mp4
```

---

### Example 3: Direct MTL API Usage (No FFmpeg)

If you want to use MTL APIs directly without FFmpeg:

```c
#include <mtl/st_pipeline_api.h>

int send_frames_direct_mtl(void) {
    mtl_handle dev_handle;
    st20p_tx_handle tx_handle;
    struct mtl_init_params init_params;
    struct st20p_tx_ops ops;
    
    // 1. Initialize MTL device
    memset(&init_params, 0, sizeof(init_params));
    init_params.num_ports = 1;
    snprintf(init_params.port[0], sizeof(init_params.port[0]), "0000:31:00.0");
    inet_pton(AF_INET, "192.168.1.100", init_params.sip_addr[0]);
    
    dev_handle = mtl_init(&init_params);  // MTL API
    if (!dev_handle) return -1;
    
    // 2. Create TX session
    memset(&ops, 0, sizeof(ops));
    ops.width = 1920;
    ops.height = 1080;
    ops.fps = ST_FPS_P60;
    ops.input_fmt = ST_FRAME_FMT_YUV422PLANAR10LE;
    ops.transport_fmt = ST20_FMT_YUV_422_10BIT;
    ops.framebuff_cnt = 3;
    ops.flags = ST20P_TX_FLAG_BLOCK_GET;
    
    inet_pton(AF_INET, "239.168.1.1", ops.port.dip_addr[0]);
    ops.port.udp_port[0] = 20000;
    
    tx_handle = st20p_tx_create(dev_handle, &ops);  // MTL API
    if (!tx_handle) {
        mtl_uninit(dev_handle);
        return -1;
    }
    
    // 3. Transmit frames in loop
    for (int i = 0; i < 300; i++) {  // Send 300 frames (5 seconds @ 60fps)
        struct st_frame* frame;
        
        // Get frame buffer (blocking)
        frame = st20p_tx_get_frame(tx_handle);  // MTL API
        if (!frame) break;
        
        // Fill frame with your video data
        // Example: generate test pattern or copy from camera/file
        memset(frame->addr[0], 0x80, frame->data_size);  // Gray frame
        
        // Submit frame for transmission
        st20p_tx_put_frame(tx_handle, frame);  // MTL API
    }
    
    // 4. Cleanup
    st20p_tx_free(tx_handle);    // MTL API
    mtl_uninit(dev_handle);       // MTL API
    
    return 0;
}
```

**Build and run:**
```bash
# Compile
gcc -o direct_mtl direct_example.c -lmtl

# Execute (requires root or CAP_NET_RAW)
sudo ./direct_mtl
```

---

### Configuration Options

The plugin exposes MTL parameters as FFmpeg options:

```bash
# Network configuration
-p_port <PCIe_device>      # e.g., "0000:31:00.0"
-p_sip <source_IP>         # e.g., "192.168.1.100"
-p_tx_ip <dest_IP>         # e.g., "239.168.1.1" (multicast)
-udp_port <port>           # e.g., 20000
-payload_type <PT>         # RTP payload type (default: 112)

# Performance tuning
-fb_cnt <count>            # Frame buffer count: 3-8 (default: 3)
-dma_dev <dma_dev>         # DMA device for acceleration
```

---

### Performance Considerations

#### 1. Zero-Copy (Future Enhancement)
Currently uses `memcpy()` to copy FFmpeg packet data to MTL frame buffer. Future versions may support zero-copy using external frame mode.

#### 2. Blocking Mode
Uses `ST20P_TX_FLAG_BLOCK_GET` so `st20p_tx_get_frame()` blocks until buffer is available. This provides:
- ✅ **Simpler code** - No need for polling loops
- ✅ **Better synchronization** - Automatic pacing
- ⚠️ **May block FFmpeg** - If network is slow

#### 3. Frame Buffer Count
- **Lower (3-4):** Less memory, lower latency, more likely to drop frames
- **Higher (6-8):** More buffering, higher latency, more stable transmission

```bash
# Low latency (live production)
-fb_cnt 3

# Stability (busy system)
-fb_cnt 6
```

#### 4. Format Conversion Overhead
Y210LE format requires conversion to RFC4175 before transmission:

```c
if (s->pixel_format == AV_PIX_FMT_Y210LE) {
    st20_y210_to_rfc4175_422be10((uint16_t*)pkt->data,
                                 (struct st20_rfc4175_422_10_pg2_be*)(frame->addr[0]),
                                 s->width, s->height);
}
```

**Recommendation:** Use native formats (YUV422P10LE, RGB24) to avoid conversion overhead.

---

### Supported Pixel Formats

| FFmpeg Format | MTL Input Format | Transport Format | Use Case |
|--------------|------------------|------------------|----------|
| `AV_PIX_FMT_YUV422P10LE` | `ST_FRAME_FMT_YUV422PLANAR10LE` | `ST20_FMT_YUV_422_10BIT` | Broadcast video (recommended) |
| `AV_PIX_FMT_RGB24` | `ST_FRAME_FMT_RGB8` | `ST20_FMT_RGB_8BIT` | Graphics/CGI content |
| `AV_PIX_FMT_Y210LE` | `ST_FRAME_FMT_Y210` | `ST20_FMT_YUV_422_10BIT` | Tiber Broadcast Suite (workaround) |

---

### Debugging Tips

#### Enable Debug Logging

```bash
# FFmpeg debug output
ffmpeg -loglevel debug -re -i input.mp4 \
  -f mtl_st20p \
  -p_port 0000:31:00.0 \
  ...

# MTL library debug (set environment variable)
export MTL_LOG_LEVEL=debug
ffmpeg -re -i input.mp4 ...
```

#### Check Frame Counter

The plugin logs frame counter periodically:

```
[mtl_st20p @ 0x5634...] mtl_st20p_write_close(0), frame_counter 1800
```

This shows 1800 frames were transmitted (30 seconds @ 60fps).

#### Monitor Network Traffic

```bash
# Capture ST2110 packets
sudo tcpdump -i eth0 -nn dst 239.168.1.1 and port 20000

# Check packet rate
sudo iftop -i eth0
```

---

## Installation and Build

### Prerequisites

```bash
# System requirements
- Ubuntu 20.04/22.04 or RHEL 8/9
- GCC 9.0+
- FFmpeg 4.4, 6.1, or 7.0
- Media Transport Library (MTL) installed
- Intel Ethernet controller (E810/XXV710/XL710)

# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential git pkg-config yasm nasm \
    libx264-dev libx265-dev libnuma-dev
```

### Step 1: Build OpenH264 (for H.264 codec support)

```bash
git clone https://github.com/cisco/openh264.git
cd openh264
git checkout openh264v2.4.0
make -j "$(nproc)"
sudo make install
sudo ldconfig
cd ../
```

### Step 2: Build MTL Library

```bash
git clone https://github.com/OpenVisualCloud/Media-Transport-Library.git
cd Media-Transport-Library
./build.sh
sudo make install
cd ../
```

### Step 3: Build FFmpeg with MTL Support

```bash
# Set MTL source path
export MTL_SOURCE=$(pwd)/Media-Transport-Library

# Clone FFmpeg
git clone https://github.com/FFmpeg/FFmpeg.git
cd FFmpeg
git checkout release/7.0

# Apply MTL patches
git am $MTL_SOURCE/ecosystem/ffmpeg_plugin/7.0/*.patch

# Copy MTL plugin source files
cp $MTL_SOURCE/ecosystem/ffmpeg_plugin/mtl_*.c libavdevice/
cp $MTL_SOURCE/ecosystem/ffmpeg_plugin/mtl_*.h libavdevice/

# Configure FFmpeg with MTL support
./configure \
    --enable-shared \
    --disable-static \
    --enable-nonfree \
    --enable-pic \
    --enable-gpl \
    --enable-libopenh264 \
    --enable-encoder=libopenh264 \
    --enable-mtl

# Build
make -j "$(nproc)"
sudo make install
sudo ldconfig
```

**For FFmpeg 4.4 or 6.1:** Replace `7.0` with `4.4` or `6.1` in the commands above.

### Step 4: Verify Installation

```bash
# Check FFmpeg version and MTL devices
ffmpeg -version | grep mtl

# List available devices (should show mtl_st20p, mtl_st22p, etc.)
ffmpeg -devices | grep mtl

# Expected output:
# DE mtl_st20p        Media Transport Library ST2110-20 pipeline
# DE mtl_st20p_le     Media Transport Library ST2110-20 pipeline (little endian)
# DE mtl_st22         Media Transport Library ST2110-22
# DE mtl_st22p        Media Transport Library ST2110-22 pipeline
# DE mtl_st30p        Media Transport Library ST2110-30 pipeline
```

### Alternative: Quick Build Script

```bash
# Use MTL's provided build script
cd Media-Transport-Library/ecosystem/ffmpeg_plugin
./build_ffmpeg_plugin.sh

# With GPU direct support (experimental)
./build_ffmpeg_plugin.sh -g
```

---

## ST20P Uncompressed Video Workflow

### Overview

**ST20P** is the **recommended mode** for uncompressed video transmission. It provides:
- Maximum quality (no compression loss)
- Simple workflow (MTL handles format conversion)
- Supports YUV and RGB formats
- Automatic color space conversion via plugins

### Basic Workflow: MP4/MKV to ST2110-20

#### Step 1: Prepare Input File

```bash
# Example: Download or prepare a test video
# Supported formats: MP4, MKV, AVI, MOV, TS, etc.
# Supported codecs: H.264, H.265, VP9, AV1, ProRes, etc.

# For testing, you can use FFmpeg to create a test pattern:
ffmpeg -f lavfi -i testsrc=duration=10:size=1920x1080:rate=60 \
    -pix_fmt yuv420p test_1080p60.mp4
```

#### Step 2: Transmit with ST20P

**Basic Command:**
```bash
ffmpeg -re -stream_loop -1 \
    -i test_1080p60.mp4 \
    -vf fps=60 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.3 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

**Command Breakdown:**

| Parameter | Purpose | Example Value |
|-----------|---------|---------------|
| `-re` | Read input at native frame rate | - |
| `-stream_loop -1` | Loop video infinitely | -1 = infinite |
| `-i test_1080p60.mp4` | Input file | Any video file |
| `-vf fps=60` | Force output frame rate | 60 fps |
| `-p_port` | PCIe device BDF | 0000:af:01.0 |
| `-p_sip` | Source IP address | 192.168.96.3 |
| `-p_tx_ip` | Destination multicast IP | 239.168.85.20 |
| `-udp_port` | UDP destination port | 20000 |
| `-payload_type` | RTP payload type | 112 |
| `-pix_fmt` | Output pixel format | yuv422p10le |
| `-f mtl_st20p` | Use MTL ST20P muxer | - |
| `-` | Output to stdout | - |

---

#### Step 3: Receive with ST20P

**Receive and Save to File:**
```bash
ffmpeg -p_port 0000:af:01.0 \
    -p_sip 192.168.96.2 \
    -p_rx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -fps 60 \
    -pix_fmt yuv422p10le \
    -video_size 1920x1080 \
    -f mtl_st20p \
    -i "0" \
    -c:v libx264 -preset fast -crf 18 \
    output.mp4 -y
```

**Receive and Display (Real-time Preview):**
```bash
ffmpeg -p_port 0000:af:01.0 \
    -p_sip 192.168.96.2 \
    -p_rx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -fps 60 \
    -pix_fmt yuv422p10le \
    -video_size 1920x1080 \
    -f mtl_st20p \
    -i "0" \
    -f sdl "ST2110 Preview"
```

---

### Supported Pixel Formats for ST20P

| FFmpeg Format | MTL Format | Bits per Pixel | Use Case |
|---------------|-----------|----------------|----------|
| `yuv422p10le` | `ST_FRAME_FMT_YUV422PLANAR10LE` | 20 | **Broadcast standard** |
| `rgb24` | `ST_FRAME_FMT_RGB8` | 24 | RGB workflows |
| `y210le` | `ST_FRAME_FMT_Y210` | 20 | Packed YUV 4:2:2 |

**Recommended:** Use `yuv422p10le` for broadcast-quality workflows.

---

### Complete Example: MKV to ST2110-20

```bash
#!/bin/bash
# transmit_st20p.sh - Transmit MKV file as ST2110-20 stream

INPUT_FILE="movie.mkv"
NIC_PORT="0000:af:01.0"
SOURCE_IP="192.168.96.10"
DEST_IP="239.168.85.20"
UDP_PORT=20000
PAYLOAD_TYPE=112
FPS=60

ffmpeg -re -stream_loop -1 \
    -i "$INPUT_FILE" \
    -vf "fps=$FPS,scale=1920:1080" \
    -p_port "$NIC_PORT" \
    -p_sip "$SOURCE_IP" \
    -p_tx_ip "$DEST_IP" \
    -udp_port $UDP_PORT \
    -payload_type $PAYLOAD_TYPE \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

---

## ST22P Compressed Video Workflow

### Overview

**ST22P** (Pipeline mode with compression) provides:
- **10x less bandwidth** than uncompressed (e.g., 1080p60: 3 Gbps → 0.3 Gbps)
- Automatic encoding/decoding via MTL plugins
- Support for JPEGXS, H.264, H.265
- GPU/FPGA acceleration support

### Workflow: MP4 to ST2110-22 (JPEGXS)

#### Step 1: Transmit with ST22P

```bash
ffmpeg -re -stream_loop -1 \
    -i test_1080p60.mp4 \
    -vf fps=60 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.3 \
    -p_tx_ip 239.168.85.22 \
    -udp_port 30000 \
    -payload_type 112 \
    -st22_codec jpegxs \
    -pix_fmt yuv422p10le \
    -f mtl_st22p -
```

**New Parameter:**
- `-st22_codec jpegxs`: Use JPEGXS compression (default 10:1 ratio)

**Alternative Codecs:**
- `-st22_codec h264`: H.264/AVC compression
- `-st22_codec h265`: H.265/HEVC compression

---

#### Step 2: Receive with ST22P

```bash
ffmpeg -p_port 0000:af:01.0 \
    -p_sip 192.168.96.2 \
    -p_rx_ip 239.168.85.22 \
    -udp_port 30000 \
    -payload_type 112 \
    -st22_codec jpegxs \
    -fps 60 \
    -pix_fmt yuv422p10le \
    -video_size 1920x1080 \
    -f mtl_st22p \
    -i "0" \
    -c:v libx264 -preset fast -crf 18 \
    output_st22p.mp4 -y
```

---

### ST22 with FFmpeg Encoding (Advanced)

For more control over encoder settings, use **ST22 raw codestream mode** with FFmpeg encoders:

#### Transmit (FFmpeg encodes, MTL transports):

```bash
ffmpeg -re -stream_loop -1 \
    -i test_1080p60.mp4 \
    -vf fps=60 \
    -c:v libopenh264 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.3 \
    -p_tx_ip 239.168.85.22 \
    -udp_port 30000 \
    -payload_type 112 \
    -f mtl_st22 -
```

#### Receive (MTL receives, FFmpeg decodes):

```bash
ffmpeg -p_port 0000:af:01.0 \
    -p_sip 192.168.96.2 \
    -p_rx_ip 239.168.85.22 \
    -udp_port 30000 \
    -payload_type 112 \
    -fps 60 \
    -video_size 1920x1080 \
    -st22_codec h264 \
    -f mtl_st22 \
    -i "0" \
    -c:v libx264 -preset fast -crf 18 \
    output_st22.mp4 -y
```

---

## Complete Examples

### Example 1: HD Broadcast Workflow (1080p60 Uncompressed)

**Transmitter:**
```bash
#!/bin/bash
# tx_hd_broadcast.sh

ffmpeg -re -stream_loop -1 \
    -i live_event.mp4 \
    -vf "fps=60,format=yuv422p10le" \
    -p_port 0000:af:01.1 \
    -p_sip 192.168.100.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -f mtl_st20p -
```

**Receiver (Monitor/Record):**
```bash
#!/bin/bash
# rx_hd_monitor.sh

ffmpeg -p_port 0000:af:01.0 \
    -p_sip 192.168.100.20 \
    -p_rx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -fps 60 \
    -pix_fmt yuv422p10le \
    -video_size 1920x1080 \
    -f mtl_st20p \
    -i "0" \
    -map 0:v \
    -c:v libx264 -preset ultrafast -crf 18 \
    -f tee "[f=matroska]record_$(date +%Y%m%d_%H%M%S).mkv|[f=sdl]Monitor"
```

**Result:** 
- Records to timestamped MKV file
- Displays real-time preview window

---

### Example 2: 4K Production (UHD Compressed with JPEGXS)

**Transmitter:**
```bash
#!/bin/bash
# tx_4k_jpegxs.sh

ffmpeg -re -stream_loop -1 \
    -i 4k_source.mp4 \
    -vf "fps=60,scale=3840:2160,format=yuv422p10le" \
    -p_port 0000:af:01.1 \
    -p_sip 192.168.100.10 \
    -p_tx_ip 239.168.85.30 \
    -udp_port 30000 \
    -payload_type 112 \
    -st22_codec jpegxs \
    -f mtl_st22p -
```

**Receiver:**
```bash
#!/bin/bash
# rx_4k_jpegxs.sh

ffmpeg -p_port 0000:af:01.0 \
    -p_sip 192.168.100.20 \
    -p_rx_ip 239.168.85.30 \
    -udp_port 30000 \
    -payload_type 112 \
    -st22_codec jpegxs \
    -fps 60 \
    -pix_fmt yuv422p10le \
    -video_size 3840x2160 \
    -f mtl_st22p \
    -i "0" \
    -c:v libx265 -preset medium -crf 18 \
    4k_output.mp4 -y
```

**Bandwidth:** ~1.2 Gbps (vs 12 Gbps uncompressed)

---

### Example 3: Multi-Stream Setup (2 Cameras)

**Transmitter (Camera 1):**
```bash
ffmpeg -re -stream_loop -1 \
    -i camera1.mp4 \
    -vf fps=60 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

**Transmitter (Camera 2):**
```bash
ffmpeg -re -stream_loop -1 \
    -i camera2.mp4 \
    -vf fps=60 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.21 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

**Receiver (Both Streams):**
```bash
ffmpeg \
    -p_port 0000:af:01.0 -p_sip 192.168.96.20 -p_rx_ip 239.168.85.20 \
    -udp_port 20000 -payload_type 112 -fps 60 -pix_fmt yuv422p10le \
    -video_size 1920x1080 -f mtl_st20p -i "cam1" \
    \
    -p_port 0000:af:01.0 -p_sip 192.168.96.20 -p_rx_ip 239.168.85.21 \
    -udp_port 20000 -payload_type 112 -fps 60 -pix_fmt yuv422p10le \
    -video_size 1920x1080 -f mtl_st20p -i "cam2" \
    \
    -filter_complex "[0:v][1:v]hstack" \
    -c:v libx264 -preset fast \
    -f sdl "Multi-Camera View"
```

**Result:** Side-by-side display of both camera feeds

---

### Example 4: File Conversion (MP4 → ST2110 → MP4)

**Setup:** Transmit from one machine, receive and record on another

**Machine 1 (Transmitter):**
```bash
ffmpeg -re -i input.mp4 \
    -vf fps=60 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.50 \
    -udp_port 50000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

**Machine 2 (Receiver):**
```bash
ffmpeg -p_port 0000:af:01.0 \
    -p_sip 192.168.96.20 \
    -p_rx_ip 239.168.85.50 \
    -udp_port 50000 \
    -payload_type 112 \
    -fps 60 \
    -pix_fmt yuv422p10le \
    -video_size 1920x1080 \
    -f mtl_st20p \
    -i "0" \
    -c:v libx264 -preset slow -crf 18 \
    -c:a aac -b:a 192k \
    output_converted.mp4 -y
```

**Use Case:** Network-based transcoding or processing pipeline

---

## Command Line Parameters

### MTL Device Parameters

| Parameter | Required | Description | Example |
|-----------|----------|-------------|---------|
| `-p_port` | ✅ Yes | PCIe BDF path of network interface | `0000:af:01.0` |
| `-p_sip` | ✅ Yes | Source IP address (local interface) | `192.168.96.10` |
| `-p_tx_ip` | TX only | Destination multicast IP | `239.168.85.20` |
| `-p_rx_ip` | RX only | Multicast IP to listen on | `239.168.85.20` |
| `-udp_port` | ✅ Yes | UDP port number | `20000` |
| `-payload_type` | ✅ Yes | RTP payload type (96-127) | `112` |

**Finding your PCIe BDF:**
```bash
# List all Intel Ethernet devices
lspci | grep -i ethernet

# Example output:
# af:00.0 Ethernet controller: Intel Corporation Ethernet Controller E810-C
# af:01.0 Ethernet controller: Intel Corporation Ethernet Controller E810-C

# Use the BDF: 0000:af:01.0 (domain:bus:device.function)
```

---

### ST20P Specific Parameters

| Parameter | Required | Description | Example |
|-----------|----------|-------------|---------|
| `-fps` | ✅ Yes | Frame rate | `60`, `59.94`, `50`, `30`, `25` |
| `-pix_fmt` | ✅ Yes | Pixel format | `yuv422p10le`, `rgb24` |
| `-video_size` | RX only | Frame resolution | `1920x1080`, `3840x2160` |
| `-fb_cnt` | Optional | Frame buffer count | `3` (default), range 2-8 |

---

### ST22P Specific Parameters

| Parameter | Required | Description | Example |
|-----------|----------|-------------|---------|
| `-st22_codec` | ✅ Yes | Codec type | `jpegxs`, `h264`, `h265` |
| `-fps` | ✅ Yes | Frame rate | `60`, `59.94`, `50`, `30` |
| `-pix_fmt` | ✅ Yes | Pixel format | `yuv422p10le`, `yuv420p` |
| `-video_size` | RX only | Frame resolution | `1920x1080` |

---

### FFmpeg Common Parameters

| Parameter | Description | Example |
|-----------|-------------|---------|
| `-re` | Read input at native frame rate | - |
| `-stream_loop` | Loop input (0=once, -1=infinite) | `-stream_loop -1` |
| `-i` | Input file | `-i video.mp4` |
| `-vf` | Video filters | `-vf "fps=60,scale=1920:1080"` |
| `-c:v` | Video codec | `-c:v libx264` |
| `-f` | Format | `-f mtl_st20p` |
| `-y` | Overwrite output files | - |

---

## Advanced Usage

### 1. Frame Rate Conversion

**Input 30fps → Output 60fps:**
```bash
ffmpeg -re -stream_loop -1 \
    -i input_30fps.mp4 \
    -vf "fps=60,format=yuv422p10le" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -f mtl_st20p -
```

**Frame interpolation (smoother):**
```bash
ffmpeg -re -stream_loop -1 \
    -i input_30fps.mp4 \
    -vf "minterpolate='fps=60:mi_mode=mci'" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

---

### 2. Resolution Scaling

**Upscale 720p → 1080p:**
```bash
ffmpeg -re -i input_720p.mp4 \
    -vf "scale=1920:1080:flags=lanczos,fps=60" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

**Downscale 4K → 1080p:**
```bash
ffmpeg -re -i input_4k.mp4 \
    -vf "scale=1920:1080:flags=lanczos" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

---

### 3. Color Space Conversion

**RGB input → YUV output:**
```bash
ffmpeg -re -i rgb_video.mov \
    -vf "format=yuv422p10le" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -f mtl_st20p -
```

**Transmit as RGB (for graphics/CGI):**
```bash
ffmpeg -re -i rgb_graphics.mov \
    -vf "format=rgb24" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt rgb24 \
    -f mtl_st20p -
```

---

### 4. Adding Overlays/Watermarks

**Add timestamp overlay:**
```bash
ffmpeg -re -stream_loop -1 \
    -i input.mp4 \
    -vf "drawtext=text='%{localtime}':fontsize=30:fontcolor=white:x=10:y=10" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

**Add logo watermark:**
```bash
ffmpeg -re -i input.mp4 \
    -i logo.png \
    -filter_complex "[0:v][1:v]overlay=W-w-10:10" \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

---

### 5. GPU Direct (Experimental)

For ultra-low latency and reduced CPU usage:

```bash
# Build FFmpeg with GPU direct support
./build_ffmpeg_plugin.sh -g

# Transmit with GPU direct
ffmpeg -hwaccel cuda -hwaccel_output_format cuda \
    -re -i input.mp4 \
    -vf "scale_cuda=1920:1080,hwdownload,format=yuv422p10le" \
    -gpu_direct 1 \
    -gpu_driver 0 \
    -gpu_device 0 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -f mtl_st20p -
```

---

## Performance Optimization

### 1. CPU Core Allocation

```bash
# Pin FFmpeg to specific CPU cores (NUMA aware)
taskset -c 0-7 ffmpeg -re -i input.mp4 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

**Best Practice:** Dedicate cores on same NUMA node as NIC:
```bash
# Check NIC NUMA node
cat /sys/class/net/eth0/device/numa_node

# If node 0, use cores from node 0:
numactl --cpunodebind=0 --membind=0 ffmpeg ...
```

---

### 2. Real-Time Priority

```bash
# Run with real-time scheduling (requires root or CAP_SYS_NICE)
chrt -f 80 ffmpeg -re -i input.mp4 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p -
```

---

### 3. Increase Frame Buffer Count

```bash
# For RX: Reduce frame drops on busy systems
ffmpeg -fb_cnt 6 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.20 \
    -p_rx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -fps 60 \
    -pix_fmt yuv422p10le \
    -video_size 1920x1080 \
    -f mtl_st20p \
    -i "0" \
    output.mp4 -y
```

**Default:** 3 buffers  
**Range:** 2-8 buffers  
**More buffers = more latency but more stability**

---

### 4. Reduce Encoding CPU Usage

```bash
# Use faster encoding presets
ffmpeg -re -i input.mp4 \
    -vf fps=60 \
    -p_port 0000:af:01.0 \
    -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 \
    -udp_port 20000 \
    -payload_type 112 \
    -pix_fmt yuv422p10le \
    -f mtl_st20p - \
    2>&1 | grep -i "frame="

# Monitor CPU usage
htop  # or top
```

---

## Troubleshooting

### Issue 1: "mtl_st20p_read_packet timeout"

**Symptoms:**
```
[mtl_st20p @ 0x55634f8b3c80] mtl_st20p_read_packet(0), st20p_rx_get_frame timeout
[in#0/mtl_st20p @ 0x55634f8b3b40] Error during demuxing: Input/output error
```

**Cause:** No video stream detected on the specified IP/port

**Solutions:**
1. Verify transmitter is running
2. Check multicast IP matches (`-p_rx_ip` must match `-p_tx_ip`)
3. Check UDP port matches
4. Verify network connectivity:
   ```bash
   # Check if multicast packets are arriving
   tcpdump -i eth0 host 239.168.85.20 and udp port 20000
   ```
5. Check MTL device initialization:
   ```bash
   # Increase MTL log level
   export MTL_LOG_LEVEL=debug
   ```

---

### Issue 2: Frame Drops / Incomplete Frames

**Symptoms:**
- Receiver shows incomplete frames
- Stats show packet loss

**Solutions:**

1. **Check network bandwidth:**
   ```bash
   # 1080p60 uncompressed requires ~3 Gbps
   # Verify your NIC speed:
   ethtool eth0 | grep Speed
   ```

2. **Increase frame buffer count:**
   ```bash
   -fb_cnt 6  # Default is 3
   ```

3. **Check CPU usage:**
   ```bash
   # If CPU is saturated, reduce workload:
   -c:v libx264 -preset ultrafast  # Faster encoding
   ```

4. **Pin to NUMA node:**
   ```bash
   numactl --cpunodebind=0 --membind=0 ffmpeg ...
   ```

---

### Issue 3: "Unsupported pixel format"

**Symptoms:**
```
[mtl_st20p @ 0x...] unsupported pixel format: xxx
```

**Solution:**

Add explicit format conversion:
```bash
# Before MTL plugin, convert format:
-vf "format=yuv422p10le"

# Supported formats:
# - yuv422p10le (recommended)
# - rgb24
# - y210le
```

---

### Issue 4: High CPU Usage

**Symptoms:**
- FFmpeg process using 100%+ CPU
- Frame drops during encoding

**Solutions:**

1. **Use hardware acceleration:**
   ```bash
   # Intel Quick Sync (if available)
   -hwaccel qsv -c:v h264_qsv
   
   # NVIDIA NVENC
   -hwaccel cuda -c:v h264_nvenc
   ```

2. **Reduce encoding quality:**
   ```bash
   -c:v libx264 -preset ultrafast -crf 23  # Faster, lower quality
   ```

3. **Use ST22P compressed mode:**
   ```bash
   # 10x less bandwidth = less work
   -f mtl_st22p  # Instead of mtl_st20p
   ```

---

### Issue 5: Synchronization Issues

**Symptoms:**
- Audio/video out of sync
- Frame timing drift

**Solutions:**

1. **Ensure PTP is running:**
   ```bash
   # Check PTP status
   sudo ptp4l -i eth0 -m
   
   # Start PTP daemon
   sudo systemctl start ptp4l
   ```

2. **Use `-re` flag for transmit:**
   ```bash
   ffmpeg -re -i input.mp4 ...  # Real-time reading
   ```

3. **Match frame rates exactly:**
   ```bash
   # Get input frame rate
   ffprobe input.mp4 2>&1 | grep fps
   
   # Use exact frame rate
   -vf fps=59.94  # Not fps=60 if source is 59.94
   ```

---

### Debug Mode

**Enable verbose logging:**

```bash
# FFmpeg debug output
export FFREPORT=file=ffmpeg_debug.log:level=48
ffmpeg ...

# MTL debug output
export MTL_LOG_LEVEL=debug
export MTL_LOG_FILE=mtl_debug.log
ffmpeg ...

# Check logs
tail -f ffmpeg_debug.log
tail -f mtl_debug.log
```

---

## Summary and Best Practices

### Quick Decision Guide

| Your Need | Use This Mode | Command |
|-----------|---------------|---------|
| Maximum quality, sufficient bandwidth | ST20P (uncompressed) | `-f mtl_st20p` |
| Bandwidth limited, quality acceptable | ST22P (compressed) | `-f mtl_st22p` |
| Need specific codec control | ST22 with FFmpeg encoder | `-c:v libopenh264 -f mtl_st22` |
| Real-time preview | ST20P + SDL output | `-f sdl` |
| Recording streams | ST20P/ST22P + file output | `-c:v libx264 output.mp4` |

---

### Recommended Workflows

#### Production Broadcast (1080p60):
```bash
# Transmit
ffmpeg -re -i source.mp4 -vf fps=60 \
    -p_port 0000:af:01.0 -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.20 -udp_port 20000 \
    -payload_type 112 -pix_fmt yuv422p10le \
    -f mtl_st20p -

# Receive & Monitor
ffmpeg -p_port 0000:af:01.0 -p_sip 192.168.96.20 \
    -p_rx_ip 239.168.85.20 -udp_port 20000 \
    -payload_type 112 -fps 60 -pix_fmt yuv422p10le \
    -video_size 1920x1080 -f mtl_st20p -i "0" \
    -f sdl "Broadcast Monitor"
```

#### Remote Production (4K60 Compressed):
```bash
# Transmit (JPEGXS 10:1)
ffmpeg -re -i camera_4k.mp4 -vf fps=60 \
    -p_port 0000:af:01.0 -p_sip 192.168.96.10 \
    -p_tx_ip 239.168.85.30 -udp_port 30000 \
    -payload_type 112 -st22_codec jpegxs \
    -pix_fmt yuv422p10le -f mtl_st22p -

# Receive & Record
ffmpeg -p_port 0000:af:01.0 -p_sip 192.168.96.20 \
    -p_rx_ip 239.168.85.30 -udp_port 30000 \
    -payload_type 112 -st22_codec jpegxs \
    -fps 60 -pix_fmt yuv422p10le \
    -video_size 3840x2160 -f mtl_st22p -i "0" \
    -c:v libx265 -crf 18 output_4k.mp4 -y
```

---

### Key Takeaways

✅ **ST20P is simplest for uncompressed workflows** - just specify pixel format  
✅ **ST22P uses 10x less bandwidth** - ideal for bandwidth-constrained networks  
✅ **Always use `-re` flag** when transmitting files to maintain frame rate  
✅ **Match NUMA nodes** - pin FFmpeg and MTL to same NUMA as NIC  
✅ **Use multicast IPs** - 239.x.x.x range for ST2110 streams  
✅ **Enable PTP** - essential for multi-stream synchronization  
✅ **Monitor with tcpdump** - verify packets are flowing  
✅ **Check logs** - use debug mode to troubleshoot issues  

---

**Document Version:** 1.0  
**Last Updated:** February 16, 2026  
**Related Documents:**
- [MTL Pipeline Architecture Guide](mtl_pipeline_architecture_guide.md)
- [MTL Pipeline API Reference](mtl_pipeline_api_reference.md)
- [Intel Ethernet Controllers Guide](intel_ethernet_controllers_video_support.md)
- [FFmpeg Official Documentation](https://ffmpeg.org/documentation.html)
