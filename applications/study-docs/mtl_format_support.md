# MTL Format Support Matrix

Format support across FFmpeg plugin, TxApp, and RxApp for all YUV and RGB frame formats
defined in `include/st_pipeline_api.h`.

---

## YUV Formats

| MTL Format Name | Subsampling | Bit Depth | Packing | FFmpeg TX | FFmpeg TX Pixel Format | FFmpeg RX | FFmpeg RX Pixel Format | TxApp | RxApp | RxApp Display | Wire Format |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `ST_FRAME_FMT_YUV422PLANAR10LE` | 4:2:2 | 10-bit | Planar | ✅ | `yuv422p10le` | ✅ | `yuv422p10le` | ✅ default | ✅ | Full color | `ST20_FMT_YUV_422_10BIT` |
| `ST_FRAME_FMT_V210` | 4:2:2 | 10-bit | Packed (3 samples/32-bit) | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_422_10BIT` |
| `ST_FRAME_FMT_Y210` | 4:2:2 | 10-bit | Packed (16-bit/sample) | ⚠️ workaround | `y210le` | ⚠️ workaround | `y210le` | ❌ | ❌ | — | `ST20_FMT_YUV_422_10BIT` |
| `ST_FRAME_FMT_YUV422PLANAR8` | 4:2:2 | 8-bit | Planar | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_422_8BIT` |
| `ST_FRAME_FMT_UYVY` | 4:2:2 | 8-bit | Packed | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_422_8BIT` |
| `ST_FRAME_FMT_YUV422RFC4175PG2BE10` | 4:2:2 | 10-bit | RFC4175 PG2 BE | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_422_10BIT` |
| `ST_FRAME_FMT_YUV422PLANAR12LE` | 4:2:2 | 12-bit | Planar | ❌ | — | ❌ | — | ✅ | ✅ | Full color | `ST20_FMT_YUV_422_12BIT` |
| `ST_FRAME_FMT_YUV422RFC4175PG2BE12` | 4:2:2 | 12-bit | RFC4175 PG2 BE | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_422_12BIT` |
| `ST_FRAME_FMT_YUV444PLANAR10LE` | 4:4:4 | 10-bit | Planar | ❌ | — | ❌ | — | ✅ | ✅ | Full color | `ST20_FMT_YUV_444_10BIT` |
| `ST_FRAME_FMT_YUV444RFC4175PG4BE10` | 4:4:4 | 10-bit | RFC4175 PG4 BE | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_444_10BIT` |
| `ST_FRAME_FMT_YUV444PLANAR12LE` | 4:4:4 | 12-bit | Planar | ❌ | — | ❌ | — | ✅ | ✅ | Full color | `ST20_FMT_YUV_444_12BIT` |
| `ST_FRAME_FMT_YUV444RFC4175PG2BE12` | 4:4:4 | 12-bit | RFC4175 PG2 BE | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_444_12BIT` |
| `ST_FRAME_FMT_YUV420CUSTOM8` | 4:2:0 | 8-bit | Custom pass-through | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_420_8BIT` |
| `ST_FRAME_FMT_YUV422CUSTOM8` | 4:2:2 | 8-bit | Custom pass-through | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_422_8BIT` |
| `ST_FRAME_FMT_YUV420PLANAR8` | 4:2:0 | 8-bit | Planar | ❌ | — | ❌ | — | ✅ `yuv420p` | ✅ | Y-only (grayscale) | `ST20_FMT_YUV_420_8BIT` |
| `ST_FRAME_FMT_YUV422PLANAR16LE` | 4:2:2 | 10-bit (6-bit LS pad) | Planar 16-bit word | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_YUV_422_10BIT` |

---

## RGB Formats

| MTL Format Name | Subsampling | Bit Depth | Packing | FFmpeg TX | FFmpeg TX Pixel Format | FFmpeg RX | FFmpeg RX Pixel Format | TxApp | RxApp | RxApp Display | Wire Format |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `ST_FRAME_FMT_ARGB` | 4:4:4 | 8-bit | Packed 32-bit | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_RGB_8BIT` |
| `ST_FRAME_FMT_BGRA` | 4:4:4 | 8-bit | Packed 32-bit | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_RGB_8BIT` |
| `ST_FRAME_FMT_RGB8` | 4:4:4 | 8-bit | Packed 24-bit | ✅ | `rgb24` | ✅ | `rgb24` | ❌ | ❌ | — | `ST20_FMT_RGB_8BIT` |
| `ST_FRAME_FMT_GBRPLANAR10LE` | 4:4:4 | 10-bit | Planar GBR | ❌ | — | ❌ | — | ✅ | ✅ | Full color | `ST20_FMT_RGB_10BIT` |
| `ST_FRAME_FMT_RGBRFC4175PG4BE10` | 4:4:4 | 10-bit | RFC4175 PG4 BE | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_RGB_10BIT` |
| `ST_FRAME_FMT_GBRPLANAR12LE` | 4:4:4 | 12-bit | Planar GBR | ❌ | — | ❌ | — | ✅ | ✅ | Full color | `ST20_FMT_RGB_12BIT` |
| `ST_FRAME_FMT_RGBRFC4175PG2BE12` | 4:4:4 | 12-bit | RFC4175 PG2 BE | ❌ | — | ❌ | — | ❌ | ❌ | — | `ST20_FMT_RGB_12BIT` |

---

## Notes

### FFmpeg Plugin (`mtl_st20p_tx.c` / `mtl_st20p_rx.c`)
- Supports only **3 formats**: `YUV422PLANAR10LE`, `Y210` (workaround), and `RGB8`.
- `Y210` requires a manual `st20_y210_to_rfc4175_422be10` conversion step in the write path — it is not natively supported by the MTL pipeline, added as a workaround for Intel Tiber Broadcast Suite.
- Format is set via `-pixel_format` FFmpeg option; default is `yuv422p10le`.

### TxApp (`applications/TxApp`)
- Supports **7 formats**: `YUV422PLANAR10LE`, `YUV420PLANAR8`, `YUV422PLANAR12LE`, `YUV444PLANAR10LE`, `YUV444PLANAR12LE`, `GBRPLANAR10LE`, `GBRPLANAR12LE`.
- Format is set via `--fmt` CLI argument or `input_format` field in JSON config.
- Default is `YUV422PLANAR10LE`.
- Test pattern generator fills `YUV422PLANAR10LE` and `YUV422PLANAR12LE`; other formats fall back to a simple memset pattern.

### RxApp (`applications/RxApp`)
- Supports **7 formats** for display and file saving (same set as TxApp).
- `YUV420PLANAR8` is received but displayed as **grayscale luma only** — chroma decode is not implemented in `frame_converter.c`.
- All other supported formats render full color via SDL2 using `convert_pixel_to_rgb()`.
- `GBRPLANAR10LE` / `GBRPLANAR12LE` use G=addr[0], B=addr[1], R=addr[2] plane order.

### RFC4175 Wire Formats
- All `RFC4175PGxBEyy` formats are **transport-layer only** — they represent the actual ST 2110 on-wire packet group encoding.
- These are never used as user-facing frame buffer formats directly.
- MTL internally converts between planar user formats and RFC4175 wire formats.

### Custom8 Formats
- `YUV420CUSTOM8` and `YUV422CUSTOM8` are **pass-through** formats — the frame buffer is transmitted as-is without RFC4175 conversion.
- Intended for non-standard formats like I420, NV12, or YUY2.
- Not implemented in any of the apps in this repository.

---

## Quick Reference: What to Use

| Use Case | Recommended Format | App Support |
|---|---|---|
| Broadcast production | `YUV422PLANAR10LE` | All three apps |
| High color fidelity | `YUV444PLANAR10LE` | TxApp + RxApp |
| RGB workflows | `GBRPLANAR10LE` | TxApp + RxApp |
| Low bandwidth / testing | `YUV420PLANAR8` | TxApp + RxApp |
| FFmpeg pipeline | `yuv422p10le` or `rgb24` | FFmpeg plugin only |
| Intel Tiber Suite | `y210le` | FFmpeg plugin (workaround) |
