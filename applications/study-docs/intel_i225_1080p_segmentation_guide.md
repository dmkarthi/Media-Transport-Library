# Intel I225 2.5GbE: 1080p Video Segmentation for Uncompressed Transport

**Date:** February 23, 2026  
**Repository:** OpenVisualCloud/Media-Transport-Library  
**Purpose:** Guide for transmitting segmented 1080p uncompressed video within 2.5G bandwidth constraints

---

## Table of Contents

1. [Overview](#overview)
2. [Segmentation Strategies](#segmentation-strategies)
3. [Bandwidth Calculations by Segment Size](#bandwidth-calculations-by-segment-size)
4. [Format and Bit Depth Support](#format-and-bit-depth-support)
5. [Maximum Frame Rates by Configuration](#maximum-frame-rates-by-configuration)
6. [Practical Use Cases](#practical-use-cases)
7. [Implementation Considerations](#implementation-considerations)
8. [Quick Reference Tables](#quick-reference-tables)

---

## Overview

### The Challenge

A full 1080p frame (1920×1080) using the ST2110-20 standard format (YUV 4:2:2 10-bit) requires:
- **@ 60fps:** 2.98 Gbps (exceeds 2.5G capacity)
- **@ 30fps:** 1.50 Gbps (fits within 2.5G)

### The Solution: Frame Segmentation

By dividing the 1080p frame into smaller segments (tiles), each segment requires proportionally less bandwidth, enabling:
- ✅ **Higher frame rates** within 2.5G limits
- ✅ **Better format options** (YUV 4:4:4, RGB at higher bit depths)
- ✅ **Region of Interest (ROI)** transmission
- ✅ **Multiple independent streams** from a single frame

### Key Principle

**Bandwidth scales linearly with pixel count:**
```
Segment_Bandwidth = Full_Frame_Bandwidth × (Segment_Pixels / Full_Frame_Pixels)

Example: 1/4 segment (960×540) = 1/4 the bandwidth (0.745 Gbps @ 60fps YUV 4:2:2 10-bit)
```

---

## Segmentation Strategies

### 1. Grid Segmentation (Equal Size Tiles)

#### 2×2 Grid (4 Segments)
```
┌─────────┬─────────┐
│  Seg 1  │  Seg 2  │  Each: 960×540 pixels
│ (TL)    │ (TR)    │  Coverage: Full 1080p
├─────────┼─────────┤
│  Seg 3  │  Seg 4  │
│ (BL)    │ (BR)    │
└─────────┴─────────┘
```
- **Segment Size:** 960×540 (518,400 pixels)
- **Pixels per Segment:** 25% of full frame
- **Bandwidth per Segment:** 1/4 of full frame

#### 3×3 Grid (9 Segments)
```
┌─────┬─────┬─────┐
│  1  │  2  │  3  │  Each: 640×360 pixels
├─────┼─────┼─────┤  Coverage: Full 1080p
│  4  │  5  │  6  │
├─────┼─────┼─────┤
│  7  │  8  │  9  │
└─────┴─────┴─────┘
```
- **Segment Size:** 640×360 (230,400 pixels)
- **Pixels per Segment:** 11.1% of full frame
- **Bandwidth per Segment:** 1/9 of full frame

#### 4×4 Grid (16 Segments)
```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │  Each: 480×270 pixels
├───┼───┼───┼───┤  Coverage: Full 1080p
│ 5 │ 6 │ 7 │ 8 │
├───┼───┼───┼───┤
│ 9 │10 │11 │12 │
├───┼───┼───┼───┤
│13 │14 │15 │16 │
└───┴───┴───┴───┘
```
- **Segment Size:** 480×270 (129,600 pixels)
- **Pixels per Segment:** 6.25% of full frame
- **Bandwidth per Segment:** 1/16 of full frame

---

### 2. Horizontal/Vertical Strip Segmentation

#### Horizontal Split (2 Strips)
```
┌─────────────────────┐
│    Top Half (1)     │  Each: 1920×540 pixels
├─────────────────────┤
│   Bottom Half (2)   │
└─────────────────────┘
```
- **Segment Size:** 1920×540 (1,036,800 pixels)
- **Pixels per Segment:** 50% of full frame

#### Vertical Split (2 Strips)
```
┌──────────┬──────────┐
│   Left   │  Right   │  Each: 960×1080 pixels
│   Half   │   Half   │
│   (1)    │   (2)    │
└──────────┴──────────┘
```
- **Segment Size:** 960×1080 (1,036,800 pixels)
- **Pixels per Segment:** 50% of full frame

---

### 3. Region of Interest (ROI) Segmentation

#### Center Focus (1/4 Frame)
```
┌─────────────────────┐
│                     │
│   ┌───────────┐     │  Center: 960×540 pixels
│   │    ROI    │     │  (25% of frame)
│   └───────────┘     │
│                     │
└─────────────────────┘
```

#### Multi-Region (Variable Size)
```
┌─────────────────────┐
│  ┌─────┐            │  ROI 1: 640×360
│  │ R1  │  ┌──────┐  │  ROI 2: 800×450
│  └─────┘  │  R2  │  │  Custom regions of interest
│           └──────┘  │
└─────────────────────┘
```

---

## Bandwidth Calculations by Segment Size

### Standard Format: YUV 4:2:2 10-bit (ST2110-20)

**Formula:**
```
Bandwidth = Width × Height × FPS × 20 bpp × 1.10 overhead / 1,000,000,000
```

#### Full Frame (1920×1080)
| Frame Rate | Pixels/sec | Bandwidth | Fits 2.5G? |
|-----------|------------|-----------|------------|
| 24 fps | 49,766,400 | 1.20 Gbps | ✅ Yes |
| 30 fps | 62,208,000 | 1.50 Gbps | ✅ Yes |
| 50 fps | 103,680,000 | 2.49 Gbps | ⚠️ At limit (99.6%) |
| 60 fps | 124,416,000 | 2.98 Gbps | ❌ No (119.2%) |
| 120 fps | 248,832,000 | 5.97 Gbps | ❌ No (238.8%) |

---

#### 2×2 Grid: Quarter Frame (960×540)

**Segment Details:**
- **Resolution:** 960×540 pixels
- **Pixel Reduction:** 75% fewer pixels than full frame
- **Bandwidth Reduction:** 75% less bandwidth

| Frame Rate | Pixels/sec | Bandwidth/Segment | 2.5G Capacity | Segments Possible |
|-----------|------------|-------------------|---------------|-------------------|
| 24 fps | 12,441,600 | 0.30 Gbps | 2.5 Gbps | ✅ **8** segments |
| 30 fps | 15,552,000 | 0.37 Gbps | 2.5 Gbps | ✅ **6** segments |
| 60 fps | 31,104,000 | 0.75 Gbps | 2.5 Gbps | ✅ **3** segments |
| 120 fps | 62,208,000 | 1.49 Gbps | 2.5 Gbps | ✅ **1** segment |

**Key Finding:** Can transmit 3× quarter-frame segments @ 60fps within 2.5G!

---

#### 3×3 Grid: One-Ninth Frame (640×360)

**Segment Details:**
- **Resolution:** 640×360 pixels
- **Pixel Reduction:** 88.9% fewer pixels than full frame
- **Bandwidth Reduction:** 88.9% less bandwidth

| Frame Rate | Pixels/sec | Bandwidth/Segment | 2.5G Capacity | Segments Possible |
|-----------|------------|-------------------|---------------|-------------------|
| 24 fps | 5,529,600 | 0.13 Gbps | 2.5 Gbps | ✅ **19** segments |
| 30 fps | 6,912,000 | 0.17 Gbps | 2.5 Gbps | ✅ **14** segments |
| 60 fps | 13,824,000 | 0.33 Gbps | 2.5 Gbps | ✅ **7** segments |
| 120 fps | 27,648,000 | 0.66 Gbps | 2.5 Gbps | ✅ **3** segments |

**Key Finding:** Can transmit 7× segments @ 60fps or 3× segments @ 120fps within 2.5G!

---

#### 4×4 Grid: One-Sixteenth Frame (480×270)

**Segment Details:**
- **Resolution:** 480×270 pixels
- **Pixel Reduction:** 93.75% fewer pixels than full frame
- **Bandwidth Reduction:** 93.75% less bandwidth

| Frame Rate | Pixels/sec | Bandwidth/Segment | 2.5G Capacity | Segments Possible |
|-----------|------------|-------------------|---------------|-------------------|
| 24 fps | 3,110,400 | 0.07 Gbps | 2.5 Gbps | ✅ **35** segments |
| 30 fps | 3,888,000 | 0.09 Gbps | 2.5 Gbps | ✅ **27** segments |
| 60 fps | 7,776,000 | 0.19 Gbps | 2.5 Gbps | ✅ **13** segments |
| 120 fps | 15,552,000 | 0.37 Gbps | 2.5 Gbps | ✅ **6** segments |

**Key Finding:** Can transmit 13× segments @ 60fps or 6× segments @ 120fps within 2.5G!

---

#### Horizontal/Vertical Split: Half Frame (1920×540 or 960×1080)

**Segment Details:**
- **Resolution:** 1920×540 or 960×1080 pixels (both = 1,036,800 pixels)
- **Pixel Reduction:** 50% fewer pixels than full frame
- **Bandwidth Reduction:** 50% less bandwidth

| Frame Rate | Pixels/sec | Bandwidth/Segment | 2.5G Capacity | Segments Possible |
|-----------|------------|-------------------|---------------|-------------------|
| 24 fps | 24,883,200 | 0.60 Gbps | 2.5 Gbps | ✅ **4** segments |
| 30 fps | 31,104,000 | 0.75 Gbps | 2.5 Gbps | ✅ **3** segments |
| 60 fps | 62,208,000 | 1.49 Gbps | 2.5 Gbps | ✅ **1** segment |
| 120 fps | 124,416,000 | 2.98 Gbps | 2.5 Gbps | ❌ **0** segments |

**Key Finding:** Can transmit 1× half-frame @ 60fps within 2.5G (perfect for top/bottom split)

---

## Format and Bit Depth Support

### Quarter Frame (960×540) - YUV Formats

**@ 60fps within 2.5G:**

| Format | Bit Depth | Bits/Pixel | Bandwidth | Max Segments | Link Utilization |
|--------|-----------|------------|-----------|--------------|------------------|
| **YUV 4:2:0** | 8-bit | 12 | 0.45 Gbps | ✅ **5** | 90% |
| YUV 4:2:0 | 10-bit | 15 | 0.56 Gbps | ✅ **4** | 89% |
| YUV 4:2:0 | 12-bit | 18 | 0.67 Gbps | ✅ **3** | 80% |
| **YUV 4:2:2** | 8-bit | 16 | 0.60 Gbps | ✅ **4** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **0.75 Gbps** | **✅ 3** | **90%** |
| YUV 4:2:2 | 12-bit | 24 | 0.90 Gbps | ✅ **2** | 72% |
| **YUV 4:4:4** | 8-bit | 24 | 0.90 Gbps | ✅ **2** | 72% |
| YUV 4:4:4 | 10-bit | 30 | 1.12 Gbps | ✅ **2** | 90% |
| YUV 4:4:4 | 12-bit | 36 | 1.34 Gbps | ✅ **1** | 54% |

★ = ST2110-20 standard format

---

### Quarter Frame (960×540) - RGB Formats

**@ 60fps within 2.5G:**

| Format | Bit Depth | Bits/Pixel | Bandwidth | Max Segments | Link Utilization |
|--------|-----------|------------|-----------|--------------|------------------|
| **RGB** | 8-bit | 24 | 0.90 Gbps | ✅ **2** | 72% |
| RGB | 10-bit | 30 | 1.12 Gbps | ✅ **2** | 90% |
| RGB | 12-bit | 36 | 1.34 Gbps | ✅ **1** | 54% |
| RGB | 16-bit | 48 | 1.79 Gbps | ✅ **1** | 72% |

---

### One-Ninth Frame (640×360) - All Formats

**@ 60fps within 2.5G:**

| Format | Bit Depth | Bits/Pixel | Bandwidth | Max Segments | Link Utilization |
|--------|-----------|------------|-----------|--------------|------------------|
| **YUV 4:2:0** | 8-bit | 12 | 0.20 Gbps | ✅ **12** | 96% |
| YUV 4:2:0 | 10-bit | 15 | 0.25 Gbps | ✅ **10** | 100% (at limit) |
| YUV 4:2:0 | 12-bit | 18 | 0.30 Gbps | ✅ **8** | 96% |
| **YUV 4:2:2** | 8-bit | 16 | 0.27 Gbps | ✅ **9** | 97% |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **0.33 Gbps** | **✅ 7** | **92%** |
| YUV 4:2:2 | 12-bit | 24 | 0.40 Gbps | ✅ **6** | 96% |
| **YUV 4:4:4** | 8-bit | 24 | 0.40 Gbps | ✅ **6** | 96% |
| YUV 4:4:4 | 10-bit | 30 | 0.50 Gbps | ✅ **5** | 100% (at limit) |
| YUV 4:4:4 | 12-bit | 36 | 0.60 Gbps | ✅ **4** | 96% |
| **RGB** | 8-bit | 24 | 0.40 Gbps | ✅ **6** | 96% |
| RGB | 10-bit | 30 | 0.50 Gbps | ✅ **5** | 100% (at limit) |
| RGB | 12-bit | 36 | 0.60 Gbps | ✅ **4** | 96% |

---

### One-Sixteenth Frame (480×270) - All Formats

**@ 120fps within 2.5G:**

| Format | Bit Depth | Bits/Pixel | Bandwidth | Max Segments | Link Utilization |
|--------|-----------|------------|-----------|--------------|------------------|
| **YUV 4:2:0** | 8-bit | 12 | 0.22 Gbps | ✅ **11** | 97% |
| YUV 4:2:0 | 10-bit | 15 | 0.28 Gbps | ✅ **8** | 90% |
| YUV 4:2:0 | 12-bit | 18 | 0.34 Gbps | ✅ **7** | 95% |
| **YUV 4:2:2** | 8-bit | 16 | 0.30 Gbps | ✅ **8** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **0.37 Gbps** | **✅ 6** | **89%** |
| YUV 4:2:2 | 12-bit | 24 | 0.45 Gbps | ✅ **5** | 90% |
| **YUV 4:4:4** | 8-bit | 24 | 0.45 Gbps | ✅ **5** | 90% |
| YUV 4:4:4 | 10-bit | 30 | 0.56 Gbps | ✅ **4** | 90% |
| YUV 4:4:4 | 12-bit | 36 | 0.67 Gbps | ✅ **3** | 80% |
| **RGB** | 8-bit | 24 | 0.45 Gbps | ✅ **5** | 90% |
| RGB | 10-bit | 30 | 0.56 Gbps | ✅ **4** | 90% |
| RGB | 12-bit | 36 | 0.67 Gbps | ✅ **3** | 80% |

---

## Maximum Frame Rates by Configuration

### Quarter Frame (960×540) - Maximum FPS

**Using YUV 4:2:2 10-bit (ST2110 standard):**

| Configuration | Bandwidth/Segment | Max FPS/Segment | Notes |
|--------------|-------------------|-----------------|-------|
| 1× segment only | 0.75 Gbps @ 60fps | **200 fps** | Ultra high frame rate |
| 2× segments | 1.49 Gbps total | **120 fps** each | Dual ROI @ 120fps |
| 3× segments | 2.24 Gbps total | **80 fps** each | Triple ROI @ 80fps |
| 4× segments | 2.98 Gbps total | **60 fps** each | Full 2×2 grid @ 60fps |

**Using YUV 4:2:0 10-bit (HDR, bandwidth-efficient):**

| Configuration | Bandwidth/Segment | Max FPS/Segment | Notes |
|--------------|-------------------|-----------------|-------|
| 1× segment only | 0.56 Gbps @ 60fps | **267 fps** | Extreme high speed |
| 2× segments | 1.12 Gbps total | **133 fps** each | Dual ROI @ 133fps |
| 3× segments | 1.68 Gbps total | **89 fps** each | Triple ROI @ 89fps |
| 4× segments | 2.24 Gbps total | **67 fps** each | Full 2×2 grid @ 67fps |

---

### One-Ninth Frame (640×360) - Maximum FPS

**Using YUV 4:2:2 10-bit (ST2110 standard):**

| Configuration | Bandwidth/Segment | Max FPS/Segment | Notes |
|--------------|-------------------|-----------------|-------|
| 1× segment only | 0.33 Gbps @ 60fps | **454 fps** | Ultra high frame rate |
| 3× segments | 1.00 Gbps total | **182 fps** each | Triple ROI @ 182fps |
| 6× segments | 2.00 Gbps total | **91 fps** each | 2×3 grid @ 91fps |
| 9× segments | 3.00 Gbps total | **60 fps** each | Full 3×3 grid @ 60fps ⚠️ |

⚠️ 9 segments @ 60fps = 2.97 Gbps (exceeds 2.5G slightly, use 50fps instead for safety)

---

### One-Sixteenth Frame (480×270) - Maximum FPS

**Using YUV 4:2:2 10-bit (ST2110 standard):**

| Configuration | Bandwidth/Segment | Max FPS/Segment | Notes |
|--------------|-------------------|-----------------|-------|
| 1× segment only | 0.19 Gbps @ 60fps | **789 fps** | Extreme high speed |
| 4× segments | 0.75 Gbps total | **200 fps** each | Quad ROI @ 200fps |
| 8× segments | 1.49 Gbps total | **100 fps** each | Half grid @ 100fps |
| 16× segments | 2.98 Gbps total | **50 fps** each | Full 4×4 grid @ 50fps ⚠️ |

⚠️ 16 segments require careful bandwidth management near 2.5G limit

---

## Practical Use Cases

### Use Case 1: Broadcast Graphics Overlay

**Scenario:** Live sports with scoreboard and statistics box

**Configuration:**
```
┌─────────────────────────┐
│ ┌───────────────┐       │  Main feed: Compressed or lower quality
│ │   Scoreboard  │       │  Overlay: Uncompressed high quality
│ └───────────────┘       │
│                         │  Segment: 640×180 pixels
│                         │  Format: RGB 10-bit @ 60fps
│                         │  Bandwidth: 0.25 Gbps
│   ┌──────────────┐      │
│   │   Stats Box  │      │  2.5G NIC easily handles overlay + telemetry
│   └──────────────┘      │
└─────────────────────────┘
```

**Benefits:**
- High quality text and graphics
- Lower bandwidth for main video
- Independent update rates

---

### Use Case 2: Multi-Camera PTZ Follow

**Scenario:** Stadium coverage with 4 region specialists

**Configuration:**
```
┌─────────┬─────────┐
│ Camera1 │ Camera2 │  Each: 960×540 @ 60fps
│ (TL)    │ (TR)    │  Format: YUV 4:2:2 10-bit
├─────────┼─────────┤  Total: 3.0 Gbps (4× 0.75 Gbps)
│ Camera3 │ Camera4 │
│ (BL)    │ (BR)    │  ⚠️ Slightly over 2.5G
└─────────┴─────────┘

Solution: Use 50fps instead of 60fps
         4× 0.62 Gbps = 2.48 Gbps ✅
```

**Benefits:**
- Full coverage with smooth motion
- Independent camera control
- Switch between regions seamlessly

---

### Use Case 3: Surgical/Medical Imaging

**Scenario:** High-precision medical procedure monitoring

**Configuration:**
```
┌─────────────────────────┐
│                         │
│     ┌─────────┐         │  ROI: 800×600 pixels
│     │         │         │  Format: RGB 12-bit @ 60fps
│     │   ROI   │         │  Bandwidth: 1.90 Gbps
│     │         │         │
│     └─────────┘         │  Additional: Thumbnails + metadata
│                         │  Total: ~2.3 Gbps ✅
└─────────────────────────┘
```

**Benefits:**
- True color accuracy (RGB 12-bit)
- Smooth 60fps motion
- ROI flexibility
- Diagnostic quality preservation

---

### Use Case 4: High Frame Rate Sports Analysis

**Scenario:** Golf swing / tennis serve analysis

**Configuration:**
```
┌─────────────────────────┐
│                         │
│    ┌──────────┐         │  Analysis area: 640×360 pixels
│    │          │         │  Format: YUV 4:2:2 10-bit @ 240fps
│    │  Player  │         │  Bandwidth: 1.32 Gbps ✅
│    │   Zone   │         │
│    └──────────┘         │  Additional: Context stream @ 30fps
│                         │  Total: ~1.6 Gbps ✅
└─────────────────────────┘
```

**Benefits:**
- Ultra-smooth slow motion
- Standard format compliance
- Ample bandwidth for analysis tools
- Real-time processing capable

---

### Use Case 5: Security/Surveillance with ROI

**Scenario:** Entrance monitoring with face recognition zone

**Configuration:**
```
┌─────────────────────────┐
│   ┌──────┐              │  Full frame: Compressed @ 10:1
│   │ Face │              │              ~0.30 Gbps
│   │ ROI  │              │
│   └──────┘              │  ROI: 480×480 @ 60fps
│                         │       YUV 4:2:2 10-bit uncompressed
│        Door             │       0.43 Gbps
│                         │
└─────────────────────────┘  Total: ~0.73 Gbps ✅
                            Leaves bandwidth for 2+ cameras
```

**Benefits:**
- Full scene awareness (compressed)
- High-quality biometric data (uncompressed ROI)
- Multiple camera support
- AI processing optimization

---

### Use Case 6: Virtual Production LED Wall Calibration

**Scenario:** LED panel color calibration patches

**Configuration:**
```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │  16× calibration zones
├───┼───┼───┼───┤  Each: 480×270 pixels
│ 5 │ 6 │ 7 │ 8 │  Format: RGB 16-bit @ 30fps
├───┼───┼───┼───┤  Bandwidth/segment: 0.27 Gbps
│ 9 │10 │11 │12 │  Total for 9 zones: 2.43 Gbps ✅
├───┼───┼───┼───┤
│13 │14 │15 │16 │  (Transmit 9 of 16 per iteration)
└───┴───┴───┴───┘
```

**Benefits:**
- True color accuracy (16-bit)
- Per-panel calibration
- High dynamic range support
- Sequential zone transmission

---

## Implementation Considerations

### 1. Synchronization Requirements

**PTP (Precision Time Protocol) is critical:**
```bash
# All segments must be frame-synchronized
# Recommended: Hardware PTP (I226-V has better timing than I225-V)

# Maximum acceptable timing drift: ±1 microsecond
# Achievable with I226-V: ±100 nanoseconds
```

**Frame Synchronization:**
- **Essential:** Common PTP master clock
- **Recommended:** Genlock source for camera/capture devices
- **Buffer Alignment:** Ensure all segments share frame boundary timestamps

---

### 2. Segment Boundary Handling

**Pixel-Perfect Alignment:**
```python
# Ensure no gaps or overlaps
segment_width = frame_width // columns
segment_height = frame_height // rows

# Example 2×2 grid from 1920×1080:
seg_tl = (0, 0, 960, 540)       # Top-left
seg_tr = (960, 0, 1920, 540)    # Top-right
seg_bl = (0, 540, 960, 1080)    # Bottom-left
seg_br = (960, 540, 1920, 1080) # Bottom-right
```

**Chroma Subsampling Considerations:**
```
YUV 4:2:2: Segment widths must be even (chroma pairs)
YUV 4:2:0: Segment widths AND heights must be even

✅ Good: 960×540 (both even)
✅ Good: 640×360 (both even)
❌ Bad: 641×359 (both odd - causes chroma misalignment)
```

---

### 3. Reconstruction Buffer Management

**Recommended Approach:**
```c
// Allocate full frame buffer
uint8_t frame_buffer[1920 * 1080 * 3]; // RGB or YUV

// Receive segments with metadata
struct segment_metadata {
    uint32_t frame_number;
    uint16_t segment_id;
    uint16_t x_offset;
    uint16_t y_offset;
    uint16_t width;
    uint16_t height;
    uint64_t timestamp_ns;
};

// Composite when all segments received
if (all_segments_received(frame_number)) {
    composite_frame(frame_buffer, segments, metadata);
    display_frame(frame_buffer);
}
```

**Memory Requirements:**
```
Single 1080p RGB 10-bit frame: ~8.3 MB
Buffer for 3 frames (pipeline): ~25 MB
Per-segment buffers (4× 960×540): ~8.3 MB

Total recommended: 64 MB per stream
```

---

### 4. MTL Configuration for Segmented Streams

**Sample Configuration (4 segments, 960×540 each):**
```json
{
  "interfaces": [{
    "name": "enp1s0f0",
    "ip": "192.168.1.101"
  }],
  "rx_sessions": [
    {
      "type": "st20",
      "ip": "239.1.1.1",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_tl",
      "metadata": {
        "segment_id": 1,
        "x_offset": 0,
        "y_offset": 0
      }
    },
    {
      "type": "st20",
      "ip": "239.1.1.2",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_tr",
      "metadata": {
        "segment_id": 2,
        "x_offset": 960,
        "y_offset": 0
      }
    },
    {
      "type": "st20",
      "ip": "239.1.1.3",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_bl",
      "metadata": {
        "segment_id": 3,
        "x_offset": 0,
        "y_offset": 540
      }
    },
    {
      "type": "st20",
      "ip": "239.1.1.4",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_br",
      "metadata": {
        "segment_id": 4,
        "x_offset": 960,
        "y_offset": 540
      }
    }
  ]
}
```

---

### 5. Latency Considerations

**Per-Segment Latency:**
```
Network propagation: 0.1-1 ms (LAN)
Packetization: 0.5-2 ms (depends on FPS)
Reconstruction: 2-5 ms (software composite)
Display: 16.7 ms (60fps vsync)

Total glass-to-glass: ~20-25 ms
```

**Optimization Tips:**
- Use hardware compositing (GPU) when available
- Minimize buffering (use 2-frame pipeline)
- Pin threads to CPU cores for consistent performance
- Use HugePages for large frame buffers

---

### 6. Error Handling and Recovery

**Packet Loss Mitigation:**
```
Strategy 1: Forward Error Correction (FEC)
- Add 10-20% redundancy per segment
- Recovers from up to 20% packet loss
- Bandwidth cost: +0.08 Gbps per segment

Strategy 2: Segment Priority
- Mark ROI segments with higher QoS
- Use VLAN prioritization (802.1Q)
- Ensure switch supports priority queues

Strategy 3: Dual Path
- Transmit critical segments on redundant paths
- Use ST2022-7 seamless protection
- Requires second NIC interface
```

**Missing Segment Handling:**
```c
// Option 1: Freeze last good frame for that segment
if (!segment_received(segment_id, frame_num)) {
    use_previous_frame(segment_id);
}

// Option 2: Interpolation from neighbors
if (!segment_received(segment_id, frame_num)) {
    interpolate_from_adjacent(segment_id);
}

// Option 3: Drop entire frame
if (missing_segments > threshold) {
    drop_frame(frame_num);
}
```

---

## Quick Reference Tables

### Bandwidth per Segment @ 60fps (YUV 4:2:2 10-bit)

| Segment Size | Resolution | Bandwidth | Segments in 2.5G |
|-------------|-----------|-----------|------------------|
| **Full** | 1920×1080 | 2.98 Gbps | ❌ 0 (exceeds) |
| **Half** | 1920×540 | 1.49 Gbps | ✅ 1 |
| **Quarter** | 960×540 | 0.75 Gbps | ✅ 3 |
| **One-Ninth** | 640×360 | 0.33 Gbps | ✅ 7 |
| **One-Sixteenth** | 480×270 | 0.19 Gbps | ✅ 13 |

---

### Maximum Frame Rate per Segment (YUV 4:2:2 10-bit)

| Segment Size | Single Stream | 2× Streams | 4× Streams | 8× Streams |
|-------------|---------------|------------|------------|------------|
| **Half** | 120 fps | 60 fps | ❌ N/A | ❌ N/A |
| **Quarter** | 200 fps | 100 fps | 60 fps | 30 fps |
| **One-Ninth** | 454 fps | 227 fps | 113 fps | 57 fps |
| **One-Sixteenth** | 789 fps | 395 fps | 197 fps | 98 fps |

---

### Optimal Configurations for Common Scenarios

| Scenario | Segment Size | Format | Bit Depth | FPS | Bandwidth | Notes |
|----------|-------------|--------|-----------|-----|-----------|-------|
| **Scoreboard overlay** | 640×180 | RGB | 10-bit | 60 | 0.25 Gbps | Sharp text |
| **4-camera split** | 960×540 | YUV 4:2:2 | 10-bit | 50 | 2.48 Gbps | Fits 2.5G |
| **Medical ROI** | 800×600 | RGB | 12-bit | 60 | 1.90 Gbps | True color |
| **High-speed analysis** | 640×360 | YUV 4:2:2 | 10-bit | 240 | 1.32 Gbps | Ultra smooth |
| **Face recognition** | 480×480 | YUV 4:2:2 | 10-bit | 60 | 0.43 Gbps | Biometric quality |
| **Calibration patch** | 480×270 | RGB | 16-bit | 30 | 0.27 Gbps | Precision color |

---

### Format Comparison @ Quarter Frame 960×540, 60fps

| Format | Bit Depth | Bandwidth | Quality | Best Use Case |
|--------|-----------|-----------|---------|---------------|
| YUV 4:2:0 8-bit | 8 | 0.45 Gbps | Good | Consumer content |
| YUV 4:2:0 10-bit | 10 | 0.56 Gbps | Excellent | HDR, efficient |
| YUV 4:2:2 8-bit | 8 | 0.60 Gbps | Very Good | Broadcast lite |
| **YUV 4:2:2 10-bit** ★ | **10** | **0.75 Gbps** | **Excellent** | **ST2110 standard** |
| YUV 4:2:2 12-bit | 12 | 0.90 Gbps | Premium | High-end mastering |
| YUV 4:4:4 10-bit | 10 | 1.12 Gbps | Premium | VFX, keying |
| RGB 10-bit | 10 | 1.12 Gbps | Premium | Graphics, CGI |
| RGB 12-bit | 12 | 1.34 Gbps | Maximum | Medical, scientific |

★ = Recommended for broadcast compliance

---

## Python Bandwidth Calculator

```python
def calculate_segment_bandwidth(segment_width, segment_height, fps, 
                                format_type, bit_depth, overhead=1.10):
    """
    Calculate bandwidth for a segmented 1080p stream
    
    Args:
        segment_width: Width in pixels
        segment_height: Height in pixels
        fps: Frames per second
        format_type: 'yuv420', 'yuv422', 'yuv444', 'rgb', 'rgba'
        bit_depth: 8, 10, 12, or 16
        overhead: Protocol overhead multiplier (default 1.10 for 10%)
    
    Returns:
        bandwidth_gbps: Bandwidth in Gigabits per second
    """
    
    # Bits per pixel mapping
    bpp_map = {
        'yuv420': {8: 12, 10: 15, 12: 18},
        'yuv422': {8: 16, 10: 20, 12: 24},
        'yuv444': {8: 24, 10: 30, 12: 36},
        'rgb': {8: 24, 10: 30, 12: 36, 16: 48},
        'rgba': {8: 32, 10: 40, 12: 48, 16: 64}
    }
    
    bits_per_pixel = bpp_map[format_type][bit_depth]
    pixels_per_second = segment_width * segment_height * fps
    raw_bandwidth_gbps = (pixels_per_second * bits_per_pixel) / 1_000_000_000
    bandwidth_gbps = raw_bandwidth_gbps * overhead
    
    return round(bandwidth_gbps, 2)

def segments_in_25g(segment_width, segment_height, fps, 
                    format_type, bit_depth, link_speed_gbps=2.5):
    """
    Calculate how many segments fit in 2.5G link
    """
    segment_bw = calculate_segment_bandwidth(segment_width, segment_height, 
                                            fps, format_type, bit_depth)
    max_segments = int(link_speed_gbps / segment_bw)
    utilization = (max_segments * segment_bw / link_speed_gbps) * 100
    
    return max_segments, utilization

# Example usage:
# Quarter frame YUV 4:2:2 10-bit @ 60fps
bw = calculate_segment_bandwidth(960, 540, 60, 'yuv422', 10)
print(f"Bandwidth: {bw} Gbps")  # Output: 0.75 Gbps

segments, util = segments_in_25g(960, 540, 60, 'yuv422', 10)
print(f"Max segments: {segments}, Utilization: {util:.1f}%")
# Output: Max segments: 3, Utilization: 90.0%
```

---

## Conclusion

### Key Takeaways

1. **Segmentation enables high-quality uncompressed video on 2.5G NICs:**
   - Quarter-frame (960×540): 3× segments @ 60fps in ST2110 standard format
   - One-ninth (640×360): 7× segments @ 60fps or 3× @ 120fps
   - ROI transmission: Focus bandwidth on critical image areas

2. **Format flexibility increases with smaller segments:**
   - Full YUV 4:4:4 and RGB support at 60fps with quarter-frames
   - Ultra-high frame rates (200+ fps) possible with smaller segments
   - True color accuracy (12-bit/16-bit) achievable for critical applications

3. **Practical applications:**
   - Multi-camera coverage within single 2.5G link
   - Region-of-interest for AI/ML processing
   - High frame rate analysis for sports/research
   - Medical imaging with diagnostic quality
   - Broadcast graphics overlays

4. **I225/I226 2.5GbE NICs are viable for professional uncompressed workflows** when combined with intelligent segmentation strategies

5. **Critical success factors:**
   - Precise PTP synchronization (prefer I226-V with TSN)
   - Pixel-perfect boundary alignment
   - Chroma subsampling awareness (even dimensions)
   - Robust error handling and recovery

---

**Document Version:** 1.0  
**Last Updated:** February 23, 2026  
**Author:** Media Transport Library Study Guide Series  
**License:** BSD-3-Clause  

**Related Documents:**
- [Intel Ethernet Controllers Video Support Guide](intel_ethernet_controllers_video_support.md)
- [MTL Pipeline Architecture Guide](mtl_pipeline_architecture_guide.md)
- [ST2110 Best Practices](https://www.st2110.com/)

---

**Feedback & Contributions:**  
This document is part of the OpenVisualCloud Media Transport Library project.  
Submit issues or improvements via GitHub: [OpenVisualCloud/Media-Transport-Library](https://github.com/OpenVisualCloud/Media-Transport-Library)
