# RxApp-X11 - Media Transport Library Receiver with X11 Display# RxApp - Media Transport Library Receiver with Display



## Overview## Overview



RxApp-X11 is a variant of RxApp that uses X11 (Xlib) for video display instead of SDL2. This provides a more lightweight display solution with direct X11 integration, reducing dependencies while maintaining full video display capabilities for SMPTE ST 2110 streams.RxApp is a comprehensive receiver application for the Media Transport Library (MTL) that can receive SMPTE ST 2110 streams and display them in real-time using SDL2. It's designed to work as a counterpart to TxApp for testing and monitoring media transport workflows.



## Key Differences from RxApp## Features



### Display System- **ST20P Video Reception**: Receives uncompressed video over SMPTE ST 2110-20

- **X11/Xlib**: Direct X11 window management and rendering- **ST30P Audio Reception**: Receives audio over SMPTE ST 2110-30

- **No SDL2 Dependency**: Eliminates SDL2 requirement for simpler deployment- **Real-time Video Display**: SDL2-based video rendering to monitor

- **Native Linux Integration**: Uses X11 protocol directly for better performance- **Multi-session Support**: Multiple concurrent video/audio streams

- **Minimal Dependencies**: Only requires libx11-dev for display functionality- **File Recording**: Save received streams to disk

- **Live Statistics**: Frame/packet counters and reception monitoring

### Features Maintained- **Flexible Configuration**: Command-line interface with extensive options

- **ST20P Video Reception**: Full support for SMPTE ST 2110-20 streams

- **ST30P Audio Reception**: Complete audio stream handling## Display Capabilities

- **Multi-session Support**: Multiple concurrent video/audio streams

- **File Recording**: Save received streams to disk### Video Display Features

- **Live Statistics**: Frame/packet counters and reception monitoring- **Real-time Rendering**: Live video display using SDL2

- **Modular Architecture**: Clean separation of concerns- **Multiple Windows**: Each video session opens its own display window

- **Window Management**: Resizable windows with session identification

## System Requirements- **Format Support**: YUV422P10LE, YUV420P8, YUV422P12LE, YUV444P10LE, YUV444P12LE, RGB10LE, RGB12LE pixel formats

- **Performance Optimized**: Hardware-accelerated rendering when available

### Ubuntu Dependencies

```bash### Display Controls

# Essential build tools- **Enable/Disable**: Use `--no_display` to run headless

sudo apt install -y build-essential meson ninja-build pkg-config- **Window Positioning**: Automatic window offset for multiple sessions

- **Real-time Updates**: Live frame display with minimal latency

# X11 development libraries

sudo apt install -y libx11-dev## Building



# Standard libraries  ### Prerequisites

sudo apt install -y libpthread-stubs0-dev```bash

```# MTL library

sudo apt update

### Media Transport Librarysudo apt install libmtl-dev

Intel MTL version 25.02.0 or later is required.

# SDL2 for display

## Buildingsudo apt install libsdl2-dev



```bash# Build tools

# Build RxApp-X11sudo apt install meson ninja-build gcc

./build.sh```

```

### Build Steps

## Usage```bash

./build.sh

### Basic Reception```

```bash

# Receive video stream with X11 display## Usage

./build/RxApp-X11 --sip 192.168.1.100 --dip 239.168.85.20 --port 0000:af:00.0

```### Basic Commands



### Multiple Sessions#### Simple Video Reception with Display

```bash```bash

# Receive multiple video sessions./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --port 0000:af:01.0

./build/RxApp-X11 --sip 192.168.1.100 --dip 239.168.85.20 --st20p_sessions 2```

```

#### Video + Audio Reception

### Headless Mode```bash

```bash./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --st20p_sessions 1 --st30p_sessions 1

# Run without display```

./build/RxApp-X11 --sip 192.168.1.100 --dip 239.168.85.20 --no_display --rx_url received_

```#### Multiple Video Sessions

```bash

## X11 Display Features./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --st20p_sessions 2

```

### Window Management

- **Multiple Windows**: Each session opens separate X11 window#### Headless Recording (No Display)

- **Window Titles**: Shows session ID and resolution```bash

- **Positioning**: Automatic offset for multiple sessions./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --no_display --rx_url recorded_stream

- **Resizable**: Standard X11 window controls```



### Display Performance#### High Resolution Reception

- **Direct Rendering**: No intermediate SDL layer```bash

- **Hardware Acceleration**: Uses X11 display hardware capabilities  ./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --width 3840 --height 2160 --fps 60

- **Efficient Memory**: Direct XImage buffer management```

- **Low Latency**: Minimal display overhead

### Command Line Options

### Event Handling

- **Keyboard Events**: ESC key handling for window interaction```

- **Window Events**: Expose, resize, and close event processingUsage: RxApp [OPTIONS]

- **Non-blocking**: Display events don't interfere with stream processing

Options:

## Format Support  --port <pci_addr>       Network port PCI address (default: 0000:af:01.0)

  --sip <ip>              Source IP address (required)

### Video Formats  --dip <ip>              Destination/Multicast IP address (default: 239.168.85.20)

- **YUV422P10LE**: 10-bit 4:2:2 planar (primary format)  --udp_port <port>       Base UDP port (default: 20000)

- **YUV420P8**: 8-bit 4:2:0 planar  --width <width>         Video width (default: 1920)

- **Test Patterns**: Gradient patterns for unsupported formats  --height <height>       Video height (default: 1080)

  --fps <fps>             Frame rate: 25, 30, 50, 60 (default: 25)

### Color Conversion  --fmt <format>          Pixel format: yuv422p10le, yuv420p, yuv422p12le, yuv444p10le, yuv444p12le, rgb10le, rgb12le (default: yuv422p10le)

- **ITU-R BT.601**: Standard YUV to RGB conversion  --rx_url <file>         Save received frames to file

- **Bit Depth Scaling**: Automatic 10-bit to 8-bit conversion  --st20p_sessions <n>    Number of ST20P sessions (default: 1)

- **Subsampling**: Proper 4:2:2 and 4:2:0 handling  --st30p_sessions <n>    Number of ST30P sessions (default: 0)

  --time <seconds>        Test duration in seconds (0=infinite)

## Performance Characteristics  --no_display           Disable SDL display (default: display enabled)

  --help                  Show help

### Memory Usage```

- **Lower Overhead**: No SDL2 framework overhead

- **Direct Buffers**: Single image buffer per session### Working with TxApp

- **Efficient Conversion**: In-place YUV to RGB conversion

#### Start TxApp (Transmitter)

### CPU Usage```bash

- **Reduced Processing**: Direct X11 calls without SDL abstractioncd ../TxApp

- **Optimized Rendering**: Hardware-accelerated when available./build/TxApp --sip 192.168.1.101 --dip 239.168.85.20 --port 0000:af:01.0 --tx_url test.yuv

- **Event Processing**: Lightweight X11 event handling```



## Integration Testing#### Start RxApp (Receiver with Display)

```bash

### With TxApp./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --port 0000:af:01.0

```bash```

# System 1 (Receiver) - X11 display

./build/RxApp-X11 --sip 192.168.1.100 --dip 239.168.85.20## Architecture



# System 2 (Transmitter)### Core Components

../TxApp/build/TxApp --sip 192.168.1.101 --dip 239.168.85.20 --tx_url video.yuv

```1. **Main Application Context** (`rx_app_context`)

   - MTL library management

## Troubleshooting   - Global configuration

   - Session coordination

### X11 Display Issues

```bash2. **ST20P RX Context** (`st20p_rx_ctx`)

# Check X11 display availability   - Video session management

echo $DISPLAY   - Frame processing thread

   - SDL display integration

# Test X11 connection   - File recording

xdpyinfo | head -5

3. **ST30P RX Context** (`st30p_rx_ctx`)

# Verify X11 libraries   - Audio session management

pkg-config --modversion x11   - Packet processing thread

```   - Audio data handling



### Common Issues4. **SDL Display Context** (`sdl_display_ctx`)

- **No Display**: Ensure DISPLAY environment variable is set   - Window management

- **Permission Denied**: Check X11 access permissions with xauth   - Texture handling

- **Window Creation Failed**: Verify X11 server is running   - Rendering pipeline

- **Remote Display**: Use SSH X11 forwarding (`ssh -X`)

### Threading Model

### Debug Mode

```bash- **Main Thread**: Application control and coordination

# Enable X11 error reporting- **ST20P Threads**: One per video session for frame processing and display

export XLIB_ERROR_REPORTING=1- **ST30P Threads**: One per audio session for packet processing

./build/RxApp-X11 --sip 192.168.1.100 --dip 239.168.85.20- **SDL Event Loop**: Integrated into frame processing threads

```

### Display Pipeline

## Architecture

```

### X11 Handler ModuleNetwork → MTL → Frame Buffer → YUV Processing → SDL Texture → Display

- **x11_handler.c/h**: Complete X11 display implementation```

- **Window Management**: X11 window creation and event handling  

- **Image Rendering**: XImage creation and buffer management1. **Frame Reception**: MTL library receives network frames

- **Event Processing**: Non-blocking X11 event loop2. **Format Conversion**: YUV/RGB formats to display format with proper bit-depth scaling

3. **SDL Integration**: Frame data copied to SDL texture

### Frame Conversion4. **Rendering**: Hardware-accelerated display update

- **RGB Conversion**: YUV to RGB conversion for X11 display5. **Event Handling**: Window events and user interaction

- **Format Support**: Multiple input format handling

- **Efficient Processing**: Optimized conversion algorithms## Network Configuration



### Session Management### Default Settings

- **Display Integration**: X11 display per session- **Port**: 0000:af:01.0 (modify for your network card)

- **Thread Safety**: Safe multi-session display handling- **Source IP**: Must be provided (--sip parameter)

- **Resource Management**: Proper X11 resource cleanup- **Multicast Group**: 239.168.85.20 

- **Base UDP Port**: 20000

## Comparison with SDL Version

### Multi-session Port Allocation

| Feature | RxApp (SDL2) | RxApp-X11 |- **ST20P sessions**: base_port + (session_id * 2)

|---------|-------------|-----------|  - Session 0: UDP 20000

| Display Library | SDL2 | X11/Xlib |  - Session 1: UDP 20002

| Dependencies | libsdl2-dev | libx11-dev |  - Session 2: UDP 20004

| Memory Usage | Higher | Lower |- **ST30P sessions**: base_port + 100 + (session_id * 2)

| Performance | Good | Better |  - Session 0: UDP 20100

| Platform Support | Cross-platform | Linux/X11 only |  - Session 1: UDP 20102

| Window Management | SDL abstraction | Direct X11 |

### Network Requirements

## Future Enhancements- **Multicast Support**: Network must support IGMP

- **Bandwidth**: Ensure sufficient network bandwidth for uncompressed video

- **Wayland Support**: Add Wayland protocol support- **Latency**: Low-latency network recommended for real-time display

- **Hardware Acceleration**: GPU-accelerated YUV conversion  

- **Window Decorations**: Enhanced window appearance## Display Configuration

- **Multi-monitor**: Multi-display support

- **Full-screen Mode**: Dedicated full-screen display option### SDL2 Features Used
- **Hardware Acceleration**: SDL_RENDERER_ACCELERATED
- **Texture Streaming**: SDL_TEXTUREACCESS_STREAMING
- **YUV Support**: SDL_PIXELFORMAT_YUY2
- **Window Management**: Multiple resizable windows

### Display Formats
- **Input**: YUV422P10LE, YUV420P8, YUV422P12LE, YUV444P10LE, YUV444P12LE, RGB10LE, RGB12LE
- **SDL Format**: YUY2 (packed YUV 4:2:2)
- **Output**: RGB display via graphics driver

### Performance Optimization
- **Zero-copy Texture Updates**: Direct frame buffer access
- **Hardware Rendering**: GPU-accelerated when available
- **Minimal Processing**: Simple format conversion only

## File Output

### Video Files
- **Format**: Raw YUV matching input parameters
- **Naming**: `<base_name>_st20p_<session_id>.yuv`
- **Content**: Sequential frames as received

### Audio Files  
- **Format**: Raw PCM audio data
- **Naming**: `<base_name>_st30p_<session_id>.pcm`
- **Content**: Sequential audio packets

### Example Output
```bash
# Command
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --rx_url capture --st20p_sessions 2 --st30p_sessions 1

# Generated files
capture_st20p_0.yuv  # Video session 0
capture_st20p_1.yuv  # Video session 1  
capture_st30p_0.pcm  # Audio session 0
```

## Troubleshooting

### Display Issues

1. **No Display Window**
   ```bash
   # Check SDL2 installation
   pkg-config --libs sdl2
   
   # Verify X11 forwarding (SSH)
   echo $DISPLAY
   
   # Run with display disabled to test reception
   ./build/RxApp --no_display --sip 192.168.1.100 --dip 239.168.85.20
   ```

2. **Black/Corrupted Display**
   - Check video format compatibility
   - Verify frame size matches transmission
   - Ensure proper YUV format conversion

3. **Display Performance Issues**
   ```bash
   # Check GPU acceleration
   glxinfo | grep "direct rendering"
   
   # Monitor CPU usage
   top -p $(pgrep RxApp)
   
   # Reduce display load
   ./build/RxApp --width 1280 --height 720 --fps 25
   ```

### Network Issues

1. **No Frames Received**
   ```bash
   # Check multicast reception
   tcpdump -i <interface> host 239.168.85.20
   
   # Verify MTL initialization
   ./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --no_display
   
   # Check network interface
   ip addr show
   ```

2. **Packet Loss**
   ```bash
   # Monitor network statistics
   ethtool -S <interface> | grep drop
   
   # Increase buffer sizes
   sysctl net.core.rmem_max
   sysctl net.core.rmem_default
   ```

### Performance Issues

1. **High CPU Usage**
   - Use `--no_display` to isolate display overhead
   - Reduce video resolution or frame rate
   - Check for memory leaks

2. **Frame Drops**
   - Verify network bandwidth
   - Check system memory usage
   - Monitor disk I/O if recording

## Integration Examples

### With TxApp
```bash
# Terminal 1: Start receiver
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20

# Terminal 2: Start transmitter  
cd ../TxApp
./build/TxApp --sip 192.168.1.101 --dip 239.168.85.20 --tx_url test.yuv
```

### With Professional Equipment
```bash
# Receive from broadcast equipment
./build/RxApp --sip 192.168.10.100 --dip 239.255.1.1 --udp_port 5004 --width 1920 --height 1080 --fps 60
```

### Multiple Streams Monitoring
```bash
# Monitor multiple camera feeds
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --st20p_sessions 4 --width 1920 --height 1080
```

## System Requirements

### Minimum Requirements
- **CPU**: Intel Core i5 or equivalent
- **RAM**: 4GB (8GB recommended for 4K)
- **Network**: 1Gbps Ethernet (10Gbps for 4K)
- **GPU**: Any GPU with SDL2 support
- **OS**: Ubuntu 20.04+ or equivalent Linux

### Recommended for 4K
- **CPU**: Intel Core i7 or equivalent
- **RAM**: 16GB
- **Network**: 25Gbps Ethernet
- **GPU**: Dedicated GPU with hardware acceleration
- **Storage**: NVMe SSD for recording

This application provides a complete solution for receiving and displaying SMPTE ST 2110 media streams with real-time monitoring capabilities.