# Zion-Client

## Project Summary

The deployment-facing files and update notes should be read with the implementation when assessing operational readiness.

The daemon's claims are therefore layered: source presence indicates an implementation surface, named unit tests indicate intended automated checks, and a passing build indicates compilation only. Connectivity and hardware behavior need separate evidence.

Zion-Client is a C++17 embedded-Linux daemon that runs hardware verification workloads and reports results to Zion-Server. The repository provides a native CMake build, Makefile helpers, Docker cross-compilation support, sample configuration, a systemd unit, and GoogleTest-based tests. Its source areas cover configuration loading, an outbound server stream, a SQLite spool, run orchestration, a module registry, HAL bridging, a Python-SDK-facing gRPC service, log collection, cleanup, and diagnostics. The client sits between board capabilities exposed by Zion-HAL and fleet-facing services exposed by Zion-Server. Existing documentation describes bidirectional gRPC, heartbeat/reconnect behavior, protobuf result serialization, durable buffering during interrupted links, and module-specific adapter dispatch. Unsupported modules can use a stub path, so a requested test is not automatically evidence of a working hardware implementation. Configuration is TOML-based and includes server addressing, the HAL listener, spool limits, optional TLS files, and extra log paths. The documented validation path is a configured native build followed by `ctest --output-on-failure`; Docker targets cover ARM64 builds. This README records observable repository features, but does not claim that a board, server connection, or physical peripheral was manually exercised. Paths and commands below are starting points for repeatable verification.

## Feature Status

| # | Name / Description | Implemented? | Test Coverage? | Manual Test |
|---:|---|---|---|---|
| 1 | CMake/Make build and install layout | Implemented | Build configuration present; run required | [ ] Manual verification required |
| 2 | TOML client configuration and sample config | Implemented | Unit coverage not established | [ ] Manual verification required |
| 3 | Outbound gRPC stream to Zion-Server | Implemented | Test files exist; integration unknown | [ ] Manual verification required |
| 4 | SQLite result/log spool with capacity controls | Implemented | Serializer/spool evidence; full coverage unknown | [ ] Manual verification required |
| 5 | Run orchestration and adapter dispatch | Implemented | `test_run_orchestrator` documented; breadth unknown | [ ] Manual verification required |
| 6 | Protobuf result serialization | Implemented | `test_result_serializer` documented | [ ] Manual verification required |
| 7 | HAL gRPC service for SDK callers | Implemented | Test coverage not independently verified | [ ] Manual verification required |
| 8 | Log collection and forwarding | Partial | Source/config evidence; dedicated coverage unknown | [ ] Manual verification required |
| 9 | Module registry and supported adapters | Partial | Registry tests not confirmed | [ ] Manual verification required |
| 10 | Diagnostics socket and janitor cleanup | Partial | Dedicated test coverage unknown | [ ] Manual verification required |
| 11 | TLS configuration | Partial | Fields documented; end-to-end proof absent | [ ] Manual verification required |
| 12 | Systemd deployment | Implemented | Unit file present; boot not verified | [ ] Manual verification required |
| 13 | Docker cross-compilation helpers | Implemented | Scripts present; target build not run | [ ] Manual verification required |

## Tests and Commands

```bash
make build          # Docker cross-build (ARM64) — the default and only build path
make test           # ctest inside Docker
```

The repository documents `test_result_serializer` and `test_run_orchestrator`. A live test needs Zion-Server, Zion-HAL, configuration, and hardware or a representative environment.

## Limitations

Manual hardware, network, TLS, systemd, Docker, and cross-target verification remains outstanding. Documented module counts and platform claims are assertions until checked against the current build and board.

## Detailed Reference

The retained implementation guide below contains the repository's extended architecture, configuration, build, and runtime notes.
# Zion-Client > **gRPC-based daemon for IV&V (Independent Verification & Validation) hardware testing on embedded Linux platforms** Zion-Client is a C++17 daemon that orchestrates hardware validation tests, collects results, and streams them to an Zion-Server instance. It manages test execution, durable result storage, log collection, and provides gRPC endpoints for both server communication and SDK integration. --- ## Table of Contents - [Overview](#overview)
- [Architecture](#architecture)
- [System Requirements](#system-requirements)
- [Dependencies](#dependencies)
- [Building](#building) - [Native Build](#native-build) - [Docker Build (Cross-compilation)](#docker-build-cross-compilation)
- [Installation](#installation)
- [Configuration](#configuration)
- [Running](#running) - [Manual Execution](#manual-execution) - [Systemd Service](#systemd-service)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [Supported Hardware Modules](#supported-hardware-modules)
- [Development](#development)
- [Troubleshooting](#troubleshooting)
- [License](#license) --- ## Overview Zion-Client provides: - **Test Orchestration**: Executes hardware validation tests via 35+ module adapters (CPU, GPU, Thermal, Storage, USB, Ethernet, etc.)
- **Result Serialization**: Converts HAL test results to Protocol Buffer format with proper sequencing and timestamping
- **Durable Storage**: SQLite-based spool for result buffering and replay on network interruption
- **Server Streaming**: Bidirectional gRPC stream to Zion-Server for commands and result delivery
- **SDK Interface**: HalService gRPC endpoint for Python SDK and end-to-end test integration
- **Log Collection**: Tails specified log files and forwards entries to server
- **Diagnostic Socket**: Local diagnostic interface for health monitoring **Target Platforms**: Zynq UltraScale+ (Zynq UltraScale+), Zynq UltraScale+, or any ARM64/x86_64 Linux system with GPIO/hardware access. --- ## Architecture ```
┌─────────────────────────────────────────────────────────────┐
│ Zion-Client │
│ │
│ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ │
│ │ StreamClient │──│ Spool │──│RunOrchestrator│ │
│ │ (gRPC to │ │ (SQLite3) │ │ (Test Exec) │ │
│ │ Server) │ └──────────────┘ └───────┬───────┘ │
│ └──────────────┘ │ │
│ │ │
│ ┌──────────────┐ ┌──────────────┐ ┌───────▼───────┐ │
│ │ LogCollector │ │ DiagSocket │ │ Module │ │
│ │ │ │ │ │ Registry │ │
│ └──────────────┘ └──────────────┘ └───────┬───────┘ │
│ │ │
│ ┌──────────────┐ ┌───────▼───────┐ │
│ │ HalService │◄───────────────────│ 35+ Adapters │ │
│ │ (gRPC for │ │ (HAL Bridge) │ │
│ │ PySDK) │ └───────────────┘ │
│ └──────────────┘ │
└─────────────────────────────────────────────────────────────┘ ▲ │ │ │ │ ▼ Zion-Server Zion-HAL (gRPC Stream) (Hardware Abstraction)
``` ### Core Components | Component | Purpose |
|-----------|---------|
| **StreamClient** | Bidirectional gRPC stream to Zion-Server; handles reconnection and heartbeat |
| **Spool** | SQLite3 database for durable result/log storage; enforces capacity limits |
| **RunOrchestrator** | Executes test runs, invokes module adapters, serializes results |
| **ModuleRegistry** | Loads `modules.yaml` and dispatches tests to appropriate adapters |
| **HalService** | gRPC service (port 50052) for Python SDK to invoke HAL tests |
| **LogCollector** | Tails configured log files and forwards to server |
| **Janitor** | Periodic cleanup of old spool entries |
| **DiagSocket** | Unix socket for local diagnostics | --- ## System Requirements ### Hardware
- **Minimum**: ARM64 or x86_64 processor (Zynq UltraScale+, Zynq UltraScale+, or similar)
- **Recommended**: 1 GB RAM, 2 GB storage for spool database ### Operating System
- **Ubuntu 22.04 LTS** (primary target)
- **Ubuntu 20.04 LTS** (supported with manual dependency builds)
- Other Debian-based ARM64/x86_64 distributions (untested) ### Network
- Connectivity to **Zion-Server** (configurable IP/port)
- Optional: Access to hardware peripherals (GPIO, I2C, SPI, USB, etc.) --- ## Dependencies ### Build-time Dependencies | Library | Version | Purpose |
|---------|---------|---------|
| **CMake** | ≥ 3.16 | Build system |
| **GCC/G++** | ≥ 9.0 (C++17) | Compiler |
| **Protocol Buffers** | ≥ 3.12 | Message serialization |
| **gRPC** | ≥ 1.30 | RPC framework |
| **spdlog** | ≥ 1.x | Structured logging |
| **toml++** | ≥ 3.4 | Configuration parsing |
| **yaml-cpp** | ≥ 0.7 | Module registry parsing |
| **SQLite3** | ≥ 3.x | Spool database |
| **GoogleTest** | ≥ 1.10 | Unit testing | ### Runtime Dependencies | Dependency | Notes |
|------------|-------|
| **Zion-HAL** | Sibling repository (must be in `../Zion-HAL/`) |
| **libprotobuf.so** | Shared library for Protocol Buffers |
| **libgrpc++.so** | gRPC C++ runtime | --- ## Building ### Prerequisites 1. **Clone both repositories** (they must be siblings): ```bash mkdir -p ~/workspace/Zion cd ~/workspace/Zion git clone <zion-client-repo-url> Zion-Client git clone <zion-hal-repo-url> Zion-HAL ``` 2. **Install system dependencies**: **Ubuntu 22.04/ 20.04:** ```bash sudo apt-get update sudo apt-get install -y \ build-essential cmake git \ protobuf-compiler libprotobuf-dev \ libgrpc++-dev libgrpc-dev \ libsqlite3-dev libspdlog-dev \ libgtest-dev pkg-config ninja-build ``` 3. **Install header-only libraries** (if not packaged): **toml++** (required on Ubuntu 22.04 ARM64): ```bash git clone --depth=1 --branch v3.4.0 https://github.com/marzer/tomlplusplus.git/tmp/tomlplusplus cmake -S/tmp/tomlplusplus -B/tmp/tomlplusplus/build \ -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib sudo cmake --install/tmp/tomlplusplus/build rm -rf/tmp/tomlplusplus ``` **yaml-cpp 0.8.0** (if system version lacks CMake targets): ```bash git clone --depth=1 --branch 0.8.0 https://github.com/jbeder/yaml-cpp.git/tmp/yaml-cpp cmake -S/tmp/yaml-cpp -B/tmp/yaml-cpp/build \ -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib -DYAML_CPP_BUILD_TESTS=OFF cmake --build/tmp/yaml-cpp/build --parallel sudo cmake --install/tmp/yaml-cpp/build rm -rf/tmp/yaml-cpp ``` 4. **Build grpc_cpp_plugin** (if missing): ```bash # Check if grpc_cpp_plugin exists which grpc_cpp_plugin || { # Build from source (see Dockerfile for full build script) # Alternatively, install grpc-plugins package if available sudo apt-get install -y protobuf-compiler-grpc } ``` --- ### Native Build **Using Makefile** (recommended):
```bash
cd ~/workspace/Zion/Zion-Client
make docker-configure # Configure inside Docker
make build # Compile with Ninja
``` **Using CMake directly**:
```bash
cd ~/workspace/Zion/Zion-Client
mkdir -p build/cross
cd build/cross
cmake ../.. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
``` **Output**:
- Binary: `build/cross/bin/zion-client`
- Tests: `build/cross/bin/test_result_serializer`, `build/cross/bin/test_run_orchestrator` --- ### Docker Build (Cross-compilation) For cross-compiling from x86_64 to ARM64 (Zynq UltraScale+): 1. **Build Docker image**: ```bash cd ~/workspace/Zion/Zion-Client/docker make build # or: docker buildx build --platform linux/arm64 --load -t zion . ``` 2. **Enter build container**: ```bash make run # Interactive shell in ARM64 container ``` 3. **Build inside container**: ```bash # Inside container: cd/workspace/Zion-Client make docker-configure make docker-build ``` 4. **Extract binaries**: ```bash # On host: cd ~/workspace/Zion/Zion-Client docker run --rm -v $(pwd):/host zion \ cp -r/workspace/Zion-Client/build/cross/bin/host/build/cross/ ``` --- ## Installation ### Manual Installation 1. **Copy binary**: ```bash sudo install -m 0755 build/cross/bin/zion-client/usr/bin/zion-client ``` 2. **Create configuration directory**: ```bash sudo mkdir -p/etc/zion sudo cp conf/client.conf.sample/etc/zion/client.conf sudo chown root:root/etc/zion/client.conf sudo chmod 0644/etc/zion/client.conf ``` 3. **Create spool directory**: ```bash sudo mkdir -p/var/lib/zion/spool sudo chown zion:zion/var/lib/zion/spool # See user setup below ``` 4. **Create zion user** (for systemd service): ```bash sudo useradd -r -s/usr/sbin/nologin -d/var/lib/zion zion sudo chown -R zion:zion/var/lib/zion ``` 5. **Install systemd service**: ```bash sudo cp systemd/zion-client.service/etc/systemd/system/ sudo systemctl daemon-reload sudo systemctl enable zion-client # start automatically at boot sudo systemctl start zion-client # start now ``` Or run everything in one step with the installer: ```bash sudo ./systemd/install.sh # add --no-start to skip starting immediately ``` --- ### CMake Install Alternatively, use the CMake install target:
```bash
cd build/cross
sudo cmake --install .
``` This installs:
- Binary → `/usr/bin/zion-client`
- Config → `/etc/zion/client.conf`
- Service → `/lib/systemd/system/zion-client.service` --- ## Configuration Edit `/etc/zion/client.conf`: ```toml
# Zion-Client configuration # ---------------------------------------------------------------------------
# gRPC server (outbound — connects to Zion-Server)
# ---------------------------------------------------------------------------
server_address = "192.168.1.100" # REQUIRED — IP or hostname of Zion-Server
server_port = 50051 # ---------------------------------------------------------------------------
# HalService listener (inbound — PySDK connects to this Client port)
# ---------------------------------------------------------------------------
# hal_service_bind = "0.0.0.0" # bind address; "127.0.0.1" for loopback only
# hal_service_port = 50052 # default 50052 # ---------------------------------------------------------------------------
# Spool (durability store)
# ---------------------------------------------------------------------------
spool_path = "/var/lib/zion/spool/spool.db"
result_spool_cap_bytes = 2147483648 # 2 GiB
log_spool_cap_bytes = 524288000 # 500 MiB # ---------------------------------------------------------------------------
# TLS (leave empty for plaintext — lab-subnet default)
# ---------------------------------------------------------------------------
# tls_ca = "/etc/zion/ca.crt"
# tls_cert = "/etc/zion/client.crt"
# tls_key = "/etc/zion/client.key" # ---------------------------------------------------------------------------
# Extra log paths to tail and forward to server
# ---------------------------------------------------------------------------
# log_paths = [
# "/var/log/zion/hal.log",
# ]
``` **Required fields**:
- `server_address`: IP/hostname of Zion-Server instance **Optional**:
- `hal_service_port`: Change if port 50052 conflicts
- `tls_*`: Enable TLS for production deployments
- `log_paths`: Additional log files to tail (e.g., HAL library logs) --- ## Running ### Manual Execution **Development run** (foreground):
```bash/usr/bin/zion-client --config/etc/zion/client.conf
``` **With debug logging**:
```bash
SPDLOG_LEVEL=debug/usr/bin/zion-client --config/etc/zion/client.conf
``` The daemon will:
1. Load configuration from `/etc/zion/client.conf`
2. Initialize spool database
3. Connect to Zion-Server (retry on failure)
4. Start HalService listener on port 50052
5. Begin heartbeat loop **Graceful shutdown**: Send `SIGTERM` or `SIGINT` (Ctrl+C) --- ### Systemd Service **Enable and start**:
```bash
sudo systemctl enable zion-client.service
sudo systemctl start zion-client.service
``` **Check status**:
```bash
sudo systemctl status zion-client.service
``` **View logs**:
```bash
sudo journalctl -u zion-client.service -f # Follow logs
sudo journalctl -u zion-client.service -n 100 # Last 100 lines
``` **Stop service**:
```bash
sudo systemctl stop zion-client.service
``` **Restart after config changes**:
```bash
sudo systemctl restart zion-client.service
``` --- ## Testing ### Run Unit Tests ```bash
cd build/cross
ctest --output-on-failure
``` Or run individual test binaries:
```bash
./bin/test_result_serializer
./bin/test_run_orchestrator
``` ### Available Tests | Test | Purpose |
|------|---------|
| `test_result_serializer` | Validates HAL → Protobuf serialization |
| `test_run_orchestrator` | Tests orchestration logic and batching | ### Manual Integration Test 1. **Start Zion-Server** (on remote machine or localhost)
2. **Configure client** to point to server IP
3. **Run client** with `--config` flag
4. **Trigger test via SDK**: ```python # Python SDK example from zion_sdk import Client client = Client("localhost:50052") result = client.run_test("cpu", "frequency_test") print(result) ``` --- ## Project Structure ```
Zion-Client/
├── CMakeLists.txt # Root build configuration
├── Makefile # High-level build commands
├── Doxyfile.in # Doxygen documentation config
├── src/
│ ├── main.cpp # Entry point
│ ├── app.h/ app.cpp # Top-level App object
│ ├── common/ # Shared utilities
│ ├── config/ # Configuration parsing (TOML)
│ ├── stream/ # StreamClient (gRPC to server)
│ ├── spool/ # Spool (SQLite3 storage)
│ ├── run/ # RunOrchestrator (test execution)
│ ├── registry/ # ModuleRegistry + 35+ adapters
│ │ ├── AdapterBase.h # Base class for all adapters
│ │ ├── AdapterRegistry.h # Adapter registration/lookup
│ │ └── adapters/ # Module-specific adapters
│ │ ├── cpu_adapter.h
│ │ ├── gpu_adapter.h
│ │ ├── thermal_adapter.h
│ │ └── ... (35+ total)
│ ├── hal_service/ # HalService gRPC endpoint
│ ├── hal_bridge/ # ResultSerializer (HAL → Protobuf)
│ ├── log/ # LogCollector
│ ├── janitor/ # Cleanup tasks
│ └── diag/ # DiagSocket
├── tests/ # Unit tests (GoogleTest)
├── conf/
│ └── client.conf.sample # Sample configuration
├── systemd/
│ └── zion-client.service # Systemd unit file
├── docker/
│ ├── Dockerfile # ARM64 build container
│ └── Makefile # Docker build helpers
└── client_update/ # Design documents ├── 01_discovery.md ├── 02_plan.md └── ...
``` --- ## Supported Hardware Modules Zion-Client includes adapters for **35 hardware modules** (with 16 additional modules planned): ### Active Modules (35) | Category | Modules |
|----------|---------|
| **Compute** | CPU, GPU, Deep Learning Accelerator, Vision Accelerator |
| **Memory** | System Memory, Storage |
| **Thermal** | Thermal, Environmental |
| **Power** | Power Management, Watchdog and Reset |
| **Networking** | Ethernet, Ethernet Switch, Communication Interface |
| **Connectivity** | USB, PCIe, External Connector |
| **Peripherals** | GPIO, Camera Interfaces, Camera Module 3, Display Interfaces |
| **Sensors** | IMU, GNSS |
| **Audio/Video** | Audio Interfaces, Video Encode/Decode |
| **System** | Clock and Timing, Software Interface, System Integration |
| **Safety** | Functional Safety, Reliability, Security Features |
| **Debug** | Debug and Development Interfaces, MCU Interface |
| **Specialized** | Serializer/Deserializer, PoE HAT, Sense HAT | ### Phase 3 Modules (16 — Planned) Accelerometer, Analog Input, Battery, Bluetooth, CAN Bus, eMMC, I²C, MIPI CSI, MIPI DSI, NVMe, PWM Output, RS-485, RTC, SD Card, SPI, UART ### Stub Adapter Unrecognized modules fall back to `StubAdapter`, which logs test invocations but returns honest SKIPPED results. --- ## Development ### Coding Standards - **Language**: C++17
- **Style**: Follow existing conventions (see `.clang-format` if present)
- **Logging**: Use `spdlog` macros (`spdlog::info`, `spdlog::error`, etc.)
- **Error Handling**: Return `bool` or `Status` from init methods; log errors before returning ### Adding a New Module Adapter 1. **Create adapter files**: ```bash touch src/registry/adapters/my_module_adapter.h touch src/registry/adapters/my_module_adapter.cc ``` 2. **Implement `AdapterBase` interface**: ```cpp// my_module_adapter.h #pragma once #include "registry/AdapterBase.h" class MyModuleAdapter : public AdapterBase { public: std::string module_id const override { return "my_module"; } hal::TestResult run_test(const std::string& test_id, const TestParameters& params) override; }; ``` 3. **Register in `app.cpp`**: ```cpp #include "registry/adapters/my_module_adapter.h"// ... reg->register_adapter(std::make_unique<MyModuleAdapter>); ``` 4. **Build and test**: ```bash make build ./build/cross/bin/zion-client --config/etc/zion/client.conf ``` --- ## Troubleshooting ### Common Issues #### 1. **gRPC connection fails** ``` [ERROR] StreamClient failed to connect to 192.168.1.100:50051 ``` **Solution**: - Verify `server_address` in `/etc/zion/client.conf` - Check network connectivity: `ping 192.168.1.100` - Ensure Zion-Server is running and reachable - Check firewall rules: `sudo ufw status` #### 2. **Missing `grpc_cpp_plugin`** ``` CMake Error: grpc_cpp_plugin not found ``` **Solution**: ```bash sudo apt-get install -y protobuf-compiler-grpc # OR build from source (see Dockerfile) ``` #### 3. **Spool database locked** ``` [ERROR] Spool::init failed: database is locked ``` **Solution**: - Stop conflicting instance: `sudo systemctl stop zion-client` - Check for stale processes: `ps aux | grep zion-client` - Remove lock if safe: `rm/var/lib/zion/spool/spool.db-shm` #### 4. **Permission denied on `/var/lib/zion`** ``` [ERROR] Failed to create spool.db: Permission denied ``` **Solution**: ```bash sudo chown -R zion:zion/var/lib/zion sudo chmod 0755/var/lib/zion ``` #### 5. **HAL library not found** ``` CMake Error: Could not find Zion-HAL ``` **Solution**: - Ensure `Zion-HAL` is in sibling directory: ```bash ls -ld ../Zion-HAL # Should exist ``` - Clone HAL repository: `git clone <hal-repo> ../Zion-HAL` #### 6. **Port 50052 already in use** ``` [ERROR] HalService failed to bind: Address already in use ``` **Solution**: - Check what's using the port: `sudo lsof -i :50052` - Change `hal_service_port` in config, or kill conflicting process --- ### Debug Logging Enable verbose logging:
```bash
SPDLOG_LEVEL=debug/usr/bin/zion-client --config/etc/zion/client.conf
``` Or modify `main.cpp` and rebuild:
```cpp
spdlog::set_level(spdlog::level::debug);// Change from ::info
``` --- ### Getting Help - **Check logs**: `journalctl -u zion-client.service -n 200`
- **Inspect spool**: `sqlite3/var/lib/zion/spool/spool.db ".tables"`
- **Test connectivity**: `grpcurl -plaintext <server_ip>:50051 list`
- **GitHub Issues**: Report bugs with full logs and config (redact sensitive data) --- ## License

MIT. See [LICENSE](LICENSE). --- ## Contributing 1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-adapter`)
3. Commit changes (`git commit -am 'Add XYZ adapter'`)
4. Push to branch (`git push origin feature/my-adapter`)
5. Open a Pull Request **Guidelines**:
- Add tests for new functionality
- Update documentation
- Follow existing code style
- Ensure CI passes --- ## Acknowledgments - Built for IV&V hardware validation on Zynq UltraScale+ and Zynq UltraScale+ platforms
- Uses gRPC for efficient client-server communication
- Protocol Buffers for portable serialization
- SQLite3 for durable local storage --- **Questions?** Contact the Zion development team or file an issue on GitHub.
