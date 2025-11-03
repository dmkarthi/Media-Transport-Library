# RxApp - Media Transport Library Receiver with Display

## Overview

RxApp is a comprehensive receiver application for the Media Transport Library (MTL) that can receive SMPTE ST 2110 streams and display them in real-time using SDL2. It's designed to work as a counterpart to TxApp for testing and monitoring media transport workflows.

## Features

- **ST20P Video Reception**: Receives uncompressed video over SMPTE ST 2110-20
- **ST30P Audio Reception**: Receives audio over SMPTE ST 2110-30
- **Real-time Video Display**: SDL2-based video rendering to monitor
- **Multi-session Support**: Multiple concurrent video/audio streams
- **File Recording**: Save received streams to disk
- **Live Statistics**: Frame/packet counters and reception monitoring
- **Flexible Configuration**: Command-line interface with extensive options

## Display Capabilities

### Video Display Features
- **Real-time Rendering**: Live video display using SDL2
- **Multiple Windows**: Each video session opens its own display window
- **Window Management**: Resizable windows with session identification
- **Format Support**: YUV422P10LE, YUV420P8, YUV422P12LE, YUV444P10LE, YUV444P12LE, RGB10LE, RGB12LE pixel formats
- **Performance Optimized**: Hardware-accelerated rendering when available

### Display Controls
- **Enable/Disable**: Use `--no_display` to run headless
- **Window Positioning**: Automatic window offset for multiple sessions
- **Real-time Updates**: Live frame display with minimal latency

## Building

### Prerequisites
```bash
# MTL library
sudo apt update
sudo apt install libmtl-dev

# SDL2 for display
sudo apt install libsdl2-dev

# Build tools
sudo apt install meson ninja-build gcc
```

### Build Steps
```bash
./build.sh
```

## Usage

### Basic Commands

#### Simple Video Reception with Display
```bash
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --port 0000:af:01.0
```

#### Video + Audio Reception
```bash
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --st20p_sessions 1 --st30p_sessions 1
```

#### Multiple Video Sessions
```bash
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --st20p_sessions 2
```

#### Headless Recording (No Display)
```bash
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --no_display --rx_url recorded_stream
```

#### High Resolution Reception
```bash
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --width 3840 --height 2160 --fps 60
```

### Command Line Options

```
Usage: RxApp [OPTIONS]

Options:
  --port <pci_addr>       Network port PCI address (default: 0000:af:01.0)
  --sip <ip>              Source IP address (required)
  --dip <ip>              Destination/Multicast IP address (default: 239.168.85.20)
  --udp_port <port>       Base UDP port (default: 20000)
  --width <width>         Video width (default: 1920)
  --height <height>       Video height (default: 1080)
  --fps <fps>             Frame rate: 25, 30, 50, 60 (default: 25)
  --fmt <format>          Pixel format: yuv422p10le, yuv420p, yuv422p12le, yuv444p10le, yuv444p12le, rgb10le, rgb12le (default: yuv422p10le)
  --rx_url <file>         Save received frames to file
  --st20p_sessions <n>    Number of ST20P sessions (default: 1)
  --st30p_sessions <n>    Number of ST30P sessions (default: 0)
  --time <seconds>        Test duration in seconds (0=infinite)
  --no_display           Disable SDL display (default: display enabled)
  --help                  Show help
```

### Working with TxApp

#### Start TxApp (Transmitter)
```bash
cd ../TxApp
./build/TxApp --sip 192.168.1.101 --dip 239.168.85.20 --port 0000:af:01.0 --tx_url test.yuv
```

#### Start RxApp (Receiver with Display)
```bash
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --port 0000:af:01.0
```

## Architecture

### Core Components

1. **Main Application Context** (`rx_app_context`)
   - MTL library management
   - Global configuration
   - Session coordination

2. **ST20P RX Context** (`st20p_rx_ctx`)
   - Video session management
   - Frame processing thread
   - SDL display integration
   - File recording

3. **ST30P RX Context** (`st30p_rx_ctx`)
   - Audio session management
   - Packet processing thread
   - Audio data handling

4. **SDL Display Context** (`sdl_display_ctx`)
   - Window management
   - Texture handling
   - Rendering pipeline

### Threading Model

- **Main Thread**: Application control and coordination
- **ST20P Threads**: One per video session for frame processing and display
- **ST30P Threads**: One per audio session for packet processing
- **SDL Event Loop**: Integrated into frame processing threads

### Display Pipeline

```
Network → MTL → Frame Buffer → YUV Processing → SDL Texture → Display
```

1. **Frame Reception**: MTL library receives network frames
2. **Format Conversion**: YUV/RGB formats to display format with proper bit-depth scaling
3. **SDL Integration**: Frame data copied to SDL texture
4. **Rendering**: Hardware-accelerated display update
5. **Event Handling**: Window events and user interaction

## Network Configuration

### Default Settings
- **Port**: 0000:af:01.0 (modify for your network card)
- **Source IP**: Must be provided (--sip parameter)
- **Multicast Group**: 239.168.85.20 
- **Base UDP Port**: 20000

### Multi-session Port Allocation
- **ST20P sessions**: base_port + (session_id * 2)
  - Session 0: UDP 20000
  - Session 1: UDP 20002
  - Session 2: UDP 20004
- **ST30P sessions**: base_port + 100 + (session_id * 2)
  - Session 0: UDP 20100
  - Session 1: UDP 20102

### Network Requirements
- **Multicast Support**: Network must support IGMP
- **Bandwidth**: Ensure sufficient network bandwidth for uncompressed video
- **Latency**: Low-latency network recommended for real-time display

## Display Configuration

### SDL2 Features Used
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