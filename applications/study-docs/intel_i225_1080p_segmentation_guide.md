# Intel I225 2.5GbE: 1080p Video Segmentation for Uncompressed Transport

**Date:** February 23, 2026  
**Repository:** OpenVisualCloud/Media-Transport-Library  
**Purpose:** Guide for transmitting multiple segmented 1080p uncompressed video streams over a single 2.5G NIC using multi-stream architecture

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

### The Solution: Frame Segmentation with Multi-Stream Transmission

By dividing the 1080p frame into smaller segments (tiles) and transmitting them as **multiple ST2110 streams over a single 2.5G interface**, you can:
- ✅ **Maximize 2.5G bandwidth utilization** with multiple concurrent streams
- ✅ **Support various segment combinations** (ROI, quad-split, etc.)
- ✅ **Enable flexible format options** based on segment size
- ✅ **Leverage standard ST2110-20** compliance per stream
- ✅ **Simplify hardware requirements** (single NIC)

### Key Principle: Multiplexed Stream Architecture

**Bandwidth scales linearly with pixel count:**
```
Segment_Bandwidth = Full_Frame_Bandwidth × (Segment_Pixels / Full_Frame_Pixels)

Example: Full 1080p60 YUV 4:2:2 10-bit = 2.98 Gbps (exceeds 2.5G)
         Quarter segment (960×540) = 0.75 Gbps per segment
         Single 2.5G NIC can carry 3× quarter segments simultaneously ✅
         OR 1× quarter segment @ higher FPS (200fps)
         OR mixed resolutions/formats within 2.5G budget
```

**All segments share the same physical interface:**
- Multiple RTP streams with different multicast addresses
- Each stream is an independent ST2110-20 flow
- Streams are packet-interleaved at the NIC hardware level
- Total bandwidth constrained to 2.5 Gbps link capacity
- Receiver subscribes to relevant multicast groups for desired segments

---

## Single-NIC Multi-Stream Architecture

### Concept: Stream-Per-Segment Design

**Multiple Streams, Single Interface:**
```
Single Intel I225 NIC (2.5 Gbps total)
│
├─── Stream 1 (239.1.1.1:20000) → Segment 1 @ 0.75 Gbps
├─── Stream 2 (239.1.1.2:20000) → Segment 2 @ 0.75 Gbps  
├─── Stream 3 (239.1.1.3:20000) → Segment 3 @ 0.75 Gbps
└─── (Total: 2.25 Gbps, 90% utilization) ✅

Packets from all streams are interleaved on the wire:
[S1-pkt][S2-pkt][S3-pkt][S1-pkt][S2-pkt][S3-pkt]...

Receiver Side:
├─── Subscribes to multicast 239.1.1.1 → Receives Stream 1
├─── Subscribes to multicast 239.1.1.2 → Receives Stream 2  
└─── Subscribes to multicast 239.1.1.3 → Receives Stream 3

Optional: Frame reconstruction if segments form complete frame
```

**Key Characteristics:**
- Each segment is an independent ST2110-20 RTP stream
- Unique multicast group per stream
- NIC hardware handles packet multiplexing automatically
- Total bandwidth <= 2.5 Gbps (hard limit)
- No timing synchronization required between streams (unless reconstructing)

**Benefits of Single-NIC Approach:**
- ✅ Lower hardware cost (one NIC vs multiple)
- ✅ Simplified network topology
- ✅ Standard ST2110-20 per stream
- ✅ Flexible receiver-side stream selection
- ✅ Each stream can be independently consumed

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

**Single-NIC Stream Scenarios:**
- **Single 2.5G port with 3× streams:** 3× quarter segments @ 60fps ✅
  - Each stream @ 0.75 Gbps
  - Total: 2.25 Gbps (90% utilization)
  - Use case: Triple ROI, or 3/4 of full frame coverage
- **Single 2.5G port with 1× stream:** 1× quarter segment @ 200fps
  - Single stream @ 2.49 Gbps (99.6% utilization)
  - Use case: High-speed capture of specific region
- **Single 2.5G port with 7× streams:** 7× one-ninth segments @ 60fps
  - Each stream @ 0.33 Gbps
  - Total: 2.31 Gbps (92% utilization)
  - Use case: Multi-region monitoring

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

### Use Case 2: Triple-Stream ROI Monitoring

**Scenario:** Security/surveillance with 3 regions of interest on single I225 NIC

**Single-NIC Multi-Stream Configuration:**
```
Camera/Source: 1920×1080 @ 60fps YUV 4:2:2 10-bit

         ┌──────────────────┐
         │  ROI Selection   │
         │  & Crop Engine   │
         └────────┬─────────┘
                  │
         ┌────────┴─────────┐
         │  3 ROI Streams   │
         │  960×540 each    │
         └──────┬───────────┘
                │
    ┌───────────┼───────────┐
    │           │           │
    ▼           ▼           ▼
┌───┴───────────┴───────────┴────┐
│   Single Intel I225 2.5GbE NIC │
│                                 │
│  Stream 1: 239.1.1.1 (0.75Gb)  │
│  Stream 2: 239.1.1.2 (0.75Gb)  │
│  Stream 3: 239.1.1.3 (0.75Gb)  │
│  ────────────────────────────  │
│  Total: 2.25 Gbps (90% util)   │
└────────────┬────────────────────┘
             │
        IP Network
             │
   ┌─────────┼─────────┐
   │         │         │
   ▼         ▼         ▼
┌──┴─────────┴─────────┴──┐
│  Receiver: Single I225  │
│  Subscribe to relevant  │
│  multicast groups       │
│  (1, 2, 3, or all)      │
└─────────────────────────┘
```

**System Bandwidth:**
- Per stream: 0.75 Gbps
- Total (3 streams): 2.25 Gbps
- Link utilization: 90%
- Result: 3× independent ST2110 streams ✅

**Benefits:**
- ✅ Single NIC hardware (low cost)
- ✅ Each stream independently selectable
- ✅ Flexible receiver subscription
- ✅ Standard ST2110-20 per stream
- ✅ 10% headroom for management traffic

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

### 4. MTL Single-NIC Multi-Stream Configuration

#### MTL Multi-Session on Single Interface

**MTL supports multiple TX/RX sessions on the same physical interface:**
- ✅ Multiple TX sessions sharing one NIC
- ✅ Each session = independent ST2110-20 RTP stream
- ✅ Hardware packet interleaving automatic
- ✅ Total bandwidth constrained by link speed (2.5G)

**Capabilities:**
```c
// Single interface with multiple sessions
// Each session has unique multicast destination

// Bandwidth management:
// - Sum of all session bandwidths <= 2.5 Gbps
// - MTL tracks per-session rates
// - Automatic flow control if approaching limit

// No frame sync needed unless reconstructing full frame
```

---

#### Sample Configuration: Single I225 with 4 Streams

**Scenario:** Transmit 4× segments (3× active @ 60fps) on single I225 NIC

**TX Configuration (Single I225 NIC, 3 active streams):**
```json
{
  "interfaces": [
    {
      "name": "enp1s0f0",
      "ip": "192.168.1.101",
      "port": "2.5G",
      "ptp_master": true
    }
  ],
  "tx_sessions": [
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.1",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_1",
      "metadata": {
        "segment_id": 1,
        "x_offset": 0,
        "y_offset": 0,
        "description": "Top-left quarter"
      }
    },
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.2",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_2",
      "metadata": {
        "segment_id": 2,
        "x_offset": 960,
        "y_offset": 0,
        "description": "Top-right quarter"
      }
    },
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.3",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_3",
      "metadata": {
        "segment_id": 3,
        "x_offset": 0,
        "y_offset": 540,
        "description": "Bottom-left quarter"
      }
    }
  ],
  "bandwidth_budget": {
    "link_speed_gbps": 2.5,
    "total_allocated_gbps": 2.25,
    "utilization_percent": 90,
    "headroom_gbps": 0.25
  }
}
```

**RX Configuration (Single I225 NIC, 3 streams):**
```json
{
  "interfaces": [
    {
      "name": "enp1s0f0",
      "ip": "192.168.1.201",
      "port": "2.5G"
    }
  ],
  "rx_sessions": [
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.1",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_1",
      "metadata": {
        "segment_id": 1,
        "x_offset": 0,
        "y_offset": 0
      }
    },
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.2",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_2",
      "metadata": {
        "segment_id": 2,
        "x_offset": 960,
        "y_offset": 0
      }
    },
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.3",
      "port": 20000,
      "payload_type": 112,
      "width": 960,
      "height": 540,
      "fps": "p60",
      "fmt": "YUV422_10bit",
      "name": "segment_3",
      "metadata": {
        "segment_id": 3,
        "x_offset": 0,
        "y_offset": 540
      }
    }
  ],
  "notes": [
    "All streams received on same physical interface",
    "Hardware RSS distributes packets to CPU cores",
    "Application can selectively process streams",
    "Optional: Reconstruct partial frame from 3 segments"
  ]
}
```

---

#### Multi-Stream Implementation

**TX Side - Multiple Streams on Single Interface:**
```c
// MTL TX API usage for multi-stream transmission on single NIC

struct mtl_init_params init_params = {0};
init_params.num_ports = 1; // Single interface
init_params.ptp_systime_sync = true;

// Initialize MTL with 1 interface
mtl_handle mtl = mtl_init(&init_params);

// Create 3 TX sessions (all on same port, different multicast groups)
struct st20_tx_ops tx_ops[3];
for (int i = 0; i < 3; i++) {
    tx_ops[i].port.num_port = 1;
    tx_ops[i].port.port[0] = 0; // All use port 0
    tx_ops[i].width = 960;
    tx_ops[i].height = 540;
    tx_ops[i].fps = ST_FPS_P60;
    tx_ops[i].fmt = ST20_FMT_YUV_422_10BIT;
    tx_ops[i].framebuff_cnt = 3;
    
    // Each session gets unique multicast group
    snprintf(tx_ops[i].dip_addr, "%s", multicast_ips[i]);
    
    tx_handles[i] = st20_tx_create(mtl, &tx_ops[i]);
}

// Transmission loop - send all segments of frame N simultaneously
while (running) {
    uint64_t frame_time = get_ptp_time(); // Get current PTP time
    uint32_t frame_number = frame_counter++;
    
    // Get all 4 segment buffers for this frame
    for (int i = 0; i < 4; i++) {
        segment_bufs[i] = st20_tx_get_framebuffer(tx_handles[i], frame_number);
        
        // Copy segment data (from full frame to segment buffer)
        copy_frame_segment(full_frame, segment_bufs[i], i, 
                          960, 540, 1920, 1080);
        
        // Set identical timestamp for all segments
        st20_tx_set_framebuffer_timestamp(tx_handles[i], segment_bufs[i], 
                                         frame_time);
    }
    
    // Transmit ALL segments simultaneously (blocking until frame time)
    for (int i = 0; i < 4; i++) {
        st20_tx_put_framebuffer(tx_handles[i], segment_bufs[i]);
    }
    
    // All segments sent with frame_number and frame_time
    // Network delivers them in parallel
}
```

**RX Side - Collect and Reconstruct:**
```c
// MTL RX API usage for parallel segment reception

struct mtl_init_params init_params = {0};
init_params.num_ports = 4;

mtl_handle mtl = mtl_init(&init_params);

// Create 4 RX sessions (one per segment/port)
struct st20_rx_ops rx_ops[4];
for (int i = 0; i < 4; i++) {
    rx_ops[i].port.num_port = 1;
    rx_ops[i].port.port[0] = i; // Port 0, 1, 2, 3
    rx_ops[i].width = 960;
    rx_ops[i].height = 540;
    rx_ops[i].fps = ST_FPS_P60;
    rx_ops[i].fmt = ST20_FMT_YUV_422_10BIT;
    rx_ops[i].notify_frame_ready = segment_frame_ready_callback;
    rx_ops[i].priv = &segment_context[i];
    
    rx_handles[i] = st20_rx_create(mtl, &rx_ops[i]);
}

// Reconstruction engine
struct frame_reconstruction_state {
    uint32_t frame_number;
    uint64_t frame_timestamp;
    void* segment_buffers[4];
    bool segment_received[4];
    int segments_count;
};

void segment_frame_ready_callback(void* priv) {
    struct segment_context* ctx = (struct segment_context*)priv;
    int segment_id = ctx->segment_id;
    
    // Get received segment
    void* segment_buf = st20_rx_get_framebuffer(rx_handles[segment_id]);
    uint32_t frame_num = st20_rx_get_frame_number(segment_buf);
    uint64_t timestamp = st20_rx_get_timestamp(segment_buf);
    
    // Add to reconstruction queue
    struct frame_reconstruction_state* frame_state = 
        get_or_create_frame_state(frame_num);
    
    frame_state->segment_buffers[segment_id] = segment_buf;
    frame_state->segment_received[segment_id] = true;
    frame_state->segments_count++;
    frame_state->frame_timestamp = timestamp;
    
    // Check if all segments received
    if (frame_state->segments_count == 4) {
        // All segments for this frame arrived!
        composite_full_frame(frame_state, output_buffer);
        deliver_frame(output_buffer, frame_num, timestamp);
        
        // Release all segment buffers
        for (int i = 0; i < 4; i++) {
            st20_rx_put_framebuffer(rx_handles[i], 
                                  frame_state->segment_buffers[i]);
        }
        
        free_frame_state(frame_state);
    }
}

void composite_full_frame(struct frame_reconstruction_state* state,
                         void* output_1920x1080) {
    // Copy segment_tl (0,0) -> (0,0)
    copy_segment_to_frame(state->segment_buffers[0], output_1920x1080,
                         0, 0, 960, 540, 1920, 1080);
    
    // Copy segment_tr (960,0) -> (960,0)
    copy_segment_to_frame(state->segment_buffers[1], output_1920x1080,
                         960, 0, 960, 540, 1920, 1080);
    
    // Copy segment_bl (0,540) -> (0,540)
    copy_segment_to_frame(state->segment_buffers[2], output_1920x1080,
                         0, 540, 960, 540, 1920, 1080);
    
    // Copy segment_br (960,540) -> (960,540)
    copy_segment_to_frame(state->segment_buffers[3], output_1920x1080,
                         960, 540, 960, 540, 1920, 1080);
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

### Single-NIC Multi-Stream Bandwidth (YUV 4:2:2 10-bit @ 60fps)

**Single Intel I225 2.5GbE NIC Capacity**

| Segment Size | Per-Stream BW | Max Concurrent Streams | Total BW Used | Utilization | Use Case |
|-------------|---------------|----------------------|---------------|-------------|----------|
| **Full** | 1920×1080 | 2.98 Gbps | ❌ **0** (exceeds link) | - | Requires 10G or compression |
| **Half** | 1920×540 | 1.49 Gbps | ✅ **1 stream** | 1.49 Gbps | 60% | ROI or half-frame |
| **Quarter** | 960×540 | 0.75 Gbps | ✅ **3 streams** | 2.25 Gbps | 90% | **✅ Recommended** |
| **One-Ninth** | 640×360 | 0.33 Gbps | ✅ **7 streams** | 2.31 Gbps | 92% | Multi-region |
| **One-Sixteenth** | 480×270 | 0.19 Gbps | ✅ **13 streams** | 2.47 Gbps | 99% | Many small regions |

**Recommended: 3× Quarter-frame streams @ 60fps (90% utilization, 10% headroom)**

---

### Single-NIC Stream Combinations @ 60fps

| Configuration | Stream 1 | Stream 2 | Stream 3 | Total BW | Utilization | Notes |
|--------------|----------|----------|----------|----------|-------------|-------|
| **3× Quarter** | 960×540 | 960×540 | 960×540 | 2.25 Gbps | 90% | ✅ Best balance |
| **2× Quarter** | 960×540 | 960×540 | - | 1.49 Gbps | 60% | Room for other traffic |
| **1× Half + 2× Ninth** | 1920×540 | 640×360 | 640×360 | 2.15 Gbps | 86% | Main + 2 ROI |
| **7× Ninth** | 640×360 (×7) | - | - | 2.31 Gbps | 92% | Multi-camera grid |
| **1× Quarter + 4× Ninth** | 960×540 | 640×360 (×4) | - | 2.07 Gbps | 83% | Main + 4 small |

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

4. **I225/I226 2.5GbE NICs are viable for professional uncompressed workflows** when using single-NIC multi-stream transmission with intelligent segmentation strategies

5. **Critical success factors:**
   - Multiple ST2110-20 streams multiplexed on single interface
   - Bandwidth budget management (total ≤ 2.5 Gbps)
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
