# Open M-Plane Developer Guide

## VM-first Developer Build

For the CloudlyRANbeta POC, use the Ubuntu 22.04 Vagrant workflow from the
parent repo and the scripts in `dev/`. This avoids the Yocto image path for
normal iteration on `mplane_client`, `mplane_server`, and `libhalmplane`.

```bash
cd /workspace/cloudlyRANbeta/open-mplane
./dev/scripts/bootstrap_ubuntu22.sh
./dev/scripts/build_all.sh
./dev/scripts/validate_build.sh
```

The Yocto `meta-mplane` workflow remains available for target images and
hardware deployment.

The validated development environment is the parent repo's VirtualBox-backed
Ubuntu 22.04 VM. Docker can support containerized experiments, but it is not
the primary Open M-Plane dev path for this POC because the server workflow
depends on sysrepo/netopeer2 runtime behavior and local shared-library wiring.

Open M-Plane implements portions of the O-RAN Alliance Management Plane (M-Plane) used to manage Open Fronthaul radio units. This document summarizes the project layout and describes how to build and run the software.

## Repository Overview
- **meta-mplane** – Yocto layer and build scripts used to produce runnable images.
- **libhalmplane** – C library that implements the hardware abstraction layer (HAL). Implementations exist for **ZCU111**, **x86**, and a **modular** board type.
- **mplane_server** – Stand‑alone M-Plane server application.
- **mplane_client** – Client software used to communicate with the server.
- **climp** – Command line interface for invoking HAL functions directly.

Implementation details for each component can be found in the respective `IMPLEMENTATION.md` files within the subdirectories.

## Building the Yocto Image
1. Install build dependencies (example for Ubuntu):
   ```bash
   sudo apt install build-essential chrpath diffstat
   ```
2. Sync Yocto layers and set up the build environment:
   ```bash
   cd meta-mplane
   ./sync_yocto.sh
   source fb-oru-init-build-env meta-x86
   ```
3. Build the image:
   ```bash
   bitbake mplane-image-x86
   ```

The same process can be repeated with `meta-armv8` for ARM targets or the `petalinux` configuration for the ZCU111 board.

## Preparing Hardware and Running the Server
1. Create a target directory for the root filesystem and copy the build output:
   ```bash
   mkdir -p ../mplane-server/var/volatile/log
   rsync -racvP --stats build/tmp/work/*/mplane-image-x86/*/rootfs/ ../mplane-server/
   ```
2. Mount system directories and chroot into the environment:
   ```bash
   cd ../mplane-server
   for i in /dev /proc /run /sys; do sudo mount --bind $i ${i:1}; done
   sudo chroot $(pwd) /bin/bash
   ```
3. Inside the chroot, run the setup scripts installed by Yocto:
   ```bash
   usr/share/netopeer2/scripts/setup.sh
   usr/share/netopeer2/scripts/merge_hostkey.sh
   usr/share/netopeer2/scripts/merge_config.sh
   usr/share/mplane-server/scripts/o-ran-user-config.sh
   ```
4. Start the server:
   ```bash
   mplane-server-app \
     --cfg-data-path /usr/share/mplane-server \
     --netopeer-path /usr/local/bin \
     --yang-mods-path /usr/share/mplane-server/modules --netopeerdbg 2
   ```

## Building and Running the Client
1. Fetch and build dependencies:
   ```bash
   cd mplane_client/utils
   ./get_deps.sh --no-fwdproxy --dir ../
   ./build_deps.sh --no-netopeer2 --dir ../
   ./build_mpclient.sh --parallel 2
   ```
2. Run the client and demo tools:
   ```bash
   cd ../build
   ./wrapper.sh ./mpc_client            # starts the RPC server
   ./wrapper.sh ./mpclient-demo         # example CLI client
   ```

## Further Documentation
- [meta-mplane/README.md](meta-mplane/README.md) – Yocto build usage
- [libhalmplane/README.md](libhalmplane/README.md) – HAL overview
- [mplane_server/README.md](mplane_server/README.md) – server options
- [mplane_client/README.md](mplane_client/README.md) – client usage
- [climp/README.md](climp/README.md) – CLI details

For deeper design notes see the `IMPLEMENTATION.md` files within `mplane_server`, `mplane_client`, and `climp`.
