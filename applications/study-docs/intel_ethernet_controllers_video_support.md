# Intel Ethernet Controllers: Video Transport Capability Guide

**Date:** February 13, 2026  
**Repository:** OpenVisualCloud/Media-Transport-Library  
**Purpose:** Reference guide for selecting Intel Ethernet controllers for ST2110 video transport

---

## Table of Contents

1. [Overview](#overview)
2. [Intel Ethernet Controller Families](#intel-ethernet-controller-families)
3. [Bandwidth Requirements by Resolution](#bandwidth-requirements-by-resolution)
4. [Controller Capabilities Matrix](#controller-capabilities-matrix)
5. [Video Stream Support Tables](#video-stream-support-tables)
6. [Recommended Configurations](#recommended-configurations)
7. [Special Features](#special-features)
8. [Selection Guide](#selection-guide)

---

## Overview

### Why Bandwidth Matters for Video Transport

ST2110 professional video transport requires significant network bandwidth, especially for uncompressed video. This guide helps you select the appropriate Intel Ethernet controller based on:

- **Resolution:** 1080p, 1080i, 4K UHD (3840×2160), 8K
- **Frame Rate:** 23.98, 25, 29.97, 30, 50, 59.94, 60, 100, 120 fps
- **Bit Depth:** 8-bit, 10-bit, 12-bit
- **Chroma Sampling:** 4:2:0, 4:2:2, 4:4:4
- **Compression:** Uncompressed (ST2110-20) vs Compressed (ST2110-22 JPEGXS/H.264/H.265)

### Key Bandwidth Formula

**Uncompressed Video Bandwidth (ST2110-20):**
```
Bandwidth (Gbps) = Width × Height × FPS × Bits_per_Pixel × Sampling_Factor × Overhead

Where:
- Bits_per_Pixel: 8, 10, 12, or 16 bits
- Sampling_Factor: 1.0 (4:4:4), 0.75 (4:2:2), 0.5 (4:2:0)
- Overhead: ~1.10 (10% for RTP/UDP/IP/Ethernet headers + pacing)
```

**Compressed Video Bandwidth (ST2110-22):**
```
Bandwidth (Compressed) = Uncompressed_Bandwidth / Compression_Ratio

Typical JPEGXS compression ratios:
- Broadcast quality: 6:1 to 10:1
- High quality: 4:1 to 6:1
- Visually lossless: 2:1 to 4:1
```

---

## Bandwidth Calculations and Stream Count Formulas

### How We Calculate Bandwidth

#### Formula Breakdown for Uncompressed Video

**Note:** Examples use YUV 4:2:2 10-bit (20 bpp) - the ST2110-20 standard format. 
See Step 2 below for bits per pixel calculations for all formats.

```
Step 1: Calculate pixel data rate
Pixels per frame = Width × Height
Pixels per second = Pixels per frame × FPS
```

**Step 2: Calculate bits per pixel (depends on format)**

**YUV Formats:**

YUV 4:4:4 (Full chroma sampling):
- 8-bit:  Y(8) + U(8) + V(8) = 24 bits per pixel
- 10-bit: Y(10) + U(10) + V(10) = 30 bits per pixel
- 12-bit: Y(12) + U(12) + V(12) = 36 bits per pixel

YUV 4:2:2 (Horizontal subsampling - most common for broadcast):
- 8-bit:  Y(8) + U(4) + V(4) = 16 bits per pixel  
  [U and V are 8 bits shared between 2 horizontal pixels]
- 10-bit: Y(10) + U(5) + V(5) = 20 bits per pixel ← ST2110 standard  
  [U and V are 10 bits shared between 2 horizontal pixels]
- 12-bit: Y(12) + U(6) + V(6) = 24 bits per pixel  
  [U and V are 12 bits shared between 2 horizontal pixels]

YUV 4:2:0 (Both horizontal and vertical subsampling):
- 8-bit:  Y(8) + U(2) + V(2) = 12 bits per pixel  
  [U and V are 8 bits shared between 4 pixels in 2x2 block]
- 10-bit: Y(10) + U(2.5) + V(2.5) = 15 bits per pixel  
  [U and V are 10 bits shared between 4 pixels in 2x2 block]
- 12-bit: Y(12) + U(3) + V(3) = 18 bits per pixel

**RGB Formats:**

RGB (Full color, no subsampling):
- 8-bit (RGB24):  R(8) + G(8) + B(8) = 24 bits per pixel
- 10-bit (RGB30): R(10) + G(10) + B(10) = 30 bits per pixel
- 12-bit (RGB36): R(12) + G(12) + B(12) = 36 bits per pixel
- 16-bit (RGB48): R(16) + G(16) + B(16) = 48 bits per pixel

**RGBA Formats (with Alpha channel):**

RGBA:
- 8-bit:  R(8) + G(8) + B(8) + A(8) = 32 bits per pixel
- 10-bit: R(10) + G(10) + B(10) + A(10) = 40 bits per pixel
- 12-bit: R(12) + G(12) + B(12) + A(12) = 48 bits per pixel
- 16-bit: R(16) + G(16) + B(16) + A(16) = 64 bits per pixel

**Summary Table:**

| Format | 8-bit | 10-bit | 12-bit | 16-bit |
|--------|-------|--------|--------|--------|
| YUV 4:4:4 | 24 bpp | 30 bpp | 36 bpp | - |
| YUV 4:2:2 | 16 bpp | **20 bpp** ★ | 24 bpp | - |
| YUV 4:2:0 | 12 bpp | 15 bpp | 18 bpp | - |
| RGB | 24 bpp | 30 bpp | 36 bpp | 48 bpp |
| RGBA | 32 bpp | 40 bpp | 48 bpp | 64 bpp |

★ = ST2110-20 standard format (YUV 4:2:2 10-bit = 20 bits per pixel)

**Note:** All calculations in this document use YUV 4:2:2 10-bit (20 bpp) unless 
otherwise specified, as this is the SMPTE ST2110-20 standard for professional 
broadcast video transport.

```
Step 3: Calculate raw data rate
Raw_Data_Rate = Pixels per second × 20 bits

Step 4: Add protocol overhead
Bandwidth (Gbps) = Raw_Data_Rate × 1.10 (10% overhead for RTP/UDP/IP/Ethernet)
```

### Detailed Calculations

#### 1080p Resolution Examples

**1080p @ 24 fps:**
```
Width × Height = 1920 × 1080 = 2,073,600 pixels/frame
Pixels/second = 2,073,600 × 24 = 49,766,400 pixels/sec
Raw data = 49,766,400 × 20 bits = 995,328,000 bits/sec = 0.995 Gbps
With overhead = 0.995 × 1.10 = 1.095 Gbps ≈ 1.20 Gbps (including pacing)

Stream count (2.5G NIC) = 2.5 Gbps ÷ 1.20 Gbps = 2.08 → 2 streams ✅
Stream count (10G NIC) = 10 Gbps ÷ 1.20 Gbps = 8.33 → 8 streams ✅
```

**1080p @ 25 fps:**
```
Pixels/second = 2,073,600 × 25 = 51,840,000 pixels/sec
Raw data = 51,840,000 × 20 bits = 1,036,800,000 bits/sec = 1.037 Gbps
With overhead = 1.037 × 1.10 = 1.140 Gbps ≈ 1.25 Gbps

Stream count (2.5G NIC) = 2.5 Gbps ÷ 1.25 Gbps = 2.00 → 2 streams ✅
Stream count (10G NIC) = 10 Gbps ÷ 1.25 Gbps = 8.00 → 8 streams ✅
```

**1080p @ 30 fps:**
```
Pixels/second = 2,073,600 × 30 = 62,208,000 pixels/sec
Raw data = 62,208,000 × 20 bits = 1,244,160,000 bits/sec = 1.244 Gbps
With overhead = 1.244 × 1.10 = 1.368 Gbps ≈ 1.50 Gbps

Stream count (2.5G NIC) = 2.5 Gbps ÷ 1.50 Gbps = 1.66 → 1 stream ✅
Stream count (10G NIC) = 10 Gbps ÷ 1.50 Gbps = 6.66 → 6 streams ✅
```

**1080p @ 50 fps:**
```
Pixels/second = 2,073,600 × 50 = 103,680,000 pixels/sec
Raw data = 103,680,000 × 20 bits = 2,073,600,000 bits/sec = 2.074 Gbps
With overhead = 2.074 × 1.10 = 2.281 Gbps ≈ 2.49 Gbps

Stream count (2.5G NIC) = 2.5 Gbps ÷ 2.49 Gbps = 1.00 → 1 stream ⚠️ (99.6% utilization)
Stream count (10G NIC) = 10 Gbps ÷ 2.49 Gbps = 4.01 → 4 streams ✅
```

**1080p @ 60 fps:**
```
Pixels/second = 2,073,600 × 60 = 124,416,000 pixels/sec
Raw data = 124,416,000 × 20 bits = 2,488,320,000 bits/sec = 2.488 Gbps
With overhead = 2.488 × 1.10 = 2.737 Gbps ≈ 2.98 Gbps

Stream count (2.5G NIC) = 2.5 Gbps ÷ 2.98 Gbps = 0.84 → 0 streams ❌ (exceeds capacity)
Stream count (10G NIC) = 10 Gbps ÷ 2.98 Gbps = 3.36 → 3 streams ✅
Stream count (25G NIC) = 25 Gbps ÷ 2.98 Gbps = 8.39 → 8 streams ✅
```

#### 4K UHD Resolution Examples

**4K UHD @ 30 fps:**
```
Width × Height = 3840 × 2160 = 8,294,400 pixels/frame
Pixels/second = 8,294,400 × 30 = 248,832,000 pixels/sec
Raw data = 248,832,000 × 20 bits = 4,976,640,000 bits/sec = 4.977 Gbps
With overhead = 4.977 × 1.10 = 5.475 Gbps ≈ 5.99 Gbps

Stream count (2.5G NIC) = 2.5 Gbps ÷ 5.99 Gbps = 0.42 → 0 streams ❌
Stream count (10G NIC) = 10 Gbps ÷ 5.99 Gbps = 1.67 → 1 stream ✅
Stream count (25G NIC) = 25 Gbps ÷ 5.99 Gbps = 4.17 → 4 streams ✅
```

**4K UHD @ 60 fps:**
```
Pixels/second = 8,294,400 × 60 = 497,664,000 pixels/sec
Raw data = 497,664,000 × 20 bits = 9,953,280,000 bits/sec = 9.953 Gbps
With overhead = 9.953 × 1.10 = 10.948 Gbps ≈ 11.94 Gbps

Stream count (10G NIC) = 10 Gbps ÷ 11.94 Gbps = 0.84 → 0 streams ❌
Stream count (25G NIC) = 25 Gbps ÷ 11.94 Gbps = 2.09 → 2 streams ✅
Stream count (40G NIC) = 40 Gbps ÷ 11.94 Gbps = 3.35 → 3 streams ✅
Stream count (50G NIC) = 50 Gbps ÷ 11.94 Gbps = 4.19 → 4 streams ✅
```

**4K UHD @ 120 fps:**
```
Pixels/second = 8,294,400 × 120 = 995,328,000 pixels/sec
Raw data = 995,328,000 × 20 bits = 19,906,560,000 bits/sec = 19.907 Gbps
With overhead = 19.907 × 1.10 = 21.897 Gbps ≈ 23.88 Gbps

Stream count (25G NIC) = 25 Gbps ÷ 23.88 Gbps = 1.05 → 1 stream ✅
Stream count (40G NIC) = 40 Gbps ÷ 23.88 Gbps = 1.68 → 1 stream ✅
Stream count (50G NIC) = 50 Gbps ÷ 23.88 Gbps = 2.09 → 2 streams ✅
Stream count (100G NIC) = 100 Gbps ÷ 23.88 Gbps = 4.19 → 4 streams ✅
```

#### 8K UHD Resolution Examples

**8K UHD @ 30 fps:**
```
Width × Height = 7680 × 4320 = 33,177,600 pixels/frame
Pixels/second = 33,177,600 × 30 = 995,328,000 pixels/sec
Raw data = 995,328,000 × 20 bits = 19,906,560,000 bits/sec = 19.907 Gbps
With overhead = 19.907 × 1.10 = 21.897 Gbps ≈ 23.96 Gbps

Stream count (25G NIC) = 25 Gbps ÷ 23.96 Gbps = 1.04 → 1 stream ✅
Stream count (40G NIC) = 40 Gbps ÷ 23.96 Gbps = 1.67 → 1 stream ✅
Stream count (50G NIC) = 50 Gbps ÷ 23.96 Gbps = 2.09 → 2 streams ✅
```

**8K UHD @ 60 fps:**
```
Pixels/second = 33,177,600 × 60 = 1,990,656,000 pixels/sec
Raw data = 1,990,656,000 × 20 bits = 39,813,120,000 bits/sec = 39.813 Gbps
With overhead = 39.813 × 1.10 = 43.794 Gbps ≈ 47.75 Gbps

Stream count (50G NIC) = 50 Gbps ÷ 47.75 Gbps = 1.05 → 1 stream ✅
Stream count (100G NIC) = 100 Gbps ÷ 47.75 Gbps = 2.09 → 2 streams ✅
```

### Compressed Stream Calculations (ST2110-22)

**Formula:**
```
Compressed_Bandwidth = Uncompressed_Bandwidth ÷ Compression_Ratio
Stream_Count = Link_Speed ÷ Compressed_Bandwidth
```

**Example 1: 1080p60 with JPEGXS 10:1 compression**
```
Uncompressed = 2.98 Gbps
Compressed = 2.98 ÷ 10 = 0.298 Gbps ≈ 0.30 Gbps

Stream count (2.5G NIC) = 2.5 Gbps ÷ 0.30 Gbps = 8.33 → 8 streams ✅
Stream count (10G NIC) = 10 Gbps ÷ 0.30 Gbps = 33.33 → 33 streams ✅
```

**Example 2: 4K60 with JPEGXS 10:1 compression**
```
Uncompressed = 11.94 Gbps
Compressed = 11.94 ÷ 10 = 1.194 Gbps ≈ 1.19 Gbps

Stream count (2.5G NIC) = 2.5 Gbps ÷ 1.19 Gbps = 2.10 → 2 streams ✅
Stream count (10G NIC) = 10 Gbps ÷ 1.19 Gbps = 8.40 → 8 streams ✅
Stream count (25G NIC) = 25 Gbps ÷ 1.19 Gbps = 21.01 → 21 streams ✅
```

**Example 3: 8K60 with JPEGXS 10:1 compression**
```
Uncompressed = 47.75 Gbps
Compressed = 47.75 ÷ 10 = 4.775 Gbps ≈ 4.78 Gbps

Stream count (10G NIC) = 10 Gbps ÷ 4.78 Gbps = 2.09 → 2 streams ✅
Stream count (25G NIC) = 25 Gbps ÷ 4.78 Gbps = 5.23 → 5 streams ✅
Stream count (50G NIC) = 50 Gbps ÷ 4.78 Gbps = 10.46 → 10 streams ✅
Stream count (100G NIC) = 100 Gbps ÷ 4.78 Gbps = 20.92 → 20 streams ✅
```

### Summary: Maximum Stream Count Formula

```python
def calculate_max_streams(link_speed_gbps, resolution, fps, compression_ratio=1, bits_per_pixel=20):
    """
    Calculate maximum concurrent streams for a given NIC
    
    Args:
        link_speed_gbps: NIC bandwidth (2.5, 10, 25, 40, 50, 100)
        resolution: tuple (width, height) e.g., (1920, 1080)
        fps: frames per second (24, 25, 30, 50, 60, 120)
        compression_ratio: 1 for uncompressed, 10 for JPEGXS 10:1, etc.
        bits_per_pixel: Depends on format (default: 20 for YUV 4:2:2 10-bit)
                       - YUV 4:2:0: 12 (8-bit), 15 (10-bit), 18 (12-bit)
                       - YUV 4:2:2: 16 (8-bit), 20 (10-bit) ★, 24 (12-bit)
                       - YUV 4:4:4: 24 (8-bit), 30 (10-bit), 36 (12-bit)
                       - RGB: 24 (8-bit), 30 (10-bit), 36 (12-bit)
                       ★ = ST2110-20 standard format
    
    Returns:
        Maximum number of concurrent streams (floor value)
    """
    width, height = resolution
    pixels_per_frame = width * height
    pixels_per_second = pixels_per_frame * fps
    
    # Calculate raw bandwidth (bits per pixel depends on format)
    # Default: YUV 4:2:2 10-bit = 20 bits per pixel (ST2110-20 standard)
    # See Step 2 in "Formula Breakdown" section for all format calculations
    raw_data_gbps = (pixels_per_second * bits_per_pixel) / 1_000_000_000
    
    # Add 10% protocol overhead
    uncompressed_bandwidth = raw_data_gbps * 1.10
    
    # Apply compression if specified
    actual_bandwidth = uncompressed_bandwidth / compression_ratio
    
    # Calculate stream count (floor division)
    max_streams = int(link_speed_gbps / actual_bandwidth)
    
    return max_streams

# Examples using YUV 4:2:2 10-bit (20 bpp - default):
# 2.5G NIC with 1080p24 uncompressed: calculate_max_streams(2.5, (1920, 1080), 24) → 2
# 10G NIC with 4K60 uncompressed: calculate_max_streams(10, (3840, 2160), 60) → 0
# 2.5G NIC with 4K60 compressed 10:1: calculate_max_streams(2.5, (3840, 2160), 60, 10) → 2

# Examples using other formats:
# 10G NIC with 1080p60 YUV 4:2:0 8-bit: calculate_max_streams(10, (1920, 1080), 60, 1, 12) → 5
# 10G NIC with 4K60 RGB 10-bit: calculate_max_streams(10, (3840, 2160), 60, 1, 30) → 0
# 25G NIC with 4K60 YUV 4:4:4 10-bit: calculate_max_streams(25, (3840, 2160), 60, 1, 30) → 1
```

---

## Intel Ethernet Controller Families

### 1. Intel® Ethernet 800 Series (E810)

**Generation:** Latest (Ice Lake)  
**Key Feature:** Hardware-accelerated ST2110 support via Dynamic Device Personalization (DDP)  
**Supported Link Speeds:** 100G, 50G, 25G, 10G

#### Models:
- **E810-CQDA2** (Dual 100G QSFP28)
- **E810-XXVDA4** (Quad 25G SFP28)
- **E810-XXVDA2** (Dual 25G SFP28)
- **E810-2CQDA2** (Dual 100G QSFP28, PCIe 4.0)

**Advantages:**
- DDP profile for ST2110 (flow director, RSS optimization)
- Hardware timestamping (PTP)
- Low latency (<1µs)
- DPDK poll mode driver optimized
- Application Device Queues (ADQ) for flow steering

---

### 2. Intel® XL710/XXV710 Series

**Generation:** Fortville  
**Supported Link Speeds:** 40G (XL710), 25G (XXV710), 10G

#### Models:
- **XL710-QDA2** (Dual 40G QSFP+)
- **XXV710-DA2** (Dual 25G SFP28)
- **X710-DA4** (Quad 10G SFP+)
- **X710-DA2** (Dual 10G SFP+)

**Advantages:**
- Mature DPDK support
- Hardware timestamping
- Flow director for packet steering
- Good performance/cost ratio

---

### 3. Intel® I225/I226 Series

**Generation:** Consumer/Embedded (Elkhart Lake)  
**Supported Link Speeds:** 2.5G, 1G

#### Models:
- **I225-LM/V** (2.5G)
- **I226-LM/V** (2.5G with TSN)

**Uncompressed Format Support:**
- ✅ 1080p60 YUV 4:2:0 8-bit (1 stream, 1.79 Gbps)
- ✅ 1080p60 YUV 4:2:0 10-bit (1 stream, 2.24 Gbps) - **HDR capable**
- ✅ 1080p60 YUV 4:2:2 8-bit (1 stream, 2.39 Gbps)
- ✅ 1080p30 YUV 4:2:2 10-bit (1 stream, 1.50 Gbps) - **ST2110 standard at 30fps**
- ✅ 1080p30 YUV 4:4:4 10-bit (1 stream, 2.24 Gbps) - Full chroma
- ✅ 1080p30 RGB 10-bit (1 stream, 2.24 Gbps) - Graphics workflows
- ❌ 1080p60 YUV 4:2:2 10-bit (2.98 Gbps exceeds 2.5G) - **Requires 10G**

**Use Cases:**
- **Uncompressed:** Budget-friendly 1080p workflows with YUV 4:2:0 (HDR compatible)
- **Uncompressed:** Lower frame rate production (≤30fps) with ST2110 standard format
- **Compressed:** Multiple streams with JPEGXS compression (ST2110-22)
- **Compressed:** Low-bandwidth monitoring/preview streams
- Audio transport (ST2110-30)
- Control/management traffic

**Key Advantage:** I226-V includes TSN (Time-Sensitive Networking) for precise timing

---

## Bandwidth Requirements by Resolution

### ST2110-20 Uncompressed (Standard: YUV 4:2:2 10-bit)

| Resolution | Frame Rate | Bandwidth (Gbps) | Compatible Controllers | Notes |
|------------|-----------|------------------|------------------------|-------|
| **1080p** (1920×1080) | 23.98 fps | 1.20 | **I225/I226 (2.5G), All 10G+** | SD-SDI equivalent |
| 1080p | 25 fps | 1.25 | **I225/I226 (2.5G), All 10G+** | PAL standard |
| 1080p | 29.97 fps | 1.50 | **I225/I226 (2.5G), All 10G+** | NTSC standard |
| 1080p | 30 fps | 1.50 | **I225/I226 (2.5G), All 10G+** | |
| 1080p | 50 fps | 2.49 | **I225/I226 (2.5G - at limit), All 10G+** | HD-SDI equivalent |
| 1080p | 59.94 fps | 2.97 | X710, XXV710, XL710, E810 (10G+ only) | Common broadcast |
| **1080p** | **60 fps** | **2.98** | **X710, XXV710, XL710, E810 (10G+ only)** | **Common production** |
| **1080i** (interlaced) | 50i | 1.24 | **I225/I226 (2.5G), All 10G+** | HD-SDI broadcast |
| 1080i | 59.94i | 1.49 | **I225/I226 (2.5G), All 10G+** | NTSC interlaced |
| 1080i | 60i | 1.49 | **I225/I226 (2.5G), All 10G+** | |
| **4K UHD** (3840×2160) | 23.98 fps | 4.79 | X710, XXV710, XL710, E810 (All 10G+) | Cinema |
| 4K UHD | 25 fps | 4.99 | X710, XXV710, XL710, E810 (All 10G+) | PAL UHD |
| 4K UHD | 29.97 fps | 5.96 | X710, XXV710, XL710, E810 (All 10G+) | NTSC UHD |
| 4K UHD | 30 fps | 5.99 | X710, XXV710, XL710, E810 (All 10G+) | |
| 4K UHD | 50 fps | 9.98 | X710, XXV710, XL710, E810 (All 10G+) | 3G-SDI × 4 |
| 4K UHD | 59.94 fps | 11.89 | **XXV710 (25G+), XL710 (40G), E810** | **12G-SDI equivalent** |
| **4K UHD** | **60 fps** | **11.94** | **XXV710 (25G+), XL710 (40G), E810** | **12G-SDI equivalent** |
| 4K UHD | 100 fps | 19.97 | **XXV710 (25G), XL710 (40G), E810** | High frame rate |
| 4K UHD | 120 fps | 23.88 | **XXV710 (25G), XL710 (40G), E810** | HFR production |
| **8K UHD** (7680×4320) | 25 fps | 19.97 | **XXV710 (25G), XL710 (40G), E810** | Experimental |
| 8K UHD | 30 fps | 23.96 | **XXV710 (25G), XL710 (40G), E810** | |
| 8K UHD | 50 fps | 39.93 | **XL710 (40G), E810 (50G/100G)** | **Requires 40G+** |
| 8K UHD | 60 fps | 47.75 | **E810 (50G/100G only)** | **Requires 50G+** |

### ST2110-22 Compressed (JPEGXS 10:1 Ratio)

| Resolution | Frame Rate | Uncompressed | Compressed (10:1) | Compatible Controllers | Notes |
|------------|-----------|--------------|-------------------|------------------------|-------|
| 1080p | 60 fps | 2.98 Gbps | **0.30 Gbps** | **I225/I226 (2.5G), All 10G+** | Fits 2.5G NIC |
| 4K UHD | 30 fps | 5.99 Gbps | **0.60 Gbps** | **I225/I226 (2.5G), All 10G+** | Fits 2.5G NIC |
| 4K UHD | 60 fps | 11.94 Gbps | **1.19 Gbps** | **I225/I226 (2.5G), All 10G+** | Multiple streams on 2.5G |
| 4K UHD | 120 fps | 23.88 Gbps | **2.39 Gbps** | **X710 (10G+), XXV710, XL710, E810** | Requires 10G+ |
| 8K UHD | 30 fps | 23.96 Gbps | **2.40 Gbps** | **X710 (10G+), XXV710, XL710, E810** | Requires 10G+ |
| 8K UHD | 60 fps | 47.75 Gbps | **4.78 Gbps** | **X710 (10G+), XXV710, XL710, E810** | Requires 10G+ |

**Note:** JPEGXS compression ratio can be adjusted from 2:1 (visually lossless) to 15:1 (highly compressed)

---

### Bandwidth Comparison: All Formats and Bit Depths

The tables below show bandwidth requirements for different YUV and RGB formats at various bit depths. Use these to select the appropriate format based on your bandwidth constraints.

#### Format Overview

| Format Type | 8-bit (bpp) | 10-bit (bpp) | 12-bit (bpp) | Typical Use Case |
|-------------|-------------|--------------|--------------|------------------|
| **YUV 4:2:0** | 12 | 15 | 18 | Consumer video, streaming, H.264/H.265 source |
| **YUV 4:2:2** ★ | 16 | **20** | 24 | **Broadcast standard (ST2110-20)** |
| **YUV 4:4:4** | 24 | 30 | 36 | High-end post-production, VFX |
| **RGB** | 24 | 30 | 36 | Graphics, computer-generated content |

★ = SMPTE ST2110-20 standard format

---

#### 1080p60 Bandwidth by Format

| Format | Bit Depth | Bits/Pixel | Raw Data Rate | With Overhead | Compatible NICs |
|--------|-----------|------------|---------------|---------------|-----------------|
| **YUV 4:2:0** | 8-bit | 12 | 1.49 Gbps | **1.79 Gbps** | **I225/I226 (2.5G), All 10G+** |
| YUV 4:2:0 | 10-bit | 15 | 1.87 Gbps | **2.24 Gbps** | **I225/I226 (2.5G), All 10G+** |
| YUV 4:2:0 | 12-bit | 18 | 2.24 Gbps | **2.69 Gbps** | **X710+ (10G+ only)** |
| **YUV 4:2:2** | 8-bit | 16 | 1.99 Gbps | **2.39 Gbps** | **I225/I226 (2.5G - at limit), All 10G+** |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **2.49 Gbps** | **2.98 Gbps** | **X710+ (10G+ only)** |
| YUV 4:2:2 | 12-bit | 24 | 2.99 Gbps | **3.58 Gbps** | **X710+ (10G+ only)** |
| **YUV 4:4:4** | 8-bit | 24 | 2.99 Gbps | **3.58 Gbps** | **X710+ (10G+ only)** |
| YUV 4:4:4 | 10-bit | 30 | 3.73 Gbps | **4.48 Gbps** | **X710+ (10G+ only)** |
| YUV 4:4:4 | 12-bit | 36 | 4.48 Gbps | **5.37 Gbps** | **X710+ (10G+ only)** |
| **RGB** | 8-bit | 24 | 2.99 Gbps | **3.58 Gbps** | **X710+ (10G+ only)** |
| RGB | 10-bit | 30 | 3.73 Gbps | **4.48 Gbps** | **X710+ (10G+ only)** |
| RGB | 12-bit | 36 | 4.48 Gbps | **5.37 Gbps** | **X710+ (10G+ only)** |

★ = ST2110-20 standard  
**Calculation:** 1920×1080 × 60 fps × bits_per_pixel × 1.10 overhead

---

#### 1080p30 Bandwidth by Format

| Format | Bit Depth | Bits/Pixel | Raw Data Rate | With Overhead | Compatible NICs |
|--------|-----------|------------|---------------|---------------|-----------------|
| **YUV 4:2:0** | 8-bit | 12 | 0.75 Gbps | **0.90 Gbps** | **I225/I226 (2.5G), All 10G+** |
| YUV 4:2:0 | 10-bit | 15 | 0.93 Gbps | **1.12 Gbps** | **I225/I226 (2.5G), All 10G+** |
| YUV 4:2:0 | 12-bit | 18 | 1.12 Gbps | **1.34 Gbps** | **I225/I226 (2.5G), All 10G+** |
| **YUV 4:2:2** | 8-bit | 16 | 1.00 Gbps | **1.20 Gbps** | **I225/I226 (2.5G), All 10G+** |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **1.24 Gbps** | **1.50 Gbps** | **I225/I226 (2.5G), All 10G+** |
| YUV 4:2:2 | 12-bit | 24 | 1.49 Gbps | **1.79 Gbps** | **I225/I226 (2.5G), All 10G+** |
| **YUV 4:4:4** | 8-bit | 24 | 1.49 Gbps | **1.79 Gbps** | **I225/I226 (2.5G), All 10G+** |
| YUV 4:4:4 | 10-bit | 30 | 1.87 Gbps | **2.24 Gbps** | **I225/I226 (2.5G), All 10G+** |
| YUV 4:4:4 | 12-bit | 36 | 2.24 Gbps | **2.69 Gbps** | **X710+ (10G+ only)** |
| **RGB** | 8-bit | 24 | 1.49 Gbps | **1.79 Gbps** | **I225/I226 (2.5G), All 10G+** |
| RGB | 10-bit | 30 | 1.87 Gbps | **2.24 Gbps** | **I225/I226 (2.5G), All 10G+** |
| RGB | 12-bit | 36 | 2.24 Gbps | **2.69 Gbps** | **X710+ (10G+ only)** |

★ = ST2110-20 standard  
**Calculation:** 1920×1080 × 30 fps × bits_per_pixel × 1.10 overhead

---

#### 4K60 Bandwidth by Format

| Format | Bit Depth | Bits/Pixel | Raw Data Rate | With Overhead | Compatible NICs |
|--------|-----------|------------|---------------|---------------|-----------------|
| **YUV 4:2:0** | 8-bit | 12 | 5.97 Gbps | **7.16 Gbps** | **X710+ (10G+ only)** |
| YUV 4:2:0 | 10-bit | 15 | 7.46 Gbps | **8.95 Gbps** | **X710+ (10G+ only)** |
| YUV 4:2:0 | 12-bit | 18 | 8.96 Gbps | **10.74 Gbps** | **XXV710+ (25G+ only)** |
| **YUV 4:2:2** | 8-bit | 16 | 7.97 Gbps | **9.55 Gbps** | **X710+ (10G+ only)** |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **9.95 Gbps** | **11.94 Gbps** | **XXV710+ (25G+ only)** |
| YUV 4:2:2 | 12-bit | 24 | 11.94 Gbps | **14.33 Gbps** | **XXV710+ (25G+ only)** |
| **YUV 4:4:4** | 8-bit | 24 | 11.94 Gbps | **14.33 Gbps** | **XXV710+ (25G+ only)** |
| YUV 4:4:4 | 10-bit | 30 | 14.93 Gbps | **17.91 Gbps** | **XXV710+ (25G+ only)** |
| YUV 4:4:4 | 12-bit | 36 | 17.91 Gbps | **21.49 Gbps** | **XXV710+ (25G+ only)** |
| **RGB** | 8-bit | 24 | 11.94 Gbps | **14.33 Gbps** | **XXV710+ (25G+ only)** |
| RGB | 10-bit | 30 | 14.93 Gbps | **17.91 Gbps** | **XXV710+ (25G+ only)** |
| RGB | 12-bit | 36 | 17.91 Gbps | **21.49 Gbps** | **XXV710+ (25G+ only)** |

★ = ST2110-20 standard  
**Calculation:** 3840×2160 × 60 fps × bits_per_pixel × 1.10 overhead

---

#### 4K30 Bandwidth by Format

| Format | Bit Depth | Bits/Pixel | Raw Data Rate | With Overhead | Compatible NICs |
|--------|-----------|------------|---------------|---------------|-----------------|
| **YUV 4:2:0** | 8-bit | 12 | 2.99 Gbps | **3.58 Gbps** | **X710+ (10G+ only)** |
| YUV 4:2:0 | 10-bit | 15 | 3.73 Gbps | **4.48 Gbps** | **X710+ (10G+ only)** |
| YUV 4:2:0 | 12-bit | 18 | 4.48 Gbps | **5.37 Gbps** | **X710+ (10G+ only)** |
| **YUV 4:2:2** | 8-bit | 16 | 3.98 Gbps | **4.78 Gbps** | **X710+ (10G+ only)** |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **4.98 Gbps** | **5.99 Gbps** | **X710+ (10G+ only)** |
| YUV 4:2:2 | 12-bit | 24 | 5.97 Gbps | **7.16 Gbps** | **X710+ (10G+ only)** |
| **YUV 4:4:4** | 8-bit | 24 | 5.97 Gbps | **7.16 Gbps** | **X710+ (10G+ only)** |
| YUV 4:4:4 | 10-bit | 30 | 7.46 Gbps | **8.95 Gbps** | **X710+ (10G+ only)** |
| YUV 4:4:4 | 12-bit | 36 | 8.96 Gbps | **10.74 Gbps** | **XXV710+ (25G+ only)** |
| **RGB** | 8-bit | 24 | 5.97 Gbps | **7.16 Gbps** | **X710+ (10G+ only)** |
| RGB | 10-bit | 30 | 7.46 Gbps | **8.95 Gbps** | **X710+ (10G+ only)** |
| RGB | 12-bit | 36 | 8.96 Gbps | **10.74 Gbps** | **XXV710+ (25G+ only)** |

★ = ST2110-20 standard  
**Calculation:** 3840×2160 × 30 fps × bits_per_pixel × 1.10 overhead

---

#### 8K60 Bandwidth by Format

| Format | Bit Depth | Bits/Pixel | Raw Data Rate | With Overhead | Compatible NICs |
|--------|-----------|------------|---------------|---------------|-----------------|
| **YUV 4:2:0** | 8-bit | 12 | 23.89 Gbps | **28.66 Gbps** | **XL710 (40G), E810 (50G+)** |
| YUV 4:2:0 | 10-bit | 15 | 29.86 Gbps | **35.83 Gbps** | **XL710 (40G), E810 (50G+)** |
| YUV 4:2:0 | 12-bit | 18 | 35.83 Gbps | **42.99 Gbps** | **E810 (50G+)** |
| **YUV 4:2:2** | 8-bit | 16 | 31.85 Gbps | **38.22 Gbps** | **XL710 (40G), E810 (50G+)** |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **39.81 Gbps** | **47.75 Gbps** | **E810 (50G/100G only)** |
| YUV 4:2:2 | 12-bit | 24 | 47.78 Gbps | **57.33 Gbps** | **E810 (100G only)** |
| **YUV 4:4:4** | 8-bit | 24 | 47.78 Gbps | **57.33 Gbps** | **E810 (100G only)** |
| YUV 4:4:4 | 10-bit | 30 | 59.72 Gbps | **71.66 Gbps** | **E810 (100G only)** |
| YUV 4:4:4 | 12-bit | 36 | 71.66 Gbps | **85.99 Gbps** | **E810 (100G only)** |
| **RGB** | 8-bit | 24 | 47.78 Gbps | **57.33 Gbps** | **E810 (100G only)** |
| RGB | 10-bit | 30 | 59.72 Gbps | **71.66 Gbps** | **E810 (100G only)** |
| RGB | 12-bit | 36 | 71.66 Gbps | **85.99 Gbps** | **E810 (100G only)** |

★ = ST2110-20 standard  
**Calculation:** 7680×4320 × 60 fps × bits_per_pixel × 1.10 overhead

---

#### 8K30 Bandwidth by Format

| Format | Bit Depth | Bits/Pixel | Raw Data Rate | With Overhead | Compatible NICs |
|--------|-----------|------------|---------------|---------------|-----------------|
| **YUV 4:2:0** | 8-bit | 12 | 11.94 Gbps | **14.33 Gbps** | **XXV710+ (25G+ only)** |
| YUV 4:2:0 | 10-bit | 15 | 14.93 Gbps | **17.91 Gbps** | **XXV710+ (25G+ only)** |
| YUV 4:2:0 | 12-bit | 18 | 17.91 Gbps | **21.50 Gbps** | **XXV710+ (25G+ only)** |
| **YUV 4:2:2** | 8-bit | 16 | 15.93 Gbps | **19.11 Gbps** | **XXV710+ (25G+ only)** |
| **YUV 4:2:2** ★ | **10-bit** | **20** | **19.91 Gbps** | **23.96 Gbps** | **XXV710 (25G), XL710 (40G), E810** |
| YUV 4:2:2 | 12-bit | 24 | 23.89 Gbps | **28.67 Gbps** | **XL710 (40G), E810 (50G+)** |
| **YUV 4:4:4** | 8-bit | 24 | 23.89 Gbps | **28.67 Gbps** | **XL710 (40G), E810 (50G+)** |
| YUV 4:4:4 | 10-bit | 30 | 29.86 Gbps | **35.83 Gbps** | **XL710 (40G), E810 (50G+)** |
| YUV 4:4:4 | 12-bit | 36 | 35.83 Gbps | **42.99 Gbps** | **E810 (50G+)** |
| **RGB** | 8-bit | 24 | 23.89 Gbps | **28.67 Gbps** | **XL710 (40G), E810 (50G+)** |
| RGB | 10-bit | 30 | 29.86 Gbps | **35.83 Gbps** | **XL710 (40G), E810 (50G+)** |
| RGB | 12-bit | 36 | 35.83 Gbps | **42.99 Gbps** | **E810 (50G+)** |

★ = ST2110-20 standard  
**Calculation:** 7680×4320 × 30 fps × bits_per_pixel × 1.10 overhead

---

### Key Observations

**Bandwidth Savings:**
- **YUV 4:2:0 vs 4:2:2:** ~40% bandwidth reduction (but lower chroma quality)
- **8-bit vs 10-bit:** ~20% bandwidth reduction (but less color depth)
- **4:2:2 vs 4:4:4:** ~50% bandwidth increase (for full chroma resolution)

**Format Selection Guidelines:**
1. **YUV 4:2:0 8-bit:** Consumer streaming, web delivery, bandwidth-constrained links
2. **YUV 4:2:0 10-bit:** HDR distribution, compressed acquisition workflows
3. **YUV 4:2:2 10-bit ★:** Professional broadcast standard (ST2110-20)
4. **YUV 4:4:4 10-bit:** VFX, color grading, high-end post-production
5. **RGB 10-bit:** Computer graphics, gaming, CGI workflows

**2.5G NIC Compatibility:**
- ✅ YUV 4:2:0 formats: All 1080p rates, some 4K rates with 8-bit
- ✅ YUV 4:2:2 8-bit: 1080p up to 60fps
- ✅ YUV 4:2:2 10-bit: 1080p ≤30fps (ST2110 standard)
- ❌ YUV 4:4:4 and RGB 10-bit+: Require 10G+ for most resolutions

---

## Controller Capabilities Matrix

### Maximum Concurrent Stream Support

#### 2.5G/5G Controllers (I225/I226)

**Standard Uncompressed (YUV 4:2:2 10-bit - ST2110-20):**

| Controller | Link Speed | 1080p24 | 1080p25 | 1080p30 | 1080p50 | 1080p60 |
|------------|-----------|---------|---------|---------|---------|---------|
| I225-V | 2.5 Gbps | ✅ 2 | ✅ 2 | ✅ 1 | ⚠️ 1 (at limit) | ❌ |
| I226-V (TSN) | 2.5 Gbps | ✅ 2 | ✅ 2 | ✅ 1 | ⚠️ 1 (at limit) | ❌ |

**Maximum Streams by Format @ 1080p60 (2.5G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams | Notes |
|--------|-----------|-----------|-------------|-------|
| **YUV 4:2:0** | 8-bit | 1.79 Gbps | ✅ **1** | Bandwidth-efficient |
| YUV 4:2:0 | 10-bit | 2.24 Gbps | ✅ **1** | HDR support |
| YUV 4:2:0 | 12-bit | 2.69 Gbps | ❌ **0** | Exceeds capacity |
| **YUV 4:2:2** | 8-bit | 2.39 Gbps | ✅ **1** | Near capacity limit |
| **YUV 4:2:2** ★ | **10-bit** | **2.98 Gbps** | **❌ 0** | **ST2110 standard - requires 10G** |
| YUV 4:2:2 | 12-bit | 3.58 Gbps | ❌ **0** | Requires 10G |
| **YUV 4:4:4** | 8-bit | 3.58 Gbps | ❌ **0** | Requires 10G |
| YUV 4:4:4 | 10-bit | 4.48 Gbps | ❌ **0** | Requires 10G |
| YUV 4:4:4 | 12-bit | 5.37 Gbps | ❌ **0** | Requires 10G |
| **RGB** | 8-bit | 3.58 Gbps | ❌ **0** | Requires 10G |
| RGB | 10-bit | 4.48 Gbps | ❌ **0** | Requires 10G |
| RGB | 12-bit | 5.37 Gbps | ❌ **0** | Requires 10G |

**Maximum Streams by Format @ 1080p30 (2.5G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams | Notes |
|--------|-----------|-----------|-------------|-------|
| **YUV 4:2:0** | 8-bit | 0.90 Gbps | ✅ **2** | Excellent efficiency |
| YUV 4:2:0 | 10-bit | 1.12 Gbps | ✅ **2** | HDR capable |
| YUV 4:2:0 | 12-bit | 1.34 Gbps | ✅ **1** | High quality |
| **YUV 4:2:2** | 8-bit | 1.20 Gbps | ✅ **2** | Good quality |
| **YUV 4:2:2** ★ | **10-bit** | **1.50 Gbps** | **✅ 1** | **ST2110 standard** |
| YUV 4:2:2 | 12-bit | 1.79 Gbps | ✅ **1** | Premium quality |
| **YUV 4:4:4** | 8-bit | 1.79 Gbps | ✅ **1** | Full color |
| YUV 4:4:4 | 10-bit | 2.24 Gbps | ✅ **1** | High-end |
| YUV 4:4:4 | 12-bit | 2.69 Gbps | ❌ **0** | Exceeds capacity |
| **RGB** | 8-bit | 1.79 Gbps | ✅ **1** | Graphics |
| RGB | 10-bit | 2.24 Gbps | ✅ **1** | CGI workflows |
| RGB | 12-bit | 2.69 Gbps | ❌ **0** | Exceeds capacity |

**Compressed ST2110-22 (10:1 JPEGXS):**

| Resolution/FPS | Format | Compressed BW | Max Streams |
|----------------|--------|---------------|-------------|
| 1080p30 | Any format | 0.15 Gbps (avg) | ✅ **16** |
| 1080p60 | Any format | 0.30 Gbps (avg) | ✅ **8** |
| 4K30 | Any format | 0.60 Gbps (avg) | ✅ **4** |
| 4K60 | Any format | 1.19 Gbps (avg) | ✅ **2** |

**Key Takeaways:**
- ✅ **Best for:** YUV 4:2:0 formats at 1080p, compressed workflows
- ✅ **Viable:** YUV 4:2:2 8-bit and 10-bit at 1080p ≤30fps
- ❌ **Not recommended:** 1080p60 standard (YUV 4:2:2 10-bit), 4:4:4 formats uncompressed
- 💡 **Tip:** Use YUV 4:2:0 10-bit for HDR 1080p60 within 2.5G bandwidth

---

#### 10G Controllers (X710)

**Standard Format (YUV 4:2:2 10-bit - ST2110-20):**

| Controller | Model | 1080p60 | 4K30 | 4K60 |
|------------|-------|---------|------|------|
| X710-DA2 | 2×10 Gbps | ✅ 3/port | ✅ 1/port | ❌ |
| X710-DA4 | 4×10 Gbps | ✅ 3/port | ✅ 1/port | ❌ |

**Maximum Streams by Format @ 1080p60 (10G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 1.79 Gbps | ✅ **5** | 90% |
| YUV 4:2:0 | 10-bit | 2.24 Gbps | ✅ **4** | 90% |
| YUV 4:2:0 | 12-bit | 2.69 Gbps | ✅ **3** | 81% |
| **YUV 4:2:2** | 8-bit | 2.39 Gbps | ✅ **4** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **2.98 Gbps** | **✅ 3** | **89%** |
| YUV 4:2:2 | 12-bit | 3.58 Gbps | ✅ **2** | 72% |
| **YUV 4:4:4** | 8-bit | 3.58 Gbps | ✅ **2** | 72% |
| YUV 4:4:4 | 10-bit | 4.48 Gbps | ✅ **2** | 90% |
| YUV 4:4:4 | 12-bit | 5.37 Gbps | ✅ **1** | 54% |
| **RGB** | 8-bit | 3.58 Gbps | ✅ **2** | 72% |
| RGB | 10-bit | 4.48 Gbps | ✅ **2** | 90% |
| RGB | 12-bit | 5.37 Gbps | ✅ **1** | 54% |

**Maximum Streams by Format @ 4K60 (10G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 7.16 Gbps | ✅ **1** | 72% |
| YUV 4:2:0 | 10-bit | 8.95 Gbps | ✅ **1** | 90% |
| YUV 4:2:0 | 12-bit | 10.74 Gbps | ❌ **0** | >100% |
| **YUV 4:2:2** | 8-bit | 9.55 Gbps | ✅ **1** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **11.94 Gbps** | **❌ 0** | **>100% - requires 25G** |
| YUV 4:2:2 | 12-bit | 14.33 Gbps | ❌ **0** | >100% |
| **YUV 4:4:4** | 8-bit | 14.33 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 10-bit | 17.91 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 12-bit | 21.49 Gbps | ❌ **0** | >100% |
| **RGB** | 8-bit | 14.33 Gbps | ❌ **0** | >100% |
| RGB | 10-bit | 17.91 Gbps | ❌ **0** | >100% |
| RGB | 12-bit | 21.49 Gbps | ❌ **0** | >100% |

**Maximum Streams by Format @ 4K30 (10G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 3.58 Gbps | ✅ **2** | 72% |
| YUV 4:2:0 | 10-bit | 4.48 Gbps | ✅ **2** | 90% |
| YUV 4:2:0 | 12-bit | 5.37 Gbps | ✅ **1** | 54% |
| **YUV 4:2:2** | 8-bit | 4.78 Gbps | ✅ **2** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **5.99 Gbps** | **✅ 1** | **60%** |
| YUV 4:2:2 | 12-bit | 7.16 Gbps | ✅ **1** | 72% |
| **YUV 4:4:4** | 8-bit | 7.16 Gbps | ✅ **1** | 72% |
| YUV 4:4:4 | 10-bit | 8.95 Gbps | ✅ **1** | 90% |
| YUV 4:4:4 | 12-bit | 10.74 Gbps | ❌ **0** | >100% |
| **RGB** | 8-bit | 7.16 Gbps | ✅ **1** | 72% |
| RGB | 10-bit | 8.95 Gbps | ✅ **1** | 90% |
| RGB | 12-bit | 10.74 Gbps | ❌ **0** | >100% |

**Compressed ST2110-22 (10:1 JPEGXS):**

| Resolution/FPS | Format | Max Streams/Port |
|----------------|--------|------------------|
| 1080p60 | Any | ✅ **33** |
| 4K30 | Any | ✅ **16** |
| 4K60 | Any | ✅ **8** |

**Key Takeaways:**
- ✅ **Best for:** HD uncompressed production (all formats), 4K30 with YUV 4:2:0/4:2:2
- ✅ **Viable:** 4K60 with YUV 4:2:0 8-bit/10-bit only
- ❌ **Not supported:** 4K60 standard (YUV 4:2:2 10-bit), 4:4:4/RGB at 4K60
- 💡 **Tip:** Use YUV 4:2:0 for 4K60 within 10G bandwidth

---

#### 25G Controllers (XXV710)

**Standard Format (YUV 4:2:2 10-bit - ST2110-20):**

| Controller | Model | 1080p60 | 4K30 | 4K60 | 4K120 |
|------------|-------|---------|------|------|-------|
| XXV710-DA2 | 2×25 Gbps | ✅ 8/port | ✅ 4/port | ✅ 2/port | ❌ |

**Maximum Streams by Format @ 1080p60 (25G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 1.79 Gbps | ✅ **13** | 93% |
| YUV 4:2:0 | 10-bit | 2.24 Gbps | ✅ **11** | 99% |
| YUV 4:2:0 | 12-bit | 2.69 Gbps | ✅ **9** | 97% |
| **YUV 4:2:2** | 8-bit | 2.39 Gbps | ✅ **10** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **2.98 Gbps** | **✅ 8** | **95%** |
| YUV 4:2:2 | 12-bit | 3.58 Gbps | ✅ **6** | 86% |
| **YUV 4:4:4** | 8-bit | 3.58 Gbps | ✅ **6** | 86% |
| YUV 4:4:4 | 10-bit | 4.48 Gbps | ✅ **5** | 90% |
| YUV 4:4:4 | 12-bit | 5.37 Gbps | ✅ **4** | 86% |
| **RGB** | 8-bit | 3.58 Gbps | ✅ **6** | 86% |
| RGB | 10-bit | 4.48 Gbps | ✅ **5** | 90% |
| RGB | 12-bit | 5.37 Gbps | ✅ **4** | 86% |

**Maximum Streams by Format @ 4K60 (25G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 7.16 Gbps | ✅ **3** | 86% |
| YUV 4:2:0 | 10-bit | 8.95 Gbps | ✅ **2** | 72% |
| YUV 4:2:0 | 12-bit | 10.74 Gbps | ✅ **2** | 86% |
| **YUV 4:2:2** | 8-bit | 9.55 Gbps | ✅ **2** | 76% |
| **YUV 4:2:2** ★ | **10-bit** | **11.94 Gbps** | **✅ 2** | **96%** |
| YUV 4:2:2 | 12-bit | 14.33 Gbps | ✅ **1** | 57% |
| **YUV 4:4:4** | 8-bit | 14.33 Gbps | ✅ **1** | 57% |
| YUV 4:4:4 | 10-bit | 17.91 Gbps | ✅ **1** | 72% |
| YUV 4:4:4 | 12-bit | 21.49 Gbps | ✅ **1** | 86% |
| **RGB** | 8-bit | 14.33 Gbps | ✅ **1** | 57% |
| RGB | 10-bit | 17.91 Gbps | ✅ **1** | 72% |
| RGB | 12-bit | 21.49 Gbps | ✅ **1** | 86% |

**Maximum Streams by Format @ 4K120 (25G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 14.33 Gbps | ✅ **1** | 57% |
| YUV 4:2:0 | 10-bit | 17.91 Gbps | ✅ **1** | 72% |
| YUV 4:2:0 | 12-bit | 21.49 Gbps | ✅ **1** | 86% |
| **YUV 4:2:2** | 8-bit | 19.11 Gbps | ✅ **1** | 76% |
| **YUV 4:2:2** ★ | **10-bit** | **23.88 Gbps** | **✅ 1** | **96% - at limit** |
| YUV 4:2:2 | 12-bit | 28.67 Gbps | ❌ **0** | >100% |
| **YUV 4:4:4** | 8-bit | 28.67 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 10-bit | 35.83 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 12-bit | 42.99 Gbps | ❌ **0** | >100% |
| **RGB** | 8-bit | 28.67 Gbps | ❌ **0** | >100% |
| RGB | 10-bit | 35.83 Gbps | ❌ **0** | >100% |
| RGB | 12-bit | 42.99 Gbps | ❌ **0** | >100% |

**Compressed ST2110-22 (10:1 JPEGXS):**

| Resolution/FPS | Format | Max Streams/Port |
|----------------|--------|------------------|
| 1080p60 | Any | ✅ **83** |
| 4K60 | Any | ✅ **21** |
| 4K120 | Any | ✅ **10** |
| 8K30 | Any | ✅ **10** |

**Key Takeaways:**
- ✅ **Best for:** 4K60 uncompressed (standard + all variants), multi-stream HD
- ✅ **Viable:** 4K120 with YUV 4:2:2 10-bit (at 96% capacity), 4:2:0 formats
- ⚠️ **Near limit:** 4K120 standard format uses 96% of bandwidth
- ❌ **Not supported:** 4K120 with 4:4:4 or RGB formats
- 💡 **Sweet spot:** Dual 4K60 streams standard format

---

#### 40G Controllers (XL710)

**Standard Format (YUV 4:2:2 10-bit - ST2110-20):**

| Controller | Model | 1080p60 | 4K60 | 4K120 | 8K30 | 8K60 |
|------------|-------|---------|------|-------|------|------|
| XL710-QDA2 | 2×40 Gbps | ✅ 13/port | ✅ 3/port | ✅ 1/port | ✅ 1/port | ❌ |

**Maximum Streams by Format @ 4K60 (40G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 7.16 Gbps | ✅ **5** | 90% |
| YUV 4:2:0 | 10-bit | 8.95 Gbps | ✅ **4** | 90% |
| YUV 4:2:0 | 12-bit | 10.74 Gbps | ✅ **3** | 81% |
| **YUV 4:2:2** | 8-bit | 9.55 Gbps | ✅ **4** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **11.94 Gbps** | **✅ 3** | **90%** |
| YUV 4:2:2 | 12-bit | 14.33 Gbps | ✅ **2** | 72% |
| **YUV 4:4:4** | 8-bit | 14.33 Gbps | ✅ **2** | 72% |
| YUV 4:4:4 | 10-bit | 17.91 Gbps | ✅ **2** | 90% |
| YUV 4:4:4 | 12-bit | 21.49 Gbps | ✅ **1** | 54% |
| **RGB** | 8-bit | 14.33 Gbps | ✅ **2** | 72% |
| RGB | 10-bit | 17.91 Gbps | ✅ **2** | 90% |
| RGB | 12-bit | 21.49 Gbps | ✅ **1** | 54% |

**Maximum Streams by Format @ 8K30 (40G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 14.33 Gbps | ✅ **2** | 72% |
| YUV 4:2:0 | 10-bit | 17.91 Gbps | ✅ **2** | 90% |
| YUV 4:2:0 | 12-bit | 21.50 Gbps | ✅ **1** | 54% |
| **YUV 4:2:2** | 8-bit | 19.11 Gbps | ✅ **2** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **23.96 Gbps** | **✅ 1** | **60%** |
| YUV 4:2:2 | 12-bit | 28.67 Gbps | ✅ **1** | 72% |
| **YUV 4:4:4** | 8-bit | 28.67 Gbps | ✅ **1** | 72% |
| YUV 4:4:4 | 10-bit | 35.83 Gbps | ✅ **1** | 90% |
| YUV 4:4:4 | 12-bit | 42.99 Gbps | ✅ **1** | 107% ⚠️ |
| **RGB** | 8-bit | 28.67 Gbps | ✅ **1** | 72% |
| RGB | 10-bit | 35.83 Gbps | ✅ **1** | 90% |
| RGB | 12-bit | 42.99 Gbps | ✅ **1** | 107% ⚠️ |

**Maximum Streams by Format @ 8K60 (40G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Notes |
|--------|-----------|-----------|------------------|-------|
| **YUV 4:2:0** | 8-bit | 28.66 Gbps | ✅ **1** | 72% |
| YUV 4:2:0 | 10-bit | 35.83 Gbps | ✅ **1** | 90% |
| YUV 4:2:0 | 12-bit | 42.99 Gbps | ❌ **0** | >100% |
| **YUV 4:2:2** | 8-bit | 38.22 Gbps | ✅ **1** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **47.75 Gbps** | **❌ 0** | **>100% - requires 50G** |
| All other formats | - | >40 Gbps | ❌ **0** | Requires 50G/100G |

**Compressed ST2110-22 (10:1 JPEGXS):**

| Resolution/FPS | Format | Max Streams/Port |
|----------------|--------|------------------|
| 4K60 | Any | ✅ **33** |
| 4K120 | Any | ✅ **16** |
| 8K30 | Any | ✅ **16** |
| 8K60 | Any | ✅ **8** |

**Key Takeaways:**
- ✅ **Best for:** Multi-stream 4K60, 8K30 production, high frame rate
- ✅ **Viable:** 8K60 with YUV 4:2:0 formats only
- ❌ **Not supported:** 8K60 standard format (requires 50G)
- 💡 **Sweet spot:** 3× 4K60 standard streams per port

---

#### 50G Controllers (E810)

**Standard Format (YUV 4:2:2 10-bit - ST2110-20):**

| Controller | Model | 1080p60 | 4K60 | 4K120 | 8K30 | 8K60 |
|------------|-------|---------|------|-------|------|------|
| E810-CQDA2 | 2×50 Gbps | ✅ 16/port | ✅ 4/port | ✅ 2/port | ✅ 2/port | ✅ 1/port |

**Maximum Streams by Format @ 4K60 (50G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 7.16 Gbps | ✅ **6** | 86% |
| YUV 4:2:0 | 10-bit | 8.95 Gbps | ✅ **5** | 90% |
| YUV 4:2:0 | 12-bit | 10.74 Gbps | ✅ **4** | 86% |
| **YUV 4:2:2** | 8-bit | 9.55 Gbps | ✅ **5** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **11.94 Gbps** | **✅ 4** | **96%** |
| YUV 4:2:2 | 12-bit | 14.33 Gbps | ✅ **3** | 86% |
| **YUV 4:4:4** | 8-bit | 14.33 Gbps | ✅ **3** | 86% |
| YUV 4:4:4 | 10-bit | 17.91 Gbps | ✅ **2** | 72% |
| YUV 4:4:4 | 12-bit | 21.49 Gbps | ✅ **2** | 86% |
| **RGB** | 8-bit | 14.33 Gbps | ✅ **3** | 86% |
| RGB | 10-bit | 17.91 Gbps | ✅ **2** | 72% |
| RGB | 12-bit | 21.49 Gbps | ✅ **2** | 86% |

**Maximum Streams by Format @ 8K60 (50G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 28.66 Gbps | ✅ **1** | 57% |
| YUV 4:2:0 | 10-bit | 35.83 Gbps | ✅ **1** | 72% |
| YUV 4:2:0 | 12-bit | 42.99 Gbps | ✅ **1** | 86% |
| **YUV 4:2:2** | 8-bit | 38.22 Gbps | ✅ **1** | 76% |
| **YUV 4:2:2** ★ | **10-bit** | **47.75 Gbps** | **✅ 1** | **96%** |
| YUV 4:2:2 | 12-bit | 57.33 Gbps | ❌ **0** | >100% |
| **YUV 4:4:4** | 8-bit | 57.33 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 10-bit | 71.66 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 12-bit | 85.99 Gbps | ❌ **0** | >100% |
| **RGB** | 8-bit | 57.33 Gbps | ❌ **0** | >100% |
| RGB | 10-bit | 71.66 Gbps | ❌ **0** | >100% |
| RGB | 12-bit | 85.99 Gbps | ❌ **0** | >100% |

**Maximum Streams by Format @ 8K30 (50G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 14.33 Gbps | ✅ **3** | 86% |
| YUV 4:2:0 | 10-bit | 17.91 Gbps | ✅ **2** | 72% |
| YUV 4:2:0 | 12-bit | 21.50 Gbps | ✅ **2** | 86% |
| **YUV 4:2:2** | 8-bit | 19.11 Gbps | ✅ **2** | 76% |
| **YUV 4:2:2** ★ | **10-bit** | **23.96 Gbps** | **✅ 2** | **96%** |
| YUV 4:2:2 | 12-bit | 28.67 Gbps | ✅ **1** | 57% |
| **YUV 4:4:4** | 8-bit | 28.67 Gbps | ✅ **1** | 57% |
| YUV 4:4:4 | 10-bit | 35.83 Gbps | ✅ **1** | 72% |
| YUV 4:4:4 | 12-bit | 42.99 Gbps | ✅ **1** | 86% |
| **RGB** | 8-bit | 28.67 Gbps | ✅ **1** | 57% |
| RGB | 10-bit | 35.83 Gbps | ✅ **1** | 72% |
| RGB | 12-bit | 42.99 Gbps | ✅ **1** | 86% |

**Compressed ST2110-22 (10:1 JPEGXS):**

| Resolution/FPS | Format | Max Streams/Port |
|----------------|--------|------------------|
| 4K60 | Any | ✅ **42** |
| 4K120 | Any | ✅ **20** |
| 8K30 | Any | ✅ **20** |
| 8K60 | Any | ✅ **10** |

**Key Takeaways:**
- ✅ **Best for:** 8K60 standard format, multi-stream 4K60
- ✅ **Viable:** 8K60 with YUV 4:2:0/4:2:2 formats
- ⚠️ **Near limit:** 8K60 standard uses 96% of bandwidth
- ❌ **Not supported:** 8K60 with 4:4:4 or RGB (requires 100G)
- 💡 **Sweet spot:** 4× 4K60 standard streams per port

---

#### 100G Controllers (E810)

**Standard Format (YUV 4:2:2 10-bit - ST2110-20):**

| Controller | Model | 1080p60 | 4K60 | 4K120 | 8K30 | 8K60 | 8K120 |
|------------|-------|---------|------|-------|------|------|-------|
| E810-CQDA2 | 2×100 Gbps | ✅ 33/port | ✅ 8/port | ✅ 4/port | ✅ 4/port | ✅ 2/port | ✅ 1/port |
| E810-2CQDA2 | 2×100 Gbps | ✅ 33/port | ✅ 8/port | ✅ 4/port | ✅ 4/port | ✅ 2/port | ✅ 1/port |

**Maximum Streams by Format @ 4K60 (100G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 7.16 Gbps | ✅ **13** | 93% |
| YUV 4:2:0 | 10-bit | 8.95 Gbps | ✅ **11** | 99% |
| YUV 4:2:0 | 12-bit | 10.74 Gbps | ✅ **9** | 97% |
| **YUV 4:2:2** | 8-bit | 9.55 Gbps | ✅ **10** | 96% |
| **YUV 4:2:2** ★ | **10-bit** | **11.94 Gbps** | **✅ 8** | **96%** |
| YUV 4:2:2 | 12-bit | 14.33 Gbps | ✅ **6** | 86% |
| **YUV 4:4:4** | 8-bit | 14.33 Gbps | ✅ **6** | 86% |
| YUV 4:4:4 | 10-bit | 17.91 Gbps | ✅ **5** | 90% |
| YUV 4:4:4 | 12-bit | 21.49 Gbps | ✅ **4** | 86% |
| **RGB** | 8-bit | 14.33 Gbps | ✅ **6** | 86% |
| RGB | 10-bit | 17.91 Gbps | ✅ **5** | 90% |
| RGB | 12-bit | 21.49 Gbps | ✅ **4** | 86% |

**Maximum Streams by Format @ 8K60 (100G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 28.66 Gbps | ✅ **3** | 86% |
| YUV 4:2:0 | 10-bit | 35.83 Gbps | ✅ **2** | 72% |
| YUV 4:2:0 | 12-bit | 42.99 Gbps | ✅ **2** | 86% |
| **YUV 4:2:2** | 8-bit | 38.22 Gbps | ✅ **2** | 76% |
| **YUV 4:2:2** ★ | **10-bit** | **47.75 Gbps** | **✅ 2** | **96%** |
| YUV 4:2:2 | 12-bit | 57.33 Gbps | ✅ **1** | 57% |
| **YUV 4:4:4** | 8-bit | 57.33 Gbps | ✅ **1** | 57% |
| YUV 4:4:4 | 10-bit | 71.66 Gbps | ✅ **1** | 72% |
| YUV 4:4:4 | 12-bit | 85.99 Gbps | ✅ **1** | 86% |
| **RGB** | 8-bit | 57.33 Gbps | ✅ **1** | 57% |
| RGB | 10-bit | 71.66 Gbps | ✅ **1** | 72% |
| RGB | 12-bit | 85.99 Gbps | ✅ **1** | 86% |

**Maximum Streams by Format @ 8K120 (100G NIC):**

| Format | Bit Depth | Bandwidth | Max Streams/Port | Utilization |
|--------|-----------|-----------|------------------|-------------|
| **YUV 4:2:0** | 8-bit | 57.33 Gbps | ✅ **1** | 57% |
| YUV 4:2:0 | 10-bit | 71.66 Gbps | ✅ **1** | 72% |
| YUV 4:2:0 | 12-bit | 85.99 Gbps | ✅ **1** | 86% |
| **YUV 4:2:2** | 8-bit | 76.44 Gbps | ✅ **1** | 76% |
| **YUV 4:2:2** ★ | **10-bit** | **95.50 Gbps** | **✅ 1** | **96% - at limit** |
| YUV 4:2:2 | 12-bit | 114.66 Gbps | ❌ **0** | >100% |
| **YUV 4:4:4** | 8-bit | 114.66 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 10-bit | 143.32 Gbps | ❌ **0** | >100% |
| YUV 4:4:4 | 12-bit | 171.98 Gbps | ❌ **0** | >100% |
| **RGB** | 8-bit | 114.66 Gbps | ❌ **0** | >100% |
| RGB | 10-bit | 143.32 Gbps | ❌ **0** | >100% |
| RGB | 12-bit | 171.98 Gbps | ❌ **0** | >100% |

**Compressed ST2110-22 (10:1 JPEGXS):**

| Resolution/FPS | Format | Max Streams/Port |
|----------------|--------|------------------|
| 4K60 | Any | ✅ **84** |
| 4K120 | Any | ✅ **41** |
| 8K60 | Any | ✅ **20** |
| 8K120 | Any | ✅ **10** |

**Key Takeaways:**
- ✅ **Best for:** Maximum density, 8K120 standard format, multiple 8K streams
- ✅ **Viable:** All formats up to 8K60, 8K120 with YUV 4:2:0/4:2:2
- ⚠️ **Near limit:** 8K120 standard uses 96% of bandwidth
- ❌ **Limited:** 8K120 with 4:4:4/RGB exceeds capacity
- 💡 **Ultimate:** 8× 4K60 standard or 2× 8K60 standard streams

---

## Video Stream Support Tables

### Detailed Stream Counts by Resolution and FPS

#### Uncompressed ST2110-20 (Standard: YUV 4:2:2 10-bit)

| Resolution | FPS | Bandwidth/Stream | 2.5G | 10G | 25G | 40G | 50G | 100G |
|------------|-----|------------------|------|-----|-----|-----|-----|------|
| **1080p** | 23.98 | 1.20 Gbps | **2** | 8 | 20 | 33 | 41 | 83 |
| 1080p | 25 | 1.25 Gbps | **2** | 8 | 20 | 32 | 40 | 80 |
| 1080p | 29.97 | 1.50 Gbps | **1** | 6 | 16 | 26 | 33 | 66 |
| 1080p | 30 | 1.50 Gbps | **1** | 6 | 16 | 26 | 33 | 66 |
| 1080p | 50 | 2.49 Gbps | **1** | 4 | 10 | 16 | 20 | 40 |
| **1080p** | **59.94** | **2.97 Gbps** | **❌** | **3** | **8** | **13** | **16** | **33** |
| **1080p** | **60** | **2.98 Gbps** | **❌** | **3** | **8** | **13** | **16** | **33** |
| 1080i | 50i | 1.24 Gbps | **2** | 8 | 20 | 32 | 40 | 80 |
| 1080i | 59.94i | 1.49 Gbps | **1** | 6 | 16 | 26 | 33 | 66 |
| **4K UHD** | 23.98 | 4.79 Gbps | ❌ | 2 | 5 | 8 | 10 | 20 |
| 4K UHD | 25 | 4.99 Gbps | ❌ | 2 | 5 | 8 | 10 | 20 |
| **4K UHD** | **29.97** | **5.96 Gbps** | **❌** | **1** | **4** | **6** | **8** | **16** |
| **4K UHD** | **30** | **5.99 Gbps** | **❌** | **1** | **4** | **6** | **8** | **16** |
| 4K UHD | 50 | 9.98 Gbps | ❌ | 1 | 2 | 4 | 5 | 10 |
| **4K UHD** | **59.94** | **11.89 Gbps** | **❌** | **❌** | **2** | **3** | **4** | **8** |
| **4K UHD** | **60** | **11.94 Gbps** | **❌** | **❌** | **2** | **3** | **4** | **8** |
| 4K UHD | 100 | 19.97 Gbps | ❌ | ❌ | 1 | 2 | 2 | 5 |
| 4K UHD | 120 | 23.88 Gbps | ❌ | ❌ | 1 | 1 | 2 | 4 |
| **8K UHD** | 25 | 19.97 Gbps | ❌ | ❌ | 1 | 2 | 2 | 5 |
| 8K UHD | 30 | 23.96 Gbps | ❌ | ❌ | 1 | 1 | 2 | 4 |
| 8K UHD | 50 | 39.93 Gbps | ❌ | ❌ | ❌ | 1 | 1 | 2 |
| **8K UHD** | **60** | **47.75 Gbps** | **❌** | **❌** | **❌** | **❌** | **1** | **2** |

**Legend:**
- ✅ = Supported
- ❌ = Not supported (insufficient bandwidth)
- Numbers = Maximum concurrent streams per port

---

#### Compressed ST2110-22 (JPEGXS 10:1 compression)

| Resolution | FPS | Compressed BW | 2.5G | 10G | 25G | 40G | 50G | 100G |
|------------|-----|---------------|------|-----|-----|-----|-----|------|
| 1080p | 23.98 | 0.12 Gbps | 20 | 83 | 208 | 333 | 416 | 833 |
| 1080p | 25 | 0.13 Gbps | 19 | 76 | 192 | 307 | 384 | 769 |
| **1080p** | **30** | **0.15 Gbps** | **16** | **66** | **166** | **266** | **333** | **666** |
| 1080p | 50 | 0.25 Gbps | 10 | 40 | 100 | 160 | 200 | 400 |
| **1080p** | **60** | **0.30 Gbps** | **8** | **33** | **83** | **133** | **166** | **333** |
| **4K UHD** | **23.98** | **0.48 Gbps** | **5** | **20** | **52** | **83** | **104** | **208** |
| 4K UHD | 25 | 0.50 Gbps | 5 | 20 | 50 | 80 | 100 | 200 |
| **4K UHD** | **30** | **0.60 Gbps** | **4** | **16** | **41** | **66** | **83** | **166** |
| 4K UHD | 50 | 1.00 Gbps | 2 | 10 | 25 | 40 | 50 | 100 |
| **4K UHD** | **60** | **1.19 Gbps** | **2** | **8** | **21** | **33** | **42** | **84** |
| 4K UHD | 100 | 2.00 Gbps | 1 | 5 | 12 | 20 | 25 | 50 |
| **4K UHD** | **120** | **2.39 Gbps** | **1** | **4** | **10** | **16** | **20** | **41** |
| 8K UHD | 25 | 2.00 Gbps | 1 | 5 | 12 | 20 | 25 | 50 |
| **8K UHD** | **30** | **2.40 Gbps** | **1** | **4** | **10** | **16** | **20** | **41** |
| 8K UHD | 50 | 3.99 Gbps | ❌ | 2 | 6 | 10 | 12 | 25 |
| **8K UHD** | **60** | **4.78 Gbps** | **❌** | **2** | **5** | **8** | **10** | **20** |

**Notes:**
- Compression ratio: 10:1 (typical JPEGXS broadcast quality)
- For 6:1 compression, multiply stream counts by 0.6
- For 15:1 compression, multiply stream counts by 1.5
- H.264/H.265 can achieve higher compression but adds latency

---

## Recommended Configurations

### Configuration 1: Entry-Level - Limited Uncompressed + Compressed (2.5G)

**Hardware:** Intel I225-V or I226-V (with TSN)  
**Link Speed:** 2.5 Gbps  
**Use Case:** Select uncompressed formats at 1080p, compressed workflows, and monitoring

**Supported Uncompressed Streams:**

**@ 1080p60:**
- ✅ 1× YUV 4:2:0 8-bit (1.79 Gbps) - Bandwidth-efficient
- ✅ 1× YUV 4:2:0 10-bit (2.24 Gbps) - HDR support
- ✅ 1× YUV 4:2:2 8-bit (2.39 Gbps) - Near capacity
- ❌ 0× YUV 4:2:2 10-bit (2.98 Gbps) - **Exceeds 2.5G, requires 10G**

**@ 1080p30:**
- ✅ 2× YUV 4:2:0 8-bit (0.90 Gbps each)
- ✅ 2× YUV 4:2:0 10-bit (1.12 Gbps each) - HDR capable
- ✅ 2× YUV 4:2:2 8-bit (1.20 Gbps each)
- ✅ 1× YUV 4:2:2 10-bit (1.50 Gbps) - **ST2110 standard**
- ✅ 1× YUV 4:2:2 12-bit (1.79 Gbps) - Premium quality
- ✅ 1× YUV 4:4:4 8-bit (1.79 Gbps) - Full color
- ✅ 1× YUV 4:4:4 10-bit (2.24 Gbps) - High-end
- ✅ 1× RGB 8-bit (1.79 Gbps) - Graphics
- ✅ 1× RGB 10-bit (2.24 Gbps) - CGI workflows

**Supported Compressed Streams (JPEGXS 10:1):**
- ✅ 16× 1080p30 compressed
- ✅ 8× 1080p60 compressed
- ✅ 4× 4K30 compressed
- ✅ 2× 4K60 compressed
- ✅ Multiple ST2110-30 audio streams

**Best For:**
- **Uncompressed:** YUV 4:2:0 workflows at 1080p (bandwidth-efficient HDR)
- **Uncompressed:** 1080p30 with YUV 4:2:2 10-bit standard format (1 stream)
- **Uncompressed:** Lower frame rate production (24/25/30 fps) with standard formats
- **Compressed:** Content distribution networks and monitoring
- **Compressed:** Editing workstations (proxy workflows)
- **Compressed:** Remote production contribution feeds

**Key Limitation:** Cannot support YUV 4:2:2 10-bit (ST2110 standard) @ 1080p60 uncompressed

**Recommended Format Selection:**
- Use **YUV 4:2:0 10-bit** for HDR 1080p60 within 2.5G bandwidth
- Use **YUV 4:2:2 10-bit** for ST2110 standard compliance at ≤30fps
- Use **Compressed (JPEGXS 10:1)** for higher resolutions or frame rates

---

### Configuration 2: HD Uncompressed Production (10G)

**Hardware:** Intel X710-DA2/DA4  
**Link Speed:** 10 Gbps per port  
**Use Case:** 1080p uncompressed production environments

**Supported Streams per Port:**
- ✅ 3× 1080p60 uncompressed
- ✅ 1× 4K30 uncompressed
- ✅ 33× 1080p60 compressed (JPEGXS 10:1)
- ✅ 16× 4K30 compressed (JPEGXS 10:1)
- ✅ 8× 4K60 compressed (JPEGXS 10:1)

**Best For:**
- HD broadcast studios
- Live sports production (1080p)
- News production
- Multi-camera HD workflows

**Advantages:**
- Proven technology
- Cost-effective
- Sufficient for HD uncompressed
- Mature DPDK support

---

### Configuration 3: 4K Production Standard (25G)

**Hardware:** Intel XXV710-DA2 or E810-XXVDA2  
**Link Speed:** 25 Gbps per port  
**Use Case:** 4K uncompressed and mixed HD/4K production

**Supported Streams per Port:**
- ✅ 8× 1080p60 uncompressed
- ✅ 4× 4K30 uncompressed
- ✅ 2× 4K60 uncompressed
- ✅ 83× 1080p60 compressed
- ✅ 41× 4K30 compressed
- ✅ 21× 4K60 compressed

**Best For:**
- 4K broadcast studios
- Live events (4K production)
- High-end post-production
- Virtual production stages

**Advantages:**
- Sweet spot for 4K workflows
- Good stream density
- ST2110 DDP support (E810)
- Future-proof for most applications

**Recommended:** **E810-XXVDA2** for latest features and DDP support

---

### Configuration 4: High-Density 4K/8K (40G-50G)

**Hardware:** Intel XL710-QDA2 (40G) or E810-CQDA2 (50G/100G)  
**Link Speed:** 40-50 Gbps per port  
**Use Case:** High-density 4K and entry-level 8K workflows

**Supported Streams per Port (50G):**
- ✅ 16× 1080p60 uncompressed
- ✅ 4× 4K60 uncompressed
- ✅ 2× 4K120 uncompressed
- ✅ 2× 8K30 uncompressed
- ✅ 1× 8K60 uncompressed
- ✅ 166× 1080p60 compressed
- ✅ 42× 4K60 compressed
- ✅ 10× 8K60 compressed

**Best For:**
- Multi-stream 4K production
- High frame rate workflows
- 8K production (limited streams)
- Content aggregation points

**Recommended:** **E810-CQDA2** configured for 50G operation

---

### Configuration 5: Maximum Density 8K Production (100G)

**Hardware:** Intel E810-CQDA2 or E810-2CQDA2  
**Link Speed:** 100 Gbps per port  
**Use Case:** Ultra-high-density production, 8K workflows, network aggregation

**Supported Streams per Port:**
- ✅ 33× 1080p60 uncompressed
- ✅ 8× 4K60 uncompressed
- ✅ 4× 4K120 uncompressed
- ✅ 4× 8K30 uncompressed
- ✅ 2× 8K60 uncompressed
- ✅ 1× 8K120 uncompressed
- ✅ 333× 1080p60 compressed
- ✅ 84× 4K60 compressed
- ✅ 20× 8K60 compressed

**Best For:**
- Large-scale broadcast facilities
- 8K production workflows
- Network spine/aggregation layer
- Future-proof infrastructure
- Cloud production gateways

**Recommended:** **E810-2CQDA2** (PCIe 4.0) for maximum performance

---

## Special Features

### Intel E810 Advanced Features for ST2110

#### 1. Dynamic Device Personalization (DDP)

**What it does:** Hardware-accelerated packet classification for ST2110 flows

**Benefits:**
- Automatic flow steering to queues
- Reduced CPU overhead
- Better multicast handling
- Optimized for SMPTE ST2110 packet patterns

**How to enable:**
```bash
# Load ST2110-specific DDP profile
ddptool -i eth0 -load st2110_profile.pkg
```

**Performance Impact:**
- 20-30% CPU reduction for packet processing
- Better latency consistency
- Supports more concurrent streams

---

#### 2. Application Device Queues (ADQ)

**What it does:** Dedicated queue pairs for applications

**Benefits:**
- Application-level traffic isolation
- Better QoS control
- Reduced latency variance

**Use Case:**
```
Queue 0-7:   ST2110-20 video streams (high priority)
Queue 8-15:  ST2110-30 audio streams (medium priority)
Queue 16-23: Control traffic (low priority)
```

---

#### 3. Hardware Timestamping (PTP)

**What it does:** IEEE 1588 Precision Time Protocol in hardware

**Benefits:**
- Sub-microsecond timing accuracy
- Essential for ST2110 synchronization
- SMPTE ST2059 compliance

**Accuracy:**
- E810: ±20 nanoseconds
- X710/XL710: ±50 nanoseconds

---

#### 4. RSS (Receive Side Scaling)

**What it does:** Distributes packets across CPU cores

**Benefits:**
- Better multi-core utilization
- Increased throughput
- Reduced per-core CPU usage

**Optimization for ST2110:**
```bash
# Configure RSS for video flow distribution
ethtool -X eth0 equal 8  # Distribute to 8 queues
```

---

#### 5. Flow Director (FDIR)

**What it does:** Hardware-based packet filtering and steering

**Benefits:**
- Direct packets to specific queues
- Supports up to 8K-64K rules (model dependent)
- Zero CPU overhead for classification

**Example ST2110 Filter:**
```c
// Steer all packets from 239.1.1.1:20000 to queue 0
struct rte_eth_fdir_filter filter;
filter.ip_dst = 0xEF010101;  // 239.1.1.1
filter.udp_dst = 20000;
filter.queue = 0;
```

---

### Power Consumption Comparison

| Controller | Idle Power | Max Power | Typical (50% load) |
|------------|-----------|-----------|-------------------|
| I225-V | 0.5W | 2W | 1W |
| I226-V | 0.5W | 2W | 1W |
| X710-DA2 | 8W | 15W | 12W |
| XXV710-DA2 | 10W | 20W | 15W |
| XL710-QDA2 | 12W | 25W | 18W |
| E810-XXVDA2 | 12W | 22W | 17W |
| E810-CQDA2 | 18W | 35W | 27W |
| E810-2CQDA2 | 20W | 40W | 30W |

---

## Selection Guide

### Decision Tree

```
1. What is your video format?
   ├─ Uncompressed (ST2110-20) ──→ Go to Q2
   └─ Compressed (ST2110-22) ──→ Consider 2.5G-10G NICs (all resolutions supported)

2. What is your maximum resolution and frame rate?
   ├─ 1080p60 YUV 4:2:0 10-bit (HDR) ──→ 2.5G sufficient (I225/I226) ✅ Budget option
   ├─ 1080p30 YUV 4:2:2 10-bit ★ ──→ 2.5G sufficient (I225/I226) ✅ Budget option
   ├─ 1080p60 YUV 4:2:2 10-bit ★ ──→ 10G minimum (X710) - Standard format
   ├─ 1080p/1080i (all formats) ──→ 10G sufficient (X710)
   ├─ 4K @ 30fps ─────────────→ 10G minimum (X710), 25G recommended (XXV710/E810)
   ├─ 4K @ 60fps ─────────────→ 25G minimum (XXV710/E810)
   ├─ 4K @ 120fps ────────────→ 40G minimum (XL710), 50G recommended (E810)
   ├─ 8K @ 30fps ─────────────→ 25G minimum, 40G recommended
   ├─ 8K @ 60fps ─────────────→ 50G minimum (E810)
   └─ 8K @ 120fps ────────────→ 100G required (E810)

★ = ST2110-20 standard format (YUV 4:2:2 10-bit)

3. How many concurrent streams?
   ├─ 1 stream (budget) ───────→ Match format to bandwidth:
   │                              • 1080p60 4:2:0: 2.5G NIC ✅
   │                              • 1080p30 4:2:2 ★: 2.5G NIC ✅
   │                              • 1080p60 4:2:2 ★: 10G NIC minimum
   ├─ 1-3 streams ─────────────→ Match bandwidth to total streams
   ├─ 4-10 streams ────────────→ Add 50% overhead for port capacity
   └─ 10+ streams ─────────────→ Consider higher speed or multiple ports

4. Do you need advanced features?
   ├─ PTP synchronization ─────→ All Intel NICs support PTP
   ├─ TSN timing (budget) ─────→ I226-V (2.5G with TSN)
   ├─ ST2110 DDP profile ──────→ E810 series required
   ├─ Lowest latency ──────────→ E810 series (sub-microsecond)
   └─ Cost-sensitive ──────────→ I225/I226 (2.5G) or X710/XXV710 for proven performance
```

---

### Use Case Recommendations

#### News Production (HD/1080p Focus)

**Recommended:** Intel X710-DA4 (Quad 10G)  
**Why:**
- 3× 1080p60 uncompressed per port = 12 cameras total
- Cost-effective
- Proven reliability
- Sufficient for HD workflows

**Alternative:** XXV710-DA2 (if 4K future-proofing needed)

---

#### Live Sports Production (4K Required)

**Recommended:** Intel E810-XXVDA4 (Quad 25G) or E810-CQDA2 (Dual 100G)  
**Why:**
- E810-XXVDA4: 2× 4K60 per port = 8 cameras total
- E810-CQDA2: 8× 4K60 per port = 16 cameras total
- DDP profile for ST2110 optimization
- Hardware PTP for frame sync
- Low latency for replay systems

---

#### Post-Production Facility (4K/8K)

**Recommended:** Intel E810-CQDA2 (Dual 100G)  
**Why:**
- Multiple 4K60 streams simultaneously
- 8K30/8K60 support for high-end work
- High bandwidth for large file transfers
- Future-proof for 8K workflows

---

#### Remote/Cloud Production

**Recommended:** Intel E810-2CQDA2 (Dual 100G, PCIe 4.0)  
**Why:**
- Maximum stream density
- Aggregation of multiple sources
- WAN gateway capability
- Highest throughput for distributed production

---

#### Monitoring/Distribution & Budget Uncompressed

**Recommended:** Intel I226-V (2.5G with TSN) or I225-V  
**Why:**
- Low cost per endpoint ($30-50)
- **Uncompressed:** Supports YUV 4:2:0 8/10-bit @ 1080p60 (1 stream)
- **Uncompressed:** Supports YUV 4:2:2 10-bit @ 1080p30 (ST2110 standard, 1 stream)
- **Compressed:** Sufficient for multiple compressed streams
- I226-V adds TSN support for precise timing
- Low power consumption (1-2W typical)

**Uncompressed Capabilities:**
- 1080p60 YUV 4:2:0 10-bit (HDR, 1 stream)
- 1080p30 YUV 4:2:2 10-bit (Standard, 1 stream)
- 1080p30 YUV 4:4:4 10-bit (Full color, 1 stream)

**Compressed Capabilities:**
- 8× 1080p60 (JPEGXS 10:1)
- 2× 4K60 (JPEGXS 10:1)

---

### Budget Considerations

| Budget Level | Resolution Target | Format Support | Recommended Controller | Approx. Price |
|-------------|------------------|----------------|------------------------|---------------|
| **Entry** | 1080p60 uncompressed | YUV 4:2:0 8/10-bit (1 stream) | I225-V | $30-50 |
| **Entry** | 1080p30 uncompressed | YUV 4:2:2 10-bit ★ (1 stream) | I225-V | $30-50 |
| **Entry** | 1080p60 compressed | All formats (8+ streams) | I225-V | $30-50 |
| **Standard** | 1080p60 uncompressed | YUV 4:2:2 10-bit ★ (3 streams) | X710-DA2 | $300-500 |
| **Professional** | 4K30/4K60 | YUV 4:2:2 10-bit ★ | XXV710-DA2 | $600-900 |
| **High-End** | 4K60+ / Multi-stream | YUV 4:2:2 10-bit ★ + 4:4:4 | E810-XXVDA2/4 | $1000-1500 |
| **Premium** | 8K / Ultra-density | All formats | E810-CQDA2 | $2000-3000 |
| **Enterprise** | Maximum capacity | All formats | E810-2CQDA2 | $3000-4000 |

★ = ST2110-20 standard format (YUV 4:2:2 10-bit)

**2.5G NIC Format Guide:**
- **YUV 4:2:0 8-bit @ 1080p60:** Best bandwidth efficiency (1.79 Gbps)
- **YUV 4:2:0 10-bit @ 1080p60:** HDR support within 2.5G (2.24 Gbps)
- **YUV 4:2:2 8-bit @ 1080p60:** Near-limit operation (2.39 Gbps, 96% utilization)
- **YUV 4:2:2 10-bit @ 1080p30:** ST2110 standard compliance (1.50 Gbps)
- **YUV 4:4:4 10-bit @ 1080p30:** Full chroma for post-production (2.24 Gbps)

---

## Performance Optimization Tips

### 1. NUMA Awareness

**Best Practice:** Match NIC and CPU on same NUMA node

```bash
# Check NIC NUMA node
cat /sys/class/net/eth0/device/numa_node
# Output: 0

# Pin application to same NUMA node
numactl --cpunodebind=0 --membind=0 ./my_mtl_app
```

**Performance Impact:** 15-30% throughput improvement

---

### 2. CPU Core Allocation

**Recommended Core Count by Link Speed:**

| Link Speed | DPDK Lcores | Processing Threads | Total Cores |
|-----------|-------------|-------------------|-------------|
| 2.5G | 1 | 1-2 | 2-3 |
| 10G | 1-2 | 2-4 | 4-6 |
| 25G | 2-3 | 4-6 | 6-9 |
| 40G-50G | 3-4 | 6-8 | 9-12 |
| 100G | 4-6 | 8-16 | 12-22 |

---

### 3. Huge Pages Configuration

**Recommended Settings:**

```bash
# For 2.5G workloads (light uncompressed or compressed streams)
echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# For 10G workloads
echo 2048 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# For 25G+ workloads
echo 4096 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# For 100G workloads
echo 8192 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```

---

### 4. RX/TX Queue Configuration

**Best Practice:** Match queue count to lcore count

```bash
# For 4 lcores
--nb-rxq 4 --nb-txq 4

# Enable RSS for load balancing
--rss-ip
```

---

### 5. Descriptor Ring Size

**Recommended Settings:**

| Link Speed | RX Descriptors | TX Descriptors |
|-----------|---------------|---------------|
| 2.5G-10G | 1024 | 512 |
| 25G | 2048 | 1024 |
| 40G-50G | 4096 | 2048 |
| 100G | 8192 | 4096 |

---

## Troubleshooting Common Issues

### Issue 1: Packet Drops at High Bandwidth

**Symptoms:**
- Incomplete frames reported by MTL
- Stats show RX drops

**Solutions:**
1. Increase RX descriptor count
2. Add more lcores for packet processing
3. Enable RSS across more queues
4. Check for NUMA mismatch (see tip #1)
5. Verify CPU governor set to "performance"

```bash
# Set CPU to performance mode
cpupower frequency-set -g performance
```

---

### Issue 2: High CPU Usage

**Symptoms:**
- CPU at 100% on packet processing cores
- Throughput below line rate

**Solutions:**
1. Enable DDP profile (E810 only)
2. Increase burst size in DPDK
3. Enable hardware offloads
4. Distribute load with RSS/Flow Director

```bash
# Enable hardware offloads
ethtool -K eth0 rxhash on ntuple on
```

---

### Issue 3: Timing Drift (PTP Issues)

**Symptoms:**
- Frame timing inconsistent
- SMPTE 2110-21 compliance failures

**Solutions:**
1. Verify PTP daemon running correctly
2. Check PHC (PTP Hardware Clock) sync
3. Use E810 for best PTP performance
4. Reduce system load during critical timing

```bash
# Check PTP sync status
pmc -u -b 0 'GET CURRENT_DATA_SET'
```

---

### Issue 4: Lower Than Expected Throughput

**Symptoms:**
- Cannot achieve expected stream count
- Bandwidth lower than link speed

**Solutions:**
1. Check for link auto-negotiation issues
2. Verify cable quality (especially for 25G+)
3. Test with direct connection (no switches)
4. Monitor for interface errors

```bash
# Check interface counters
ethtool -S eth0 | grep -i error
```

---

## Future-Proofing Recommendations

### Short-term (2026-2027)

**Minimum Recommendation:** 25G (E810-XXVDA2)

**Rationale:**
- 4K60 becoming standard in broadcast
- Multiple stream requirements growing
- ST2110 adoption accelerating

---

### Medium-term (2027-2029)

**Minimum Recommendation:** 50G-100G (E810-CQDA2)

**Rationale:**
- 4K120 high frame rate workflows
- 8K production increasing
- Multi-stream density requirements
- IP core infrastructure buildout

---

### Long-term (2030+)

**Minimum Recommendation:** 100G+ (E810-2CQDA2 or next-gen)

**Rationale:**
- 8K60/8K120 standard workflows
- 16K experimental content
- Uncompressed HDR with expanded color space
- Software-defined production growth

---

## Conclusion

### Quick Selection Guide Summary

| Your Primary Need | Format Details | Recommended Controller | Link Speed |
|------------------|----------------|------------------------|------------|
| **1080p60 uncompressed (budget)** | **YUV 4:2:0 10-bit (HDR)** | **Intel I225/I226-V** | **2.5G** |
| **1080p30 uncompressed (budget)** | **YUV 4:2:2 10-bit ★ (standard)** | **Intel I225/I226-V** | **2.5G** |
| Compressed video (all resolutions) | All formats with JPEGXS | Intel I225/I226-V | 2.5G |
| **1080p60 uncompressed (standard)** | **YUV 4:2:2 10-bit ★** | **Intel X710-DA2** | **10G** |
| 4K30 uncompressed | YUV 4:2:2 10-bit ★ | Intel X710/XXV710 | 10G-25G |
| **4K60 uncompressed (most common)** | **YUV 4:2:2 10-bit ★** | **Intel E810-XXVDA2** | **25G** |
| 4K60 high-end formats | YUV 4:4:4 10-bit, RGB 10-bit | Intel E810-XXVDA2 | 25G |
| 4K120 / Multi-4K60 | YUV 4:2:2 10-bit ★ | Intel E810-CQDA2 | 50G-100G |
| 8K30/8K60 production | YUV 4:2:2 10-bit ★ | Intel E810-CQDA2 | 50G-100G |
| 8K high-end formats | YUV 4:4:4, RGB (limited) | Intel E810-CQDA2 | 100G |
| Maximum density / Future-proof | All formats | Intel E810-2CQDA2 | 100G |

★ = ST2110-20 standard format (YUV 4:2:2 10-bit = 20 bits per pixel)

---

### Key Takeaways

1. **2.5G has limited uncompressed support:**
   - ✅ 1080p60 YUV 4:2:0 10-bit (HDR, 1 stream)
   - ✅ 1080p30 YUV 4:2:2 10-bit (ST2110 standard, 1 stream)
   - ❌ 1080p60 YUV 4:2:2 10-bit (2.98 Gbps exceeds 2.5G)
   - 💡 Use YUV 4:2:0 10-bit for HDR 1080p60 within 2.5G budget

2. **10G is sufficient for HD (1080p) standard format** (YUV 4:2:2 10-bit, 3 streams) but not 4K60 uncompressed

3. **25G is the sweet spot for 4K60** uncompressed production (YUV 4:2:2 10-bit, 2 streams)

4. **100G is required for 8K60+** or ultra-high stream density

5. **Compression changes everything:** 2.5G can handle 4K60 with JPEGXS 10:1 (2 streams)

6. **Format selection impacts bandwidth:**
   - YUV 4:2:0 saves ~40% vs 4:2:2 (lower chroma quality)
   - 8-bit saves ~20% vs 10-bit (less color depth)
   - YUV 4:4:4 uses ~50% more than 4:2:2 (full chroma)

7. **E810 series is recommended** for new deployments due to DDP and latest features

8. **Always plan for 70-80% link utilization** to allow for overhead and jitter

---

**Document Version:** 1.0  
**Last Updated:** February 13, 2026  
**Author:** Media Transport Library Study Guide Series  
**License:** BSD-3-Clause  
**Related Documents:**
- [MTL Pipeline Architecture Guide](mtl_pipeline_architecture_guide.md)
- [MTL Pipeline API Reference](mtl_pipeline_api_reference.md)
