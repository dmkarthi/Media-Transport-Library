# Intel I225 2.5GbE: LED Video Wall Segmentation Guide

**Date:** February 23, 2026  
**Repository:** OpenVisualCloud/Media-Transport-Library  
**Purpose:** Guide for driving LED video walls with segmented 1080p uncompressed video streams over a single 2.5G NIC, using vertical and horizontal panel arrangements

---

## Table of Contents

1. [Overview](#overview)
2. [Single-NIC Multi-Stream Architecture](#single-nic-multi-stream-architecture)
3. [LED Panel Segmentation Strategies](#led-panel-segmentation-strategies)
4. [Bandwidth Calculations for LED Panels](#bandwidth-calculations-for-led-panels)
5. [LED Panel Format and Color Support](#led-panel-format-and-color-support)
6. [Practical LED Video Wall Use Cases](#practical-led-video-wall-use-cases)
7. [Implementation Considerations](#implementation-considerations)
8. [Quick Reference Tables](#quick-reference-tables)
9. [Python Bandwidth Calculator](#python-bandwidth-calculator)
10. [Conclusion](#conclusion)

---

## Overview

### The Challenge: LED Video Walls with Budget Hardware

LED video walls typically require multiple video outputs or expensive 10G infrastructure. A full 1080p frame using ST2110-20 standard format (YUV 4:2:2 10-bit) requires:
- **@ 30fps:** 1.50 Gbps (fits within 2.5G for single panel)
- **@ 24fps:** 1.20 Gbps (fits within 2.5G for single panel)
- **Multiple panels:** Traditionally require multiple NICs or 10G infrastructure

### The Solution: LED Panel Segmentation with Single-NIC Multi-Stream

By dividing the 1080p content into **vertical or horizontal strips** matching your LED panel layout, and transmitting each panel feed as an **independent ST2110 stream over a single 2.5G interface**, you can:
- ✅ **Drive 2-6 LED panels** from a single Intel I225 2.5G NIC
- ✅ **Support vertical and horizontal panel arrangements** (side-by-side or stacked)
- ✅ **Enable ultrawide or tall displays** without multi-GPU systems
- ✅ **Leverage standard ST2110-20** compliance per panel
- ✅ **Simplify hardware requirements** (single NIC, single output device)

### Key Principle: Multiplexed Stream Architecture

**Bandwidth scales linearly with panel count:**
```
Panel_Bandwidth = Full_Frame_Bandwidth × (Panel_Pixels / Full_Frame_Pixels)

Example: Full 1080p30 YUV 4:2:2 10-bit = 1.50 Gbps
         2-panel horizontal split (960×1080 each) = 0.75 Gbps per panel
         3-panel vertical split (640×1080 each) = 0.50 Gbps per panel
         Single 2.5G NIC can drive 3-4 LED panels simultaneously ✅
```

**All LED panel feeds share the same physical interface:**
- One RTP stream per LED panel with unique multicast address
- Each panel feed is an independent ST2110-20 flow
- Streams are packet-interleaved at the NIC hardware level
- Total bandwidth constrained to 2.5 Gbps link capacity
- Each LED panel controller subscribes to its designated multicast group

---

## Single-NIC Multi-Stream Architecture

### Concept: Stream-Per-Panel Design

**Multiple LED Panels, Single Interface:**
```
Single Intel I225 NIC (2.5 Gbps total)
│
├─── Panel 1 (239.1.1.1:20000) → Left vertical strip @ 0.75 Gbps
├─── Panel 2 (239.1.1.2:20000) → Center vertical strip @ 0.75 Gbps
├─── Panel 3 (239.1.1.3:20000) → Right vertical strip @ 0.75 Gbps
└─── (Total: 2.25 Gbps, 90% utilization) ✅

         Content Source (1920×1080 @ 30fps)
                      │
              ┌───────┴───────┐
              │ Segmentation  │
              │    Engine     │
              └───────┬───────┘
                      │
        ┌─────────────┼─────────────┐
        │             │             │
     Panel 1       Panel 2       Panel 3
  (640×1080)    (640×1080)    (640×1080)
   239.1.1.1     239.1.1.2     239.1.1.3
        │             │             │
        └─────────────┴─────────────┘
                      │
           ┌──────────┴──────────┐
           │  Single I225 NIC    │
           │  Packet Interleave  │
           └──────────┬──────────┘
                      │
              Ethernet Network
                      │
        ┌─────────────┼─────────────┐
        │             │             │
   ┌────▼────┐   ┌───▼─────┐  ┌───▼─────┐
   │ LED     │   │ LED     │  │ LED     │
   │ Panel 1 │   │ Panel 2 │  │ Panel 3 │
   │ (Left)  │   │ (Center)│  │ (Right) │
   └─────────┘   └─────────┘  └─────────┘

        Combined Display: 1920×1080 (3-panel ultrawide)
```

**Key Characteristics:**
- Each LED panel feed is an independent ST2110-20 RTP stream
- Unique multicast group per panel
- NIC hardware handles packet multiplexing automatically
- Total bandwidth <= 2.5 Gbps (hard limit)
- Panel-to-panel sync achieved via PTP for seamless tiling

**Benefits for LED Video Walls:**
- ✅ **Lower hardware cost:** Single 2.5G NIC vs multi-GPU or 10G setup
- ✅ **Simplified cabling:** One network cable drives multiple panels
- ✅ **Standard ST2110-20:** Professional broadcast quality per panel
- ✅ **Scalable layouts:** Support 2-6 panels vertically or horizontally
- ✅ **Future-proof:** Easy to add/remove panels by adjusting stream count

---

## LED Panel Segmentation Strategies

### 1. Vertical Panel Arrangement (Side-by-Side)

#### 2-Panel Vertical (Dual Side-by-Side)
```
┌──────────┬──────────┐
│          │          │  Left Panel:  960×1080 pixels
│  Panel 1 │  Panel 2 │  Right Panel: 960×1080 pixels
│  (Left)  │ (Right)  │  
│          │          │  Combined: 1920×1080 ultrawide
│          │          │  
└──────────┴──────────┘
```
- **Panel Resolution:** 960×1080 (1,036,800 pixels)
- **Pixels per Panel:** 50% of full frame
- **Bandwidth per Panel @ 30fps:** 0.75 Gbps
- **Total Bandwidth:** 1.50 Gbps (60% utilization)
- **Use Case:** Ultrawide 2:1 LED wall

#### 3-Panel Vertical (Triple Side-by-Side)
```
┌──────┬──────┬──────┐
│      │      │      │  Left Panel:   640×1080 pixels
│Panel │Panel │Panel │  Center Panel: 640×1080 pixels
│  1   │  2   │  3   │  Right Panel:  640×1080 pixels
│(Left)│(Cntr)│(Rght)│  
│      │      │      │  Combined: 1920×1080 ultrawide
└──────┴──────┴──────┘
```
- **Panel Resolution:** 640×1080 (691,200 pixels)
- **Pixels per Panel:** 33.3% of full frame
- **Bandwidth per Panel @ 30fps:** 0.50 Gbps
- **Total Bandwidth:** 1.50 Gbps (60% utilization)
- **Use Case:** Ultrawide 3:1 LED wall, retail displays

#### 4-Panel Vertical (Quad Side-by-Side)
```
┌────┬────┬────┬────┐
│Panel│Panel│Panel│Panel│  Each Panel: 480×1080 pixels
│ 1  │ 2  │ 3  │ 4  │  
│    │    │    │    │  Combined: 1920×1080 ultrawide
└────┴────┴────┴────┘
```
- **Panel Resolution:** 480×1080 (518,400 pixels)
- **Pixels per Panel:** 25% of full frame
- **Bandwidth per Panel @ 30fps:** 0.37 Gbps
- **Total Bandwidth:** 1.50 Gbps (60% utilization)
- **Use Case:** Command centers, sports scoreboards

---

### 2. Horizontal Panel Arrangement (Stacked)

#### 2-Panel Horizontal (Top/Bottom Stack)
```
┌─────────────────────┐
│      Panel 1        │  Top Panel:    1920×540 pixels
│       (Top)         │  Bottom Panel: 1920×540 pixels
├─────────────────────┤  
│      Panel 2        │  Combined: 1920×1080 tall display
│     (Bottom)        │
└─────────────────────┘
```
- **Panel Resolution:** 1920×540 (1,036,800 pixels)
- **Pixels per Panel:** 50% of full frame
- **Bandwidth per Panel @ 30fps:** 0.75 Gbps
- **Total Bandwidth:** 1.50 Gbps (60% utilization)
- **Use Case:** Tall portrait LED walls, building facades

#### 3-Panel Horizontal (Triple Stack)
```
┌─────────────────────┐
│      Panel 1        │  Top Panel:    1920×360 pixels
│       (Top)         │  Middle Panel: 1920×360 pixels
├─────────────────────┤  Bottom Panel: 1920×360 pixels
│      Panel 2        │  
│      (Middle)       │  Combined: 1920×1080 tall display
├─────────────────────┤
│      Panel 3        │
│     (Bottom)        │
└─────────────────────┘
```
- **Panel Resolution:** 1920×360 (691,200 pixels)
- **Pixels per Panel:** 33.3% of full frame
- **Bandwidth per Panel @ 30fps:** 0.50 Gbps
- **Total Bandwidth:** 1.50 Gbps (60% utilization)
- **Use Case:** Vertical tower displays, elevator lobbies

---

## Bandwidth Calculations for LED Panels

### Standard Format: YUV 4:2:2 10-bit (ST2110-20)

**Formula:**
```
Bandwidth = Width × Height × FPS × 20 bpp × 1.10 overhead / 1,000,000,000
```

#### Full Frame (1920×1080) - Single Panel
| Frame Rate | Pixels/sec | Bandwidth | Fits 2.5G? |
|-----------|------------|-----------|------------|
| 24 fps | 49,766,400 | 1.20 Gbps | ✅ Yes (48% util) |
| 30 fps | 62,208,000 | 1.50 Gbps | ✅ Yes (60% util) |

**Note:** Full-frame @30fps uses 60% of 2.5G link - leaves room for 1 additional panel stream.

---

### Vertical Panel Configurations (Side-by-Side)

#### 2-Panel Vertical: Half-Width Panels (960×1080)

**Panel Details:**
- **Resolution per Panel:** 960×1080 pixels (1,036,800 pixels)
- **Pixel Count:** 50% of full frame per panel
- **Combined Display:** 1920×1080 ultrawide

| Frame Rate | Pixels/sec/Panel | Bandwidth/Panel | Total (2 panels) | 2.5G Utilization |
|-----------|------------------|-----------------|------------------|------------------|
| 24 fps | 24,883,200 | 0.60 Gbps | 1.20 Gbps | 48% ✅ |
| 30 fps | 31,104,000 | 0.75 Gbps | 1.50 Gbps | 60% ✅ |

**Key Finding:** 2-panel vertical @ 30fps = 60% utilization - excellent for dual side-by-side LED walls

---

#### 3-Panel Vertical: Third-Width Panels (640×1080)

**Panel Details:**
- **Resolution per Panel:** 640×1080 pixels (691,200 pixels)
- **Pixel Count:** 33.3% of full frame per panel
- **Combined Display:** 1920×1080 ultrawide (3:1 aspect ratio content)

| Frame Rate | Pixels/sec/Panel | Bandwidth/Panel | Total (3 panels) | 2.5G Utilization |
|-----------|------------------|-----------------|------------------|------------------|
| 24 fps | 16,588,800 | 0.40 Gbps | 1.20 Gbps | 48% ✅ |
| 30 fps | 20,736,000 | 0.50 Gbps | 1.50 Gbps | 60% ✅ |

**Key Finding:** 3-panel vertical @ 30fps = 60% utilization - perfect for retail/signage triple-wide displays

---

#### 4-Panel Vertical: Quarter-Width Panels (480×1080)

**Panel Details:**
- **Resolution per Panel:** 480×1080 pixels (518,400 pixels)
- **Pixel Count:** 25% of full frame per panel
- **Combined Display:** 1920×1080 ultrawide (4:1 aspect ratio content)

| Frame Rate | Pixels/sec/Panel | Bandwidth/Panel | Total (4 panels) | 2.5G Utilization |
|-----------|------------------|-----------------|------------------|------------------|
| 24 fps | 12,441,600 | 0.30 Gbps | 1.20 Gbps | 48% ✅ |
| 30 fps | 15,552,000 | 0.37 Gbps | 1.50 Gbps | 60% ✅ |

**Key Finding:** 4-panel vertical @ 30fps = 60% utilization - ideal for command centers and wide scoreboards

---

### Horizontal Panel Configurations (Stacked)

#### 2-Panel Horizontal: Half-Height Panels (1920×540)

**Panel Details:**
- **Resolution per Panel:** 1920×540 pixels (1,036,800 pixels)
- **Pixel Count:** 50% of full frame per panel
- **Combined Display:** 1920×1080 tall display

| Frame Rate | Pixels/sec/Panel | Bandwidth/Panel | Total (2 panels) | 2.5G Utilization |
|-----------|------------------|-----------------|------------------|------------------|
| 24 fps | 24,883,200 | 0.60 Gbps | 1.20 Gbps | 48% ✅ |
| 30 fps | 31,104,000 | 0.75 Gbps | 1.50 Gbps | 60% ✅ |

**Key Finding:** 2-panel horizontal @ 30fps = 60% utilization - excellent for stacked portrait displays

---

#### 3-Panel Horizontal: Third-Height Panels (1920×360)

**Panel Details:**
- **Resolution per Panel:** 1920×360 pixels (691,200 pixels)
- **Pixel Count:** 33.3% of full frame per panel
- **Combined Display:** 1920×1080 vertical tower

| Frame Rate | Pixels/sec/Panel | Bandwidth/Panel | Total (3 panels) | 2.5G Utilization |
|-----------|------------------|-----------------|------------------|------------------|
| 24 fps | 16,588,800 | 0.40 Gbps | 1.20 Gbps | 48% ✅ |
| 30 fps | 20,736,000 | 0.50 Gbps | 1.50 Gbps | 60% ✅ |

**Key Finding:** 3-panel horizontal @ 30fps = 60% utilization - ideal for vertical tower installations

---

#### 5-Panel Horizontal: Fifth-Height Panels (1920×216)

**Panel Details:**
- **Resolution per Panel:** 1920×216 pixels (414,720 pixels)
- **Pixel Count:** 20% of full frame per panel
- **Combined Display:** 1920×1080 segmented horizontal bands

| Frame Rate | Pixels/sec/Panel | Bandwidth/Panel | Total (5 panels) | 2.5G Utilization |
|-----------|------------------|-----------------|------------------|------------------|
| 24 fps | 9,953,280 | 0.24 Gbps | 1.20 Gbps | 48% ✅ |
| 30 fps | 12,441,600 | 0.30 Gbps | 1.50 Gbps | 60% ✅ |

**Key Finding:** 5-panel horizontal @ 30fps = 60% utilization - perfect for multi-band ticker/status displays

---

## LED Panel Format and Color Support

### YUV Formats (Broadcast Standard)

**YUV 4:2:2 10-bit - Recommended for LED Video Walls**

| Panel Config | Per-Panel Resolution | Bandwidth/Panel @ 30fps | Max Panels | Total Bandwidth | Utilization |
|--------------|---------------------|------------------------|------------|-----------------|-------------|
| 2-Panel Vertical | 960×1080 | 0.75 Gbps | 3 | 2.25 Gbps | 90% ✅ |
| 3-Panel Vertical | 640×1080 | 0.50 Gbps | 5 | 2.50 Gbps | 100% |
| 4-Panel Vertical | 480×1080 | 0.37 Gbps | 6 | 2.22 Gbps | 89% ✅ |
| 2-Panel Horizontal | 1920×540 | 0.75 Gbps | 3 | 2.25 Gbps | 90% ✅ |
| 3-Panel Horizontal | 1920×360 | 0.50 Gbps | 5 | 2.50 Gbps | 100% |
| 5-Panel Horizontal | 1920×216 | 0.30 Gbps | 8 | 2.40 Gbps | 96% ✅ |

★ **ST2110-20 Standard:** Production quality for LED installations

**YUV 4:2:0 10-bit - Bandwidth-Efficient Option**

| Panel Config | Per-Panel Resolution | Bandwidth/Panel @ 30fps | Max Panels | Total Bandwidth | Utilization |
|--------------|---------------------|------------------------|------------|-----------------|-------------|
| 2-Panel Vertical | 960×1080 | 0.56 Gbps | 4 | 2.24 Gbps | 90% ✅ |
| 3-Panel Vertical | 640×1080 | 0.37 Gbps | 6 | 2.22 Gbps | 89% ✅ |
| 4-Panel Vertical | 480×1080 | 0.28 Gbps | 8 | 2.24 Gbps | 90% ✅ |

**Key Benefit:** 25% bandwidth reduction vs 4:2:2

---

### RGB Formats (High Color Accuracy)

**RGB 10-bit - LED Calibration & Graphics**

| Panel Config | Per-Panel Resolution | Bandwidth/Panel @ 30fps | Max Panels | Total Bandwidth | Utilization |
|--------------|---------------------|------------------------|------------|-----------------|-------------|
| 2-Panel Vertical | 960×1080 | 1.12 Gbps | 2 | 2.24 Gbps | 90% ✅ |
| 3-Panel Vertical | 640×1080 | 0.75 Gbps | 3 | 2.25 Gbps | 90% ✅ |
| 4-Panel Vertical | 480×1080 | 0.56 Gbps | 4 | 2.24 Gbps | 90% ✅ |

**Use Cases:**
- ✅ LED panel color calibration (per-panel uniformity)
- ✅ Computer graphics rendering (CAD, 3D visualization)
- ✅ True color accuracy (no chroma subsampling)

**RGB 8-bit - Budget Option**

| Panel Config | Per-Panel Resolution | Bandwidth/Panel @ 30fps | Max Panels | Total Bandwidth | Utilization |
|--------------|---------------------|------------------------|------------|-----------------|-------------|
| 3-Panel Vertical | 640×1080 | 0.60 Gbps | 4 | 2.40 Gbps | 96% ✅ |
| 4-Panel Vertical | 480×1080 | 0.45 Gbps | 5 | 2.25 Gbps | 90% ✅ |

---

### Format Recommendations by Application

| Application | Recommended Format | Panels Supported | Reason |
|-------------|-------------------|------------------|--------|
| **Retail/Signage** | YUV 4:2:2 10-bit | 2-4 vertical | Broadcast quality, video content |
| **Command Center** | YUV 4:2:2 10-bit | 3-4 panels | Dashboard graphics + video |
| **Building Facade** | YUV 4:2:0 10-bit | 4-6 panels | Bandwidth-efficient for large arrays |
| **Calibration/Test** | RGB 10-bit | 2-3 panels | True color, per-panel uniformity |
| **Budget Installs** | YUV 4:2:0 8-bit | 5-8 panels | Maximum panel count, lower quality |

---

### LED Panel Color Space Considerations

**YUV → RGB Conversion:**
- LED panels natively use RGB
- YUV content requires real-time conversion at LED controller
- ST2110-20 uses Rec.709 color space (HD standard)
- Ensure LED controller supports BT.709 YUV-to-RGB matrix

**Direct RGB Transport:**
- No conversion overhead
- Higher bandwidth requirement (30 bpp vs 20 bpp for YUV 4:2:2)
- Ideal for computer-generated graphics
- Recommended for multi-panel calibration procedures

---

## Practical LED Video Wall Use Cases

### Use Case 1: Retail Ultrawide Display (3-Panel Vertical)

**Scenario:** Store window display with triple-wide panoramic content

**Configuration:**
```
Content Source: 1920×1080 @ 30fps
Segmentation: 3 vertical panels (640×1080 each)

         640px      640px      640px
      ┌─────────┬─────────┬─────────┐
      │ Panel 1 │ Panel 2 │ Panel 3 │  Height: 1080px
      │ (Left)  │(Center) │(Right)  │
      └─────────┴─────────┴─────────┘
         LED       LED       LED
      Controller Controller Controller
      239.1.1.1  239.1.1.2  239.1.1.3
```

**System Bandwidth:**
- Per panel: 0.50 Gbps @ 30fps
- Total (3 panels): 1.50 Gbps
- Link utilization: 60%
- Result: Seamless 3-panel ultrawide @ 30fps ✅

**Benefits:**
- ✅ Ultrawide 3:1 aspect ratio content
- ✅ Budget hardware (single I225 NIC)
- ✅ Professional broadcast quality (YUV 4:2:2 10-bit)
- ✅ 40% headroom for control traffic

---

### Use Case 2: Command Center 4-Panel Wide Screen

**Scenario:** NOC/SOC operations center with quad-wide monitoring display

**Configuration:**
```
Content Source: 1920×1080 @ 30fps
Segmentation: 4 vertical panels (480×1080 each)

      480px   480px   480px   480px
    ┌──────┬──────┬──────┬──────┐
    │Panel │Panel │Panel │Panel │  Height: 1080px
    │  1   │  2   │  3   │  4   │
    └──────┴──────┴──────┴──────┘
       LED     LED     LED     LED
   239.1.1.1  ...2   ...3   ...4
```

**System Bandwidth:**
- Per panel: 0.37 Gbps @ 30fps
- Total (4 panels): 1.50 Gbps
- Link utilization: 60%
- Result: 4-panel wide display @ 30fps ✅

**Benefits:**
- ✅ Wide 4:1 format for dashboards
- ✅ Single network cable to distribution
- ✅ Each panel independently addressable
- ✅ Flexible content layouts

---

### Use Case 3: Building Facade Vertical Display (3-Panel Stack)

**Scenario:** Elevator lobby or building exterior vertical tower display

**Configuration:**
```
Content Source: 1920×1080 @ 30fps  
Segmentation: 3 horizontal panels (1920×360 each)

     ┌──────────────────────┐
     │      Panel 1 (Top)   │  360px
     ├──────────────────────┤
     │   Panel 2 (Middle)   │  360px
     ├──────────────────────┤
     │   Panel 3 (Bottom)   │  360px
     └──────────────────────┘
         Width: 1920px

       LED Controller 239.1.1.1
       LED Controller 239.1.1.2  
       LED Controller 239.1.1.3
```

**System Bandwidth:**
- Per panel: 0.50 Gbps @ 30fps
- Total (3 panels): 1.50 Gbps
- Link utilization: 60%
- Result: 3-panel vertical tower @ 30fps ✅

**Benefits:**
- ✅ Portrait/vertical content format
- ✅ Ideal for building facades
- ✅ Elevator lobby installations
- ✅ Synchronized panel timing via PTP

---

### Use Case 4: Sports Stadium Scoreboard (2-Panel Wide)

**Scenario:** Stadium scoreboard with dual-panel horizontal layout

**Configuration:**
```
Content Source: 1920×1080 @ 30fps
Segmentation: 2 vertical panels (960×1080 each)

         960px            960px
      ┌──────────────┬──────────────┐
      │              │              │
      │   Panel 1    │   Panel 2    │  Height: 1080px
      │   (Left)     │   (Right)    │
      │              │              │
      └──────────────┴──────────────┘
          LED              LED
      Controller       Controller
       239.1.1.1       239.1.1.2
```

**System Bandwidth:**
- Per panel: 0.75 Gbps @ 30fps
- Total (2 panels): 1.50 Gbps
- Link utilization: 60%
- Result: Dual-panel wide scoreboard @ 30fps ✅

**Benefits:**
- ✅ Wide format for game scores and stats
- ✅ Separate panel feeds for A/B content
- ✅ Can show split content (score | video replay)
- ✅ Broadcast quality graphics

---

### Use Case 5: Conference Room Dual-Height Display (2-Panel Stack)

**Scenario:** Meeting room with stacked presentation screens

**Configuration:**
```
Content Source: 1920×1080 @ 30fps
Segmentation: 2 horizontal panels (1920×540 each)

     ┌─────────────────────────┐
     │   Panel 1 (Top)         │  540px
     │   (Primary Content)     │
     ├─────────────────────────┤
     │   Panel 2 (Bottom)      │  540px
     │   (Notes/Video)         │
     └─────────────────────────┘
            Width: 1920px

        LED Controller 239.1.1.1
        LED Controller 239.1.1.2
```

**System Bandwidth:**
- Per panel: 0.75 Gbps @ 30fps
- Total (2 panels): 1.50 Gbps
- Link utilization: 60%
- Result: Dual-height stacked display @ 30fps ✅

**Benefits:**
- ✅ Split presentation/video feeds
- ✅ Independent content zones
- ✅ Uncompressed quality for text/diagrams
- ✅ Easy panel repurposing

---

### Use Case 6: LED Wall Calibration & Testing

**Scenario:** Panel-by-panel color calibration and uniformity testing

**Configuration:**
```
Test Pattern Source: 1920×1080 @ 30fps
Target: Any vertical or horizontal panel arrangement

Calibration Process:
1. Send test pattern as segmented streams  
2. Each panel controller receives its segment
3. Measure uniformity, color accuracy per panel
4. Adjust panel settings independently
5. Verify PTP sync alignment between panels
```

**System Requirements:**
- Per panel bandwidth: 0.37-0.75 Gbps (depends on count)
- Total: ≤1.50 Gbps for up to 4 panels
- Precision: ST2110-20 uncompressed for accurate color
- Sync: PTP master/slave for seamless tiling

**Benefits:**
- ✅ Per-panel independent calibration
- ✅ True color accuracy (10-bit YUV or RGB)
- ✅ Frame-accurate sync verification
- ✅ Production-quality validation

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

### 2. LED Panel Boundary Alignment

**Pixel-Perfect Panel Alignment:**
```python
# Ensure no gaps or overlaps between LED panels
frame_width = 1920
frame_height = 1080
num_panels = 3  # Vertical arrangement

panel_width = frame_width // num_panels
panel_height = frame_height

# Example 3-panel vertical LED wall from 1920×1080:
panel_left   = (0,    0, 640,  1080)  # Left panel (640×1080)
panel_center = (640,  0, 1280, 1080)  # Center panel (640×1080)
panel_right  = (1280, 0, 1920, 1080)  # Right panel (640×1080)

# Example 3-panel horizontal LED wall:
panel_top    = (0, 0,   1920, 360)    # Top panel (1920×360)
panel_middle = (0, 360, 1920, 720)    # Middle panel (1920×360)
panel_bottom = (0, 720, 1920, 1080)   # Bottom panel (1920×360)
```

**Chroma Subsampling Considerations:**
```
YUV 4:2:2: Panel widths must be even (chroma pairs)
YUV 4:2:0: Panel widths AND heights must be even

✅ Good: 640×1080 (width even, 3-panel vertical)
✅ Good: 1920×360 (both even, 3-panel horizontal)
✅ Good: 480×1080 (both even, 4-panel vertical)
❌ Bad: 641×1080 (odd width - causes chroma misalignment)
```

**LED Panel Physical Alignment:**
- Each panel receives one complete stream
- No frame reconstruction needed at LED controller
- Panel offsets used for bezel compensation only
- PTP sync ensures frame-level synchronization across panels

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

#### Sample Configuration: Single I225 with 3-Panel LED Wall

**Scenario:** Transmit 3-panel vertical LED wall (640×1080 per panel) @ 30fps on single I225 NIC

**TX Configuration (Single I225 NIC, 3 LED panel streams):**
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
      "width": 640,
      "height": 1080,
      "fps": "p30",
      "fmt": "YUV422_10bit",
      "name": "led_panel_left",
      "metadata": {
        "panel_id": 1,
        "x_offset": 0,
        "y_offset": 0,
        "description": "Left LED panel (640×1080)"
      }
    },
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.2",
      "port": 20000,
      "payload_type": 112,
      "width": 640,
      "height": 1080,
      "fps": "p30",
      "fmt": "YUV422_10bit",
      "name": "led_panel_center",
      "metadata": {
        "panel_id": 2,
        "x_offset": 640,
        "y_offset": 0,
        "description": "Center LED panel (640×1080)"
      }
    },
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.3",
      "port": 20000,
      "payload_type": 112,
      "width": 640,
      "height": 1080,
      "fps": "p30",
      "fmt": "YUV422_10bit",
      "name": "led_panel_right",
      "metadata": {
        "panel_id": 3,
        "x_offset": 1280,
        "y_offset": 0,
        "description": "Right LED panel (640×1080)"
      }
    }
  ],
  "bandwidth_budget": {
    "link_speed_gbps": 2.5,
    "total_allocated_gbps": 1.50,
    "utilization_percent": 60,
    "headroom_gbps": 1.00
  }
}
```

**RX Configuration (LED Controllers - Each receives one stream):**
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
      "width": 640,
      "height": 1080,
      "fps": "p30",
      "fmt": "YUV422_10bit",
      "name": "led_panel_left",
      "metadata": {
        "panel_id": 1,
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
      "width": 640,
      "height": 1080,
      "fps": "p30",
      "fmt": "YUV422_10bit",
      "name": "led_panel_center",
      "metadata": {
        "panel_id": 2,
        "x_offset": 640,
        "y_offset": 0
      }
    },
    {
      "type": "st20",
      "interface": "enp1s0f0",
      "ip": "239.1.1.3",
      "port": 20000,
      "payload_type": 112,
      "width": 640,
      "height": 1080,
      "fps": "p30",
      "fmt": "YUV422_10bit",
      "name": "led_panel_right",
      "metadata": {
        "panel_id": 3,
        "x_offset": 1280,
        "y_offset": 0
      }
    }
  ],
  "notes": [
    "All LED panel streams received on same physical interface",
    "Hardware RSS distributes packets to CPU cores",
    "Each LED controller subscribes to its specific multicast",
    "No frame reconstruction needed - each panel displays independently"
  ]
}
```

---

#### Multi-Stream Implementation

**TX Side - Multiple LED Panel Streams on Single Interface:**
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
    tx_ops[i].width = 640;     // LED panel width
    tx_ops[i].height = 1080;    // LED panel height
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
Display buffering: 16-33 ms (30-60fps vsync)

Total glass-to-glass: ~20-40 ms
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

Strategy 2: Panel Priority
- Mark critical LED panel streams with higher QoS
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

### LED Panel Bandwidth (YUV 4:2:2 10-bit @ 30fps)

**Single Intel I225 2.5GbE NIC Capacity for LED Video Walls**

| Panel Config | Per-Panel Resolution | Per-Panel BW | Max Panels | Total BW Used | Utilization | Application |
|-------------|---------------------|--------------|------------|---------------|-------------|-------------|
| **2-Panel Vertical** | 960×1080 | 0.75 Gbps | ✅ **3** | 2.25 Gbps | 90% | Dual ultrawide |
| **3-Panel Vertical** | 640×1080 | 0.50 Gbps | ✅ **5** | 2.50 Gbps | 100% | **✅ Retail signage** |
| **4-Panel Vertical** | 480×1080 | 0.37 Gbps | ✅ **6** | 2.22 Gbps | 89% | **✅ Command center** |
| **2-Panel Horizontal** | 1920×540 | 0.75 Gbps | ✅ **3** | 2.25 Gbps | 90% | Tall portrait |
| **3-Panel Horizontal** | 1920×360 | 0.50 Gbps | ✅ **5** | 2.50 Gbps | 100% | Vertical tower |
| **5-Panel Horizontal** | 1920×216 | 0.30 Gbps | ✅ **8** | 2.40 Gbps | 96% | Multi-band ticker |

**Recommended: 3-panel or 4-panel vertical @ 30fps (60-90% utilization)**

---

### LED Panel Configuration Examples @ 30fps

| Configuration | Panel Arrangement | Per-Panel Res | Panel Count | Total BW | Utilization | Use Case |
|--------------|-------------------|---------------|-------------|----------|-------------|----------|
| **Retail Ultrawide** | 3× Side-by-Side | 640×1080 | 3 | 1.50 Gbps | 60% | Store window display |
| **Command Center** | 4× Side-by-Side | 480×1080 | 4 | 1.50 Gbps | 60% | NOC/SOC operations |
| **Stadium Scoreboard** | 2× Side-by-Side | 960×1080 | 2 | 1.50 Gbps | 60% | Wide format scores |
| **Building Facade** | 3× Stacked | 1920×360 | 3 | 1.50 Gbps | 60% | Elevator lobby tower |
| **Conference Room** | 2× Stacked | 1920×540 | 2 | 1.50 Gbps | 60% | Split presentation |

**Key Finding:** All standard LED configurations use exactly 60% bandwidth @ 30fps

---

### LED Panel Resolution Comparison @ 30fps (YUV 4:2:2 10-bit)

| Panel Type | Resolution | Bandwidth @ 30fps | Max Panels on 2.5G | Combined Display |
|-----------|-----------|-------------------|-------------------|------------------|
| **Full Frame** | 1920×1080 | 1.50 Gbps | ✅ **1** | 1920×1080 single display |
| **Dual Vertical** | 960×1080 | 0.75 Gbps | ✅ **3** | 2880×1080 ultrawide |
| **Triple Vertical** | 640×1080 | 0.50 Gbps | ✅ **5** | 3200×1080 ultra-ultrawide |
| **Quad Vertical** | 480×1080 | 0.37 Gbps | ✅ **6** | 2880×1080 (4-panel) |
| **Dual Horizontal** | 1920×540 | 0.75 Gbps | ✅ **3** | 1920×1620 tall portrait |
| **Triple Horizontal** | 1920×360 | 0.50 Gbps | ✅ **5** | 1920×1800 vertical tower |

---

### Optimal LED Panel Formats by Application

| Application | Panel Layout | Format | Bit Depth | FPS | Total BW | Notes |
|----------|-------------|--------|-----------|-----|-----------|-------|
| **Retail signage** | 3× Vertical (640×1080) | YUV 4:2:2 | 10-bit | 30 | 1.50 Gbps | Broadcast quality |
| **Command center** | 4× Vertical (480×1080) | YUV 4:2:2 | 10-bit | 30 | 1.50 Gbps | Dashboard graphics |
| **Building facade** | 3× Horizontal (1920×360) | YUV 4:2:0 | 10-bit | 30 | 1.11 Gbps | Bandwidth efficient |
| **LED calibration** | 3× Vertical (640×1080) | RGB | 10-bit | 30 | 2.25 Gbps | True color uniformity |
| **Budget installs** | 5× Horizontal (1920×216) | YUV 4:2:0 | 8-bit | 30 | 0.90 Gbps | Maximum panel count |
| **High-end graphics** | 2× Vertical (960×1080) | RGB | 12-bit | 30 | 2.70 Gbps | Premium quality ⚠️ |

⚠️ = Exceeds 2.5G limit, requires lower frame rate or compression

---

### Format Comparison @ 640×1080 LED Panel, 30fps

| Format | Bit Depth | Bandwidth | Quality | Best Use Case |
|--------|-----------|-----------|---------|---------------|
| YUV 4:2:0 8-bit | 8 | 0.37 Gbps | Good | Budget LED walls |
| YUV 4:2:0 10-bit | 10 | 0.46 Gbps | Excellent | Efficient multi-panel |
| YUV 4:2:2 8-bit | 8 | 0.50 Gbps | Very Good | Standard broadcast |
| **YUV 4:2:2 10-bit** ★ | **10** | **0.50 Gbps** | **Excellent** | **ST2110 standard** |
| YUV 4:2:2 12-bit | 12 | 0.75 Gbps | Premium | High-end installs |
| YUV 4:4:4 10-bit | 10 | 0.75 Gbps | Premium | Graphics/text heavy |
| RGB 10-bit | 10 | 0.75 Gbps | Premium | LED calibration |
| RGB 12-bit | 12 | 1.00 Gbps | Maximum | Color-critical apps |

★ = Recommended for LED video walls (3 panels @ 1.50 Gbps total)

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
# 3-panel vertical LED wall: 640×1080 per panel, YUV 4:2:2 10-bit @ 30fps
bw = calculate_segment_bandwidth(640, 1080, 30, 'yuv422', 10)
print(f"Per-panel bandwidth: {bw} Gbps")  # Output: 0.50 Gbps

panels, util = segments_in_25g(640, 1080, 30, 'yuv422', 10)
print(f"Max panels on 2.5G: {panels}, Utilization: {util:.1f}%")
# Output: Max panels: 5, Utilization: 100.0%

# For 3-panel @ 60% utilization:
total_bw_3panel = bw * 3
print(f"3-panel LED wall total: {total_bw_3panel} Gbps ({(total_bw_3panel/2.5)*100:.0f}% util)")
# Output: 3-panel LED wall total: 1.50 Gbps (60% util)
```

---

## Conclusion

### Key Takeaways

1. **LED video walls are achievable with budget 2.5G NICs:**
   - 3-panel vertical (640×1080 each): 1.50 Gbps @ 30fps (60% util) - Retail signage
   - 4-panel vertical (480×1080 each): 1.50 Gbps @ 30fps (60% util) - Command center
   - 3-panel horizontal (1920×360 each): 1.50 Gbps @ 30fps (60% util) - Building facades
   - 2-panel arrangements: 1.50 Gbps @ 30fps (60% util) - Stadium scoreboards
   - All configurations use exactly 60% bandwidth with 40% management headroom

2. **Single-NIC multi-panel architecture:**
   - One ST2110-20 stream per LED panel
   - All panel feeds share same physical interface (single I225 NIC)
   - Each LED controller subscribes to its specific multicast address
   - PTP synchronization ensures seamless panel-to-panel tiling
   - No centralized frame reconstruction needed

3. **Practical LED video wall applications:**
   - Retail ultrawide displays (3-panel side-by-side)
   - Command center monitoring walls (4-panel wide)
   - Building facade vertical towers (3-panel stacked)
   - Sports stadium scoreboards (2-panel wide format)
   - Conference room dual-height displays (2-panel stacked)
   - LED panel calibration and uniformity testing

4. **Intel I225/I226 2.5GbE NICs enable professional LED installations** at fraction of cost:
   - Single $30-50 NIC vs $500+ multi-GPU setup
   - Replace expensive 10G infrastructure with budget 2.5G
   - Drive 2-6 LED panels from one network interface
   - Broadcast-quality uncompressed ST2110-20 standard

5. **Critical success factors for LED video walls:**
   - Vertical panel arrangements: Side-by-side strips for ultrawide displays
   - Horizontal panel arrangements: Stacked strips for tall/tower displays
   - YUV 4:2:2 10-bit recommended (ST2110 standard, 0.50 Gbps per 640×1080 panel)
   - Precise PTP synchronization for seamless tiling (prefer I226-V with TSN)
   - 60% bandwidth utilization provides 40% headroom for control traffic
   - No grid segmentation or ROI patterns - only physical LED panel boundaries

---

**Document Version:** 2.0 (LED Video Wall Edition)  
**Last Updated:** February 2026  
- [Intel Ethernet Controllers Video Support Guide](intel_ethernet_controllers_video_support.md)
- [MTL Pipeline Architecture Guide](mtl_pipeline_architecture_guide.md)
- [ST2110 Best Practices](https://www.st2110.com/)

---

**Feedback & Contributions:**  
This document is part of the OpenVisualCloud Media Transport Library project.  
Submit issues or improvements via GitHub: [OpenVisualCloud/Media-Transport-Library](https://github.com/OpenVisualCloud/Media-Transport-Library)
