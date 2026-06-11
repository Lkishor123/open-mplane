# Open M-Plane VM Developer Build

This folder contains the development build path used by the CloudlyRANbeta POC.
It bypasses the Yocto `meta-mplane` image workflow for day-to-day development
and builds the pieces needed for the mock-RU flow in an Ubuntu 22.04 VM.

## Target

```text
mplane_client -> mplane_server -> libhalmplane modular HAL -> mock/log behavior
```

## Ubuntu Version

Use Ubuntu 22.04 LTS. The root CloudlyRANbeta repo provides a Vagrant VM for
this exact environment.

## Workflow

From inside the Vagrant VM:

```sh
cd /workspace/cloudlyRANbeta/open-mplane
./dev/scripts/bootstrap_ubuntu22.sh
./dev/scripts/build_all.sh
./dev/scripts/validate_build.sh
```

`bootstrap_ubuntu22.sh` installs the source-built NETCONF/gRPC dependency stack
under `mplane_client/deps/install`. The scripts do not install Open M-Plane
artifacts into system paths.

## Build Outputs

- `mplane_client/build/mpc_client`
- `mplane_client/build/mpclient-demo`
- `mplane_client/build/mpc_tester`
- `build/dev/install/lib/libhalmplane.so`
- `build/dev/install/sbin/mplane-server-app`
- `build/dev/install/share/mplane-server/YangConfig.xml`
- `build/dev/install/share/mplane-server/modules/*.yang`

## Environment

Load the development environment explicitly when opening a new shell:

```sh
. ./dev/scripts/env.sh
```

The Vagrant VM also sources this file from `/etc/profile.d/open-mplane-dev.sh`.

## Why This Exists

The upstream build is centered on `meta-mplane` and Yocto image generation. That
is useful for target hardware, but slow and awkward for the pre-sales POC where
developers need to iterate on `mplane_client`, `mplane_server`, and
`libhalmplane` in a mounted VM workspace.
