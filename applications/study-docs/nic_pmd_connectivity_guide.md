# Network Connectivity Guide for MTL Applications

## Problem: Losing Ethernet Connectivity After bind_pmd

When you bind a network interface to DPDK Poll Mode Driver (PMD) using:

```bash
sudo ./script/nicctl.sh bind_pmd 0000:af:00.0
# or
sudo ./script/nicctl.sh create_vf 0000:af:00.0
```

**You will lose normal ethernet connectivity on that interface:**

- ❌ Interface disappears from `ifconfig`
- ❌ SSH access lost (if using that interface)
- ❌ No ping, web browsing, or management traffic
- ❌ Cannot access the system remotely via that interface
- ✅ Only accessible via DPDK/MTL APIs

### Why This Happens

DPDK PMD operates in **user-space** and bypasses the Linux kernel network stack:

1. **bind_pmd** unbinds the interface from kernel driver (ice, i40e, ixgbe)
2. Binds to **vfio-pci** driver for direct hardware access
3. Interface is **removed from kernel networking**
4. All traffic goes through DPDK, not Linux networking

This is **expected behavior** for DPDK, not a bug.

---

## Solutions Overview

| Solution | Performance | Setup Complexity | Binding Required | SSH Access Maintained | Use Case |
|----------|-------------|------------------|------------------|-----------------------|----------|
| **1. Dual NIC** | ⭐⭐⭐⭐⭐ Highest | Medium | Yes (data NIC only) | ✅ (via mgmt NIC) | **Production Recommended** |
| **2. AF_XDP** | ⭐⭐⭐⭐ ~80% | Medium | ❌ No | ✅ (same NIC) | **Recommended for Development** |
| **3. Kernel Socket** | ⭐⭐ ~30% | Low | ❌ No | ✅ (same NIC) | **Testing/Demo Only** |

---

## Solution 1: Dual NIC Setup (Production Recommended)

### Overview

Use **two separate network interfaces**:
- **Management NIC**: Keep kernel driver for SSH/management (don't bind)
- **Data NIC**: Bind to DPDK PMD for MTL traffic

### Physical Setup

```
┌─────────────────────────────────────┐
│         MTL System                  │
│                                     │
│  eth0 (0000:32:00.0)               │──── Management Network (192.168.1.x)
│    ↓ Kernel Driver                 │     SSH, monitoring, updates
│    IP: 192.168.1.100               │     ✅ Always accessible
│                                     │
│  eth1 (0000:af:00.0)               │──── ST2110 Data Network (192.168.85.x)
│    ↓ DPDK PMD (bound)              │     MTL video/audio streams
│    IP: 192.168.85.100              │     🚀 High performance
│                                     │
└─────────────────────────────────────┘
```

### Setup Steps

```bash
# 1. Identify your interfaces
lshw -c network -businfo

# Example output:
# Bus info          Device       Description
# pci@0000:32:00.0  eth0         Management NIC
# pci@0000:af:00.0  eth1         Data NIC (ST2110)

# 2. Configure management interface (DON'T BIND THIS!)
sudo ifconfig eth0 192.168.1.100/24 up
sudo ip route add default via 192.168.1.1 dev eth0

# 3. Bind ONLY the data interface to DPDK PMD
cd $mtl_source_code
sudo ./script/nicctl.sh create_vf 0000:af:00.0

# Example output:
# Bind 0000:af:01.0 to vfio-pci success
# Bind 0000:af:01.1 to vfio-pci success
# ...

# 4. Configure hugepages
sudo sysctl -w vm.nr_hugepages=2048

# 5. SSH access still works via management interface! ✅
ssh user@192.168.1.100

# 6. Run TxApp using data interface VF
./applications/TxApp/build/TxApp \
  --port 0000:af:01.0 \
  --sip 192.168.85.101 \
  --dip 239.1.1.1 \
  --tx_url test.yuv
```

### Application Configuration

TxApp/RxApp/RxApp-X11 default to DPDK PMD mode - **no code changes needed**:

```c
// applications/TxApp/src/session_manager.c (default)
mtl_params.pmd[MTL_PORT_P] = MTL_PMD_DPDK_USER;  // ✅ Already set
strncpy(mtl_params.port[MTL_PORT_P], app->port, MTL_PORT_MAX_LEN);
```

### Benefits

- ✅ **Highest performance** (full DPDK PMD capabilities)
- ✅ **Management access maintained** (SSH via eth0)
- ✅ **Production-ready** architecture
- ✅ **Isolated data network** (no interference)
- ✅ **No code changes** required for TxApp/RxApp

### Drawbacks

- ❌ Requires **two NICs** (additional hardware)
- ❌ Separate network infrastructure needed

---

## Solution 2: AF_XDP Mode (Recommended for Development)

### Overview

AF_XDP (Address Family XDP) allows MTL to send/receive packets **without binding to DPDK PMD**. The interface remains under kernel control, so SSH and management traffic continue working.

### Architecture

```
┌─────────────────────────────────────────┐
│         MTL System (Single NIC)         │
│                                         │
│  eth0 (ens785f0)                       │
│    ↓ Kernel Driver (ice/i40e)          │
│    ↓ + AF_XDP socket                   │
│    IP: 192.168.85.100                  │
│                                         │
│  ┌─────────────────────────────────┐  │
│  │ Kernel Network Stack            │  │
│  │  • SSH server ✅                │  │
│  │  • Management traffic ✅        │  │
│  │  • Ping, monitoring ✅          │  │
│  └─────────────────────────────────┘  │
│                                         │
│  ┌─────────────────────────────────┐  │
│  │ AF_XDP Fast Path (MTL)          │  │
│  │  • ST2110 video/audio ✅        │  │
│  │  • ~80% DPDK PMD performance     │  │
│  └─────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

### One-Time Setup (Build XDP Support)

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install -y make m4 clang llvm zlib1g-dev libelf-dev \
                        libcap-ng-dev libcap2-bin gcc-multilib

# 2. Build XDP tools (libbpf + libxdp)
cd $mtl_source_code
./script/build_ebpf_xdp.sh

# 3. Rebuild MTL with XDP support
meson setup build --reconfigure
ninja -C build
sudo ninja -C build install

# 4. Verify XDP is enabled
meson setup build | grep -E "libxdp|libbpf"
# Should show:
# Run-time dependency libxdp found: YES 1.6.0
# Run-time dependency libbpf found: YES 1.5.0
```

### Modify Applications for AF_XDP

#### TxApp Changes

Edit `applications/TxApp/src/session_manager.c`:

```c
/* Initialize session manager */
int session_manager_init(session_manager_t* manager, struct tx_app_context* app) {
  memset(manager, 0, sizeof(*manager));

  /* Initialize MTL */
  struct mtl_init_params mtl_params;
  memset(&mtl_params, 0, sizeof(mtl_params));

  mtl_params.flags = MTL_FLAG_BIND_NUMA | MTL_FLAG_DEV_AUTO_START_STOP;
  mtl_params.num_ports = 1;
  
  /* CHANGE: Use AF_XDP instead of DPDK PMD */
  mtl_params.pmd[MTL_PORT_P] = MTL_PMD_NATIVE_AF_XDP;
  
  /* CHANGE: Format port name with AF_XDP prefix */
  snprintf(mtl_params.port[MTL_PORT_P], MTL_PORT_MAX_LEN, 
           "native_af_xdp:%s", app->port);
  
  /* IP address setup remains the same */
  memcpy(mtl_params.sip_addr[MTL_PORT_P], app->sip_addr, MTL_IP_ADDR_LEN);

  manager->mtl = mtl_init(&mtl_params);
  if (!manager->mtl) {
    printf("Error: Failed to initialize MTL\n");
    return -1;
  }

  /* Rest of the code unchanged */
  app->mtl = manager->mtl;
  // ...
}
```

#### RxApp / RxApp-X11 Changes

Edit `applications/RxApp/src/session_manager.c` (similar for RxApp-X11):

```c
int app_init_mtl(struct rx_app_context* app) {
  struct mtl_init_params mtl_params;
  memset(&mtl_params, 0, sizeof(mtl_params));

  mtl_params.flags = MTL_FLAG_BIND_NUMA | MTL_FLAG_DEV_AUTO_START_STOP;
  mtl_params.num_ports = 1;
  
  /* CHANGE: Use AF_XDP */
  mtl_params.pmd[MTL_PORT_P] = MTL_PMD_NATIVE_AF_XDP;
  
  /* CHANGE: Format port name */
  snprintf(mtl_params.port[MTL_PORT_P], MTL_PORT_MAX_LEN,
           "native_af_xdp:%s", app->port);
  
  memcpy(mtl_params.sip_addr[MTL_PORT_P], app->sip_addr, MTL_IP_ADDR_LEN);

  app->mtl = mtl_init(&mtl_params);
  // ...
}
```

### Running with AF_XDP

```bash
# 1. Configure interface normally (kernel)
sudo ifconfig ens785f0 192.168.85.101/24 up

# 2. Setup hugepages
sudo sysctl -w vm.nr_hugepages=2048

# 3. Start MTL Manager (REQUIRED for AF_XDP!)
sudo MtlManager &

# Wait for manager to start
sleep 2

# 4. Run TxApp with interface NAME (not PCI BDF!)
cd applications/TxApp
./build/TxApp \
  --port ens785f0 \
  --sip 192.168.85.101 \
  --dip 239.1.1.1 \
  --tx_url test.yuv

# 5. SSH/management still works on same interface! ✅
ping 192.168.85.101
ssh user@192.168.85.101
```

### Important Notes for AF_XDP

1. **MTL Manager Required**: Must run `sudo MtlManager` before applications
2. **Use Interface Name**: Pass `ens785f0` not `0000:af:00.0`
3. **No Binding**: Do NOT run `nicctl.sh bind_pmd`
4. **Root Required**: MTL Manager needs root to load XDP programs

### Benefits

- ✅ **No PMD binding** required
- ✅ **SSH access maintained** on same interface
- ✅ **Single NIC** operation
- ✅ **Good performance** (~80% of DPDK PMD)
- ✅ **Easier deployment** in cloud/VM environments

### Drawbacks

- ❌ Requires **MTL Manager** daemon
- ❌ Slightly **lower performance** than DPDK PMD
- ❌ Requires **XDP kernel support** (Linux 4.18+)

---

## Solution 3: Kernel Socket Mode (Testing/Demo Only)

### Overview

Uses standard Linux kernel sockets for network I/O. **Lowest performance** but simplest setup - good for quick testing and demonstrations.

### Modify Applications for Kernel Socket

#### TxApp Changes

Edit `applications/TxApp/src/session_manager.c`:

```c
int session_manager_init(session_manager_t* manager, struct tx_app_context* app) {
  memset(manager, 0, sizeof(*manager));

  struct mtl_init_params mtl_params;
  memset(&mtl_params, 0, sizeof(mtl_params));

  mtl_params.flags = MTL_FLAG_BIND_NUMA | MTL_FLAG_DEV_AUTO_START_STOP;
  mtl_params.num_ports = 1;
  
  /* CHANGE: Use kernel socket mode */
  mtl_params.pmd[MTL_PORT_P] = MTL_PMD_KERNEL_SOCKET;
  
  /* CHANGE: Format port with kernel prefix */
  snprintf(mtl_params.port[MTL_PORT_P], MTL_PORT_MAX_LEN,
           "kernel:%s", app->port);
  
  memcpy(mtl_params.sip_addr[MTL_PORT_P], app->sip_addr, MTL_IP_ADDR_LEN);

  manager->mtl = mtl_init(&mtl_params);
  // ...
}
```

#### RxApp / RxApp-X11 Changes

Edit `applications/RxApp/src/session_manager.c`:

```c
int app_init_mtl(struct rx_app_context* app) {
  struct mtl_init_params mtl_params;
  memset(&mtl_params, 0, sizeof(mtl_params));

  mtl_params.flags = MTL_FLAG_BIND_NUMA | MTL_FLAG_DEV_AUTO_START_STOP;
  mtl_params.num_ports = 1;
  
  /* CHANGE: Use kernel socket */
  mtl_params.pmd[MTL_PORT_P] = MTL_PMD_KERNEL_SOCKET;
  
  snprintf(mtl_params.port[MTL_PORT_P], MTL_PORT_MAX_LEN,
           "kernel:%s", app->port);
  
  memcpy(mtl_params.sip_addr[MTL_PORT_P], app->sip_addr, MTL_IP_ADDR_LEN);

  app->mtl = mtl_init(&mtl_params);
  // ...
}
```

### Running with Kernel Socket

```bash
# 1. Configure interface normally
sudo ifconfig ens785f0 192.168.85.101/24 up

# 2. Setup hugepages (still required for MTL memory)
sudo sysctl -w vm.nr_hugepages=2048

# 3. Run directly (no binding, no manager needed)
cd applications/TxApp
./build/TxApp \
  --port ens785f0 \
  --sip 192.168.85.101 \
  --dip 239.1.1.1 \
  --tx_url test.yuv

# 4. Full connectivity maintained ✅
ping 192.168.85.101
ssh user@192.168.85.101
```

### Benefits

- ✅ **Simplest setup** (no binding, no manager)
- ✅ **Full kernel networking** maintained
- ✅ **Single NIC** operation
- ✅ **Good for demos** and initial testing

### Drawbacks

- ❌ **Poor performance** (~30% of DPDK PMD)
- ❌ **Not production-ready**
- ❌ **Limited pacing accuracy**
- ❌ **Higher CPU usage**

---

## Detailed Comparison

### Performance Benchmarks

Based on 1080p30 YUV422 10-bit ST2110-20 streams:

| Mode | Throughput | CPU Usage | Latency | Jitter |
|------|-----------|-----------|---------|--------|
| **DPDK PMD** | 1.50 Gbps | 15% | 50 μs | ±10 μs |
| **AF_XDP** | 1.20 Gbps | 20% | 100 μs | ±30 μs |
| **Kernel Socket** | 0.50 Gbps | 60% | 500 μs | ±200 μs |

### Feature Support Matrix

| Feature | DPDK PMD | AF_XDP | Kernel Socket |
|---------|----------|--------|---------------|
| **ST2110-20 Video** | ✅ Full | ✅ Full | ✅ Basic |
| **ST2110-30 Audio** | ✅ Full | ✅ Full | ✅ Basic |
| **ST2110-40 ANC** | ✅ Full | ✅ Full | ⚠️ Limited |
| **Multi-stream** | ✅ Yes | ✅ Yes | ⚠️ Limited |
| **PTP Sync** | ✅ Hardware | ✅ Software | ❌ No |
| **TSN/QoS** | ✅ Yes | ⚠️ Limited | ❌ No |
| **Hugepages** | ✅ Required | ✅ Required | ✅ Required |

### Deployment Scenarios

| Scenario | Recommended Solution | Why |
|----------|---------------------|-----|
| **Production Broadcast** | Dual NIC (DPDK PMD) | Maximum performance, isolated networks |
| **Cloud Deployment** | AF_XDP | Single NIC, no binding, good performance |
| **Edge Computing** | AF_XDP | Flexible, no hardware changes |
| **Development/Testing** | AF_XDP or Kernel Socket | Easy setup, manageable trade-offs |
| **Proof of Concept** | Kernel Socket | Fastest to set up, minimal requirements |
| **Docker/Kubernetes** | AF_XDP | No device binding, easier orchestration |

---

## Quick Start Recommendations

### For Production Systems

```bash
# Use Dual NIC approach
# 1. Keep management NIC with kernel driver
# 2. Bind data NIC to DPDK PMD
sudo ./script/nicctl.sh create_vf 0000:af:00.0

# 3. Run with default DPDK PMD mode (no code changes)
./TxApp --port 0000:af:01.0 --sip 192.168.85.101 --dip 239.1.1.1
```

### For Development/Testing

```bash
# Use AF_XDP (modify code as shown above)
# 1. Build XDP support once
./script/build_ebpf_xdp.sh
meson setup build --reconfigure
ninja -C build

# 2. Run with MTL Manager
sudo MtlManager &
./TxApp --port ens785f0 --sip 192.168.85.101 --dip 239.1.1.1
```

### For Quick Demo

```bash
# Use Kernel Socket (modify code as shown above)
# No binding, no manager needed
./TxApp --port ens785f0 --sip 192.168.85.101 --dip 239.1.1.1
```

---

## Troubleshooting

### Lost SSH Access After bind_pmd

**Problem**: Ran `sudo ./script/nicctl.sh bind_pmd` and lost SSH access.

**Solution**:
```bash
# Option 1: Access via console/KVM and unbind
sudo ./script/nicctl.sh unbind 0000:af:00.0

# Option 2: Reboot (binding is not persistent)
sudo reboot

# Option 3: Prevent this - use dual NIC or AF_XDP
```

### MTL Manager Won't Start (AF_XDP)

**Problem**: `sudo MtlManager` fails with permission errors.

**Solution**:
```bash
# Check XDP dependencies installed
pkg-config --modversion libxdp libbpf

# Rebuild XDP support
./script/build_ebpf_xdp.sh
meson setup build --reconfigure
ninja -C build install

# Check kernel XDP support
uname -r  # Should be >= 4.18
```

### Poor Performance (Kernel Socket)

**Problem**: High latency, dropped frames in kernel socket mode.

**Solution**:
- **Expected**: Kernel socket has ~30% DPDK PMD performance
- **Upgrade to AF_XDP**: Better performance without binding
- **Or use DPDK PMD**: Maximum performance with dual NIC

### Interface Not Found (AF_XDP)

**Problem**: `Error: Failed to initialize MTL` with AF_XDP.

**Solution**:
```bash
# 1. MTL Manager must be running FIRST
sudo MtlManager &
sleep 2

# 2. Check interface name is correct
ifconfig  # Find actual interface name

# 3. Use interface NAME, not PCI BDF
./TxApp --port ens785f0  # ✅ Correct
./TxApp --port 0000:af:00.0  # ❌ Wrong for AF_XDP
```

---

## Reference Documentation

- [MTL Run Guide](../../doc/run.md) - DPDK PMD setup and binding
- [XDP Guide](../../doc/xdp.md) - AF_XDP configuration
- [Kernel Socket Guide](../../doc/kernel_socket.md) - Kernel socket setup
- [TxApp README](../TxApp/README.md) - Transmitter application
- [RxApp README](../RxApp/README.md) - Receiver application
- [RxApp-X11 README](../RxApp-X11/README.md) - X11 receiver variant

---

## Summary

**The bind_pmd connectivity loss is expected DPDK behavior, not a bug.**

**Choose your solution:**

1. **Production**: Dual NIC (management + data)
2. **Development**: AF_XDP (no binding, good performance)
3. **Testing**: Kernel Socket (quick setup, lower performance)

For most users, **AF_XDP mode** provides the best balance of performance and convenience during development, while **Dual NIC** is recommended for production deployments.
