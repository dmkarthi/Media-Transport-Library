# Media Transport Library Applications

## Overview

This directory contains standalone applications for the Intel Media Transport Library (MTL):

- **[RxApp](RxApp/README.md)**: Receiver application with SDL2 display for monitoring SMPTE ST 2110 streams
- **[TxApp](TxApp/README.md)**: Transmitter application for sending SMPTE ST 2110 streams

Both applications provide simplified interfaces for testing and demonstrating MTL capabilities without complex configuration requirements.

## Important: Network Connectivity Guide

**⚠️ IMPORTANT:** When using DPDK PMD mode (default), binding a network interface removes it from Linux kernel networking. **You will lose SSH access and normal ethernet connectivity on that interface.**

**Read this first:** [Network Connectivity Guide](study-docs/nic_pmd_connectivity_guide.md)

This guide explains:
- Why connectivity is lost when binding to DPDK PMD
- **Solution 1**: Use separate management interface (production recommended)
- **Solution 2**: Use AF_XDP mode (no binding, keeps connectivity)
- **Solution 3**: Use kernel socket mode (testing only)

**Quick Recommendations:**
- **Production**: Use dual NIC setup (one for management, one for MTL data)
- **Development**: Switch to AF_XDP mode (no binding required, keeps SSH access)
- **Testing**: Use kernel socket mode (lowest performance, easiest setup)

## System Requirements

### Ubuntu Package Dependencies

Before building these applications, install the following packages on Ubuntu:

```bash
# Essential build tools
sudo apt update
sudo apt install -y build-essential gcc g++ make

# Meson build system
sudo apt install -y meson ninja-build pkg-config

# Development libraries
sudo apt install -y libpthread-stubs0-dev

# SDL2 development libraries (required for RxApp display)
sudo apt install -y libsdl2-dev libsdl2-2.0-0

# Additional utilities (optional but recommended)
sudo apt install -y git cmake python3-pip
```

### Media Transport Library (MTL)

The Intel Media Transport Library must be installed separately. Please refer to the main MTL documentation for installation instructions.

**Note**: These applications require MTL version 25.02.0 or later.

## Quick Start

### 1. Install Dependencies
```bash
# Install Ubuntu packages
sudo apt update
sudo apt install -y build-essential meson ninja-build pkg-config libsdl2-dev libpthread-stubs0-dev
```

### 2. Build Applications
```bash
# Build RxApp (Receiver with Display)
cd RxApp
./build.sh

# Build TxApp (Transmitter)
cd ../TxApp
./build.sh
```

### 3. Basic Usage
```bash
# Terminal 1: Start receiver
cd RxApp
./build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --port 0000:af:00.0

# Terminal 2: Start transmitter
cd ../TxApp
./build/TxApp --sip 192.168.1.101 --dip 239.168.85.20 --port 0000:af:01.0 --tx_url test.yuv
```

## Applications

### RxApp - Receiver with Display

A comprehensive receiver application featuring:
- **Real-time Video Display**: SDL2-based rendering with multi-window support
- **Multi-format Support**: YUV422P10LE, YUV420P8, RGB10LE, and more
- **Session Management**: Multiple concurrent ST20P/ST30P sessions
- **Recording Capabilities**: Save received streams to disk
- **Live Monitoring**: Frame counters and reception statistics

**[→ RxApp Documentation](RxApp/README.md)**

### TxApp - Transmitter

A simplified transmitter application featuring:
- **ST20P/ST30P Transmission**: Video and audio over SMPTE ST 2110
- **Multi-session Support**: Concurrent streams to different destinations
- **File-based Input**: Transmit from YUV video files
- **Test Pattern Generation**: Built-in test pattern capabilities
- **Simple CLI**: Easy-to-use command line interface

**[→ TxApp Documentation](TxApp/README.md)**

## Integration Testing

Both applications are designed to work together for end-to-end testing across different systems:

```bash
# System 1 (Receiver) - IP: 192.168.1.100
./RxApp/build/RxApp --sip 192.168.1.100 --dip 239.168.85.20 --port 0000:af:00.0

# System 2 (Transmitter) - IP: 192.168.1.101  
./TxApp/build/TxApp --sip 192.168.1.101 --dip 239.168.85.20 --port 0000:af:01.0 --tx_url video.yuv
```

**Note**: For proper SMPTE ST 2110 testing, RxApp and TxApp should be deployed on separate systems with dedicated network interfaces connected to the same multicast-capable network.

## Network Configuration

### Prerequisites
- Network interfaces configured for multicast
- Hugepages configured for MTL (refer to MTL documentation)
- Proper network permissions for raw socket access

### Example Network Setup
```bash
# Enable hugepages (example)
echo 1024 | sudo tee /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

# Configure multicast routing (if needed)
sudo ip route add 239.0.0.0/8 dev <your-interface>
```

## Troubleshooting

### Common Issues

**Build Errors:**
- Verify MTL library is installed and pkg-config can find it
- Check that all Ubuntu dependencies are installed
- Ensure meson and ninja versions are compatible

**Runtime Issues:**
- Verify network interface configuration
- Check hugepage allocation
- Ensure multicast routing is configured
- Verify PCI device addresses for network ports

**Network Connectivity Issues (Losing SSH Access):**
- **Problem**: After binding to DPDK PMD, you lose SSH access and normal ethernet connectivity
- **Cause**: This is **expected behavior** - DPDK bypasses kernel networking
- **Solutions**: See [Network Connectivity Guide](study-docs/nic_pmd_connectivity_guide.md) for detailed solutions:
  - Use separate management interface (dual NIC)
  - Switch to AF_XDP mode (no binding required)
  - Switch to kernel socket mode (testing only)

**Display Issues (RxApp):**
- Verify SDL2 development libraries are installed
- Check X11 display configuration for remote systems
- Use `--no_display` flag for headless operation

### Getting Help

For detailed troubleshooting and configuration guidance:
- **Network Connectivity Issues**: See [Network Connectivity Guide](study-docs/nic_pmd_connectivity_guide.md)
- **RxApp Issues**: See [RxApp README](RxApp/README.md)
- **TxApp Issues**: See [TxApp README](TxApp/README.md)
- **MTL Setup**: Refer to main MTL documentation

## Development

### Building from Source
Both applications use the Meson build system:

```bash
# Manual build process
meson setup build
meson compile -C build

# Or use provided build scripts
./build.sh
```

### Project Structure
```
applications/
├── README.md               # This file - overview and build instructions
├── RxApp/                  # Receiver application with SDL display
│   ├── README.md          # RxApp-specific documentation and usage
│   ├── build.sh           # Build script for RxApp
│   ├── meson.build        # Meson build configuration
│   ├── configs/           # Sample configuration files
│   │   ├── rx_1v.json
│   │   └── rx_1v_1a_1anc.json
│   ├── include/           # Header files
│   │   ├── config_reader.h
│   │   ├── frame_converter.h
│   │   ├── rx_app_context.h
│   │   ├── sdl_handler.h
│   │   └── session_manager.h
│   └── src/               # Modular source code
│       ├── rx_app_main.c      # Main application (argument parsing only)
│       ├── session_manager.c   # MTL session management
│       ├── sdl_handler.c      # SDL display functionality  
│       ├── config_reader.c    # JSON configuration loading
│       └── frame_converter.c  # YUV/RGB frame conversion
└── TxApp/                  # Transmitter application
    ├── README.md          # TxApp-specific documentation and usage
    ├── build.sh           # Build script for TxApp
    ├── meson.build        # Meson build configuration
    ├── configs/           # Sample configuration files
    │   ├── tx_1v.json
    │   ├── tx_1v_1a.json
    │   ├── tx_1v_1a_1anc.json
    │   ├── tx_2v2dest_1a_1anc.json
    │   └── tx_test.json
    ├── include/           # Header files
    │   ├── config_reader.h
    │   ├── session_manager.h
    │   └── tx_app_context.h
    ├── src/               # Modular source code
    │   ├── tx_app_main.c      # Main application (argument parsing only)
    │   ├── session_manager.c   # MTL session management
    │   └── config_reader.c    # JSON configuration loading
    └── yuv422p10le_1080p.yuv # Test video file (16MB)
```

### Modular Architecture

Both applications feature clean modular designs with separated concerns:

- **Main Files**: Only contain `main()` and argument parsing logic
- **Session Management**: Complete MTL session handling in dedicated modules
- **Configuration**: Centralized JSON config loading and validation
- **Display (RxApp)**: SDL2 rendering separated into dedicated handler
- **Frame Processing**: YUV/RGB conversion and display logic modularized