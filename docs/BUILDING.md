# Building nucleo-753-can-eth Firmware

This guide provides detailed instructions for building the firmware using Docker or local toolchains.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Docker Build (Recommended)](#docker-build-recommended)
- [Local Build](#local-build)
- [CMake Presets](#cmake-presets)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Required Tools

- **CMake** 3.25 or later
- **Ninja** build system
- **Git** for version control

### Toolchain Options

**Option 1: Docker (Recommended)**
- Docker installed and running
- Access to `ghcr.io/kodezine/kdocker:latest` container
- Includes ARM GCC 14.3 and ARM Compiler for Embedded 21.1

**Option 2: Local Toolchain**
- ARM GCC 14.3 installed at `$HOME/gnuarm14.3`
- Download from: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

## Docker Build (Recommended)

Docker provides a consistent build environment matching CI/CD.

### Basic Docker Build

```bash
# Pull the latest Docker image
docker pull ghcr.io/kodezine/kdocker:latest

# Build with ARM GCC 14.3
docker run --rm \
  -v $(pwd):/workspace \
  -w /workspace \
  ghcr.io/kodezine/kdocker:latest \
  bash -c "cmake --preset nucleo-753-can-eth-gnuarm14.3 && \
           cmake --build --preset nucleo-753-can-eth-gnuarm14.3"

# Build with ARM Compiler for Embedded 21.1
docker run --rm \
  -v $(pwd):/workspace \
  -w /workspace \
  ghcr.io/kodezine/kdocker:latest \
  bash -c "cmake --preset nucleo-753-can-eth-atfe21.1 && \
           cmake --build --preset nucleo-753-can-eth-atfe21.1"
```

### Docker Build with User Permissions (Linux)

To avoid permission issues on Linux, run Docker with your user ID:

```bash
docker run --rm \
  --user $(id -u):$(id -g) \
  -v $(pwd):/workspace \
  -w /workspace \
  ghcr.io/kodezine/kdocker:latest \
  bash -c "cmake --preset nucleo-753-can-eth-gnuarm14.3 && \
           cmake --build --preset nucleo-753-can-eth-gnuarm14.3"
```

### Interactive Docker Session

For development and debugging:

```bash
docker run --rm -it \
  -v $(pwd):/workspace \
  -w /workspace \
  ghcr.io/kodezine/kdocker:latest \
  /bin/bash

# Inside the container:
cmake --preset nucleo-753-can-eth-gnuarm14.3
cmake --build --preset nucleo-753-can-eth-gnuarm14.3
```

## Local Build

### Setup

1. **Install ARM GCC 14.3**:
   ```bash
   cd $HOME
   wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
   tar xf arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
   mv arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi gnuarm14.3
   ```

2. **Create User Presets**:
   ```bash
   cp CMakeUserPresetsTemplate.json CMakeUserPresets.json
   ```

3. **Edit CMakeUserPresets.json** (optional):
   - Default toolchain path: `$HOME/gnuarm14.3`
   - Modify `COMPILER_ROOT_PATH` if installed elsewhere

### Build Commands

```bash
# Configure
cmake --preset nucleo-753-can-eth-local-gnuarm14.3

# Build
cmake --build --preset nucleo-753-can-eth-local-gnuarm14.3

# Clean build
cmake --build --preset nucleo-753-can-eth-local-gnuarm14.3 --target clean
```

### Build Output

Artifacts are generated in `build/nucleo-753-can-eth-local-gnuarm14.3/`:

```
nucleo-753-can-eth.elf    # ELF executable (114 KB typical)
nucleo-753-can-eth.hex    # Intel HEX format
nucleo-753-can-eth.bin    # Raw binary format  
nucleo-753-can-eth.map    # Linker map file
```

Size report is displayed automatically:

```
   text    data     bss     dec     hex filename
 112384    2048   67656  182088   2c778 nucleo-753-can-eth.elf
```

## CMake Presets

### Available Presets

| Preset | Toolchain | Environment | Description |
|--------|-----------|-------------|-------------|
| `nucleo-753-can-eth-gnuarm14.3` | ARM GCC 14.3 | Docker | CI/CD preset |
| `nucleo-753-can-eth-atfe21.1` | ARM Clang 21.1 | Docker | CI/CD preset |
| `nucleo-753-can-eth-local-gnuarm14.3` | ARM GCC 14.3 | Local | Development preset |

### Preset Inheritance

Presets use inheritance for modularity:

```
versions (CPM v0.42.0)
  └─> cpm (FetchContent config)
      └─> debug (Debug build + compile_commands.json)
          └─> mcu (STM32H753xx, Cortex-M7)
              └─> gnuarm14.3 / atfe21.1 (Toolchain)
                  └─> Final preset
```

### Custom Presets

Add custom presets in `CMakeUserPresets.json`:

```json
{
  "configurePresets": [
    {
      "name": "my-custom-preset",
      "inherits": ["debug", "mcu", "local-gnuarm14.3"],
      "cacheVariables": {
        "MY_CUSTOM_OPTION": "ON"
      }
    }
  ]
}
```

## Build Options

### CMake Cache Variables

Set via presets or command line:

```bash
cmake --preset nucleo-753-can-eth-local-gnuarm14.3 \
  -DCMAKE_BUILD_TYPE=Release
```

| Variable | Default | Description |
|----------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug` | Build type (Debug/Release) |
| `TOOLCHAIN_VARIANT` | `gnuarm14.3` | Toolchain selection |
| `FETCHCONTENT_UPDATES_DISCONNECTED` | `ON` | Cache dependencies |

### Dependency Management

Dependencies are fetched to `_deps/` on first configure:

```
_deps/
├── cpm/                    # CPM package manager
├── cmake_scripts-src/      # Toolchain files (v1.0.8)
├── nucleo-hal-src/         # hal753 package (version from GITHUB_BRANCH_hal753)
└── nucleo-hal-build/       # Extracted package contents
```

**Version Configuration** (`CMakePresets/VersionPresets.json`):
```json
"GITHUB_BRANCH_hal753": "v1.0.1",
"GITHUB_BRANCH_hal753_SHA256": "b7763477739c3dc7180293e3fe0d049ff653cb05e1ab6ac69152fe94a5142945",
"GITHUB_BRANCH_toolchain": "v1.0.8"
```

To force re-download:

```bash
rm -rf _deps/nucleo-hal-*
cmake --preset <preset-name>
```

### Verbose Build

```bash
cmake --build --preset nucleo-753-can-eth-gnuarm14.3 --verbose
```

## Troubleshooting

### hal753 Package Download Fails

**Error**: "Failed to download nucleo-hal-<version>-<toolchain>.tar.gz"

**Solution**:
- Check internet connection
- Verify GitHub is accessible
- Verify `GITHUB_BRANCH_hal753` and `GITHUB_BRANCH_hal753_SHA256` in `CMakePresets/VersionPresets.json`
- Check release exists: https://github.com/sohal/hal753/releases
- Try manual download:
  ```bash
  # Example for v1.0.1 gnuarm14.3
  wget https://github.com/sohal/hal753/releases/download/v1.0.1/nucleo-hal-1.0.1-gnuarm14.3.tar.gz
  # Verify checksum
  sha256sum nucleo-hal-1.0.1-gnuarm14.3.tar.gz
  ```

### Toolchain Not Found

**Error**: "Could not find toolchain file"

**Solution**:
- Verify `COMPILER_ROOT_PATH` in preset
- Check toolchain installation:
  ```bash
  ls $HOME/gnuarm14.3/bin/arm-none-eabi-gcc
  ```
- Ensure `CMAKE_TOOLCHAIN_FILE` points to correct location

### Linker Script Not Found

**Error**: "cannot open linker script file"

**Solution**:
- Verify `linker/STM32H753XX_FLASH.ld` exists
- Check file permissions
- Re-copy from hal753:
  ```bash
  cp /path/to/hal753/nucleo-h753/STM32H753XX_FLASH.ld linker/
  ```

### Permission Denied (Docker on Linux)

**Error**: "Permission denied" when accessing build artifacts

**Solution**: Run Docker with user ID:
```bash
docker run --rm --user $(id -u):$(id -g) ...
```

### CMake Version Too Old

**Error**: "CMake 3.25 or higher is required"

**Solution**: Update CMake:
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# Or use pip
pip install --upgrade cmake
```

### Ninja Not Found

**Error**: "Could not find Ninja"

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install ninja-build

# macOS
brew install ninja
```

## Next Steps

- [Flash firmware to hardware](FLASHING.md)
- [Debug with GDB](DEBUGGING.md)
- [Extend with custom Active Objects](EXTENDING_ACTIVE_OBJECTS.md)
- [Optimize memory usage](MEMORY_OPTIMIZATION.md)
