# Zion-Client Build and Deployment

## Purpose and scope
Zion-Client is the C++17 hardware-test daemon. It builds the client executable, generated protobuf/gRPC code, the sibling Zion-HAL tree, and GoogleTest binaries. It connects outbound to Zion-Server and exposes the HalService listener used by Zion-PySDK.

All commands below are run from this directory. The Makefile is authoritative. Native output is under `build/native`; Docker cross output is under `build/cross`.

## 1. Prerequisites
- Linux with CMake >= 3.16, Ninja, a C++17 compiler, Make, and `nproc`.
- Protobuf and gRPC development packages plus `protoc` and `grpc_cpp_plugin`, SQLite3, spdlog, toml++, yaml-cpp, Threads, and GoogleTest.
- A sibling `../Zion-HAL` checkout is required by the top-level CMakeLists.txt.
- Docker/buildx is required for ARM64 targets; `sshpass`, SSH/SCP access, and a reachable target are required for deployment.

The repository Dockerfile supplies the ARM64 toolchain and builds toml++, yaml-cpp, and the gRPC plugin. Docker targets require network access on first image build.

## 2. Setup
Install the prerequisites and ensure `../Zion-HAL` is checked out as a sibling.

## 3. Build and compile
```bash
make configure
make build
```
`configure` uses CMake with Ninja and `-DBUILD_TESTS=ON`; `build` invokes Ninja with `JOBS` (default `nproc`). For Docker/ARM64:
```bash
make docker-install   # conditional; may use sudo apt
make docker-image
make docker-configure
make docker-build
```
`make all` is the default alias for the native `build` target. `docker-build` uses `linux/arm64`, QEMU/buildx, and the `zion` image.

## 4. Test and lint
```bash
make test
make format
make cppcheck
make doc
```
`test` runs CTest filtered to `serializer_tests` and `runner_tests` in the native build. `format` needs clang-format and cmake-format; `cppcheck` needs cppcheck; `doc` needs Doxygen and graphviz. Docker validation is `make docker-test` and runs CTest in the ARM64 container.

## 5. Packaging and artifacts
Native binaries and libraries are in `build/native/bin` and `build/native/lib`; cross artifacts are in `build/cross/bin` and `build/cross/lib`. `make list` lists those directories. CMake installs `zion-client`, the sample config as `client.conf`, and the systemd unit; use `cd build/native && sudo cmake --install .`.

## 6. Deployment, install, and run
```bash
make run                         # local native apps
make deploy-a IP_A=<target-ip>   # Docker build, then copy lib/bin
make deploy-b IP_B=<target-ip>
make deploy                       # both targets
make run-a IP_A=<target-ip>       # deploy then run remote apps
make run-b IP_B=<target-ip>
```
Remote commands use `USER`, `PASS`, `sshpass`, and `/tmp/acu-test`; override credentials with `USER=... PASS=...` and never commit them. Remote targets need SSH/SCP access and compatible hardware. `make run [app] [args...]` runs a selected native test application; no app runs all applications.

For a host install, create the `zion` service user and writable `/var/lib/zion/spool`, install `conf/client.conf.sample` to `/etc/zion/client.conf`, then enable `systemd/zion-client.service`. Set `server_address`, ports, spool paths, and optional TLS files before starting the service. `run.sh` is a development helper and expects `build/bin/zion-client` and `conf/client.conf.dev`, so it may not match the Makefile's `build/native` layout without local adjustment.

## 7. Cleanup
```bash
make clean
make docker-clean       # conditional; requires the Docker image
```
`clean` removes `build`; `docker-clean` removes the container cross build. `docker-shell` opens the ARM64 development shell.

## 8. Hardware, cloud, and manual verification caveats
A successful compile does not verify Zion-HAL drivers, peripherals, kernel device nodes, TLS, server connectivity, or real test results. Native tests are local; Docker tests use ARM64 emulation. Deployment and `run-a`/`run-b` require credentials, network access, and the target board. Systemd startup, permissions, spool recovery, HalService port 50052, and the outbound Server stream require manual verification.
