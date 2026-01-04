# nucleo-753-can-eth

[![Firmware Build](https://github.com/sohal/nucleo-753-can-eth/actions/workflows/cmake.yml/badge.svg)](https://github.com/sohal/nucleo-753-can-eth/actions/workflows/cmake.yml)
[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](LICENSE)

Firmware executable for STM32H753 Nucleo board with CAN and Ethernet peripheral support, built using the [hal753](https://github.com/sohal/hal753) hardware abstraction library.

## Overview

This project demonstrates a complete firmware application using:
- **hal753 library** (v1.0.1) - STM32H7 HAL drivers and board support
- **QP/C framework** (v8.1.1) - Real-time event-driven architecture
- **Mongoose** - Embedded network stack for HTTP/HTTPS
- **Active Objects** - Blinky LED, network manager, stack monitor

The firmware uses QP/C's Active Object pattern with the QK preemptive kernel for efficient multitasking without an RTOS.

## Features

- ✅ LED heartbeat indicator (Blinky AO)
- ✅ Mongoose HTTP/HTTPS server (Mongoose AO)  
- ✅ Stack usage monitoring with 90% threshold alerts (Stack Monitor AO)
- ✅ Ethernet connectivity via STM32H7 ETH peripheral
- ✅ CAN bus support (ready for extension)
- ✅ Berkeley socket-style size reports
- ✅ Complete firmware artifacts (.elf, .hex, .bin, .map)

## Quick Start

### Prerequisites

- **Docker**: [ghcr.io/kodezine/kdocker:latest](https://github.com/kodezine/kdocker) (recommended)
- **OR Local Toolchain**: ARM GCC 14.3 installed at `$HOME/gnuarm14.3`
- **CMake**: 3.25 or later
- **Ninja**: Build system

### Build with Docker (Recommended)

```bash
# Clone the repository
git clone https://github.com/sohal/nucleo-753-can-eth.git
cd nucleo-753-can-eth

# Build using Docker
docker run --rm \
  -v $(pwd):/workspace \
  -w /workspace \
  ghcr.io/kodezine/kdocker:latest \
  bash -c "cmake --preset nucleo-753-can-eth-gnuarm14.3 && cmake --build --preset nucleo-753-can-eth-gnuarm14.3"

# Firmware artifacts are in: build/nucleo-753-can-eth-gnuarm14.3/
```

### Build Locally

```bash
# Copy user presets template
cp CMakeUserPresetsTemplate.json CMakeUserPresets.json

# Edit CMakeUserPresets.json to set your toolchain path if needed

# Configure and build
cmake --preset nucleo-753-can-eth-local-gnuarm14.3
cmake --build --preset nucleo-753-can-eth-local-gnuarm14.3

# Firmware artifacts are in: build/nucleo-753-can-eth-local-gnuarm14.3/
```

## Build Artifacts

After a successful build:

```
build/nucleo-753-can-eth-gnuarm14.3/
├── nucleo-753-can-eth.elf    # ELF executable for debugging
├── nucleo-753-can-eth.hex    # Intel HEX for flashing
├── nucleo-753-can-eth.bin    # Raw binary for flashing
└── nucleo-753-can-eth.map    # Linker map file for analysis
```

## Documentation

Comprehensive documentation is available in the [`docs/`](docs/) directory:

- **[BUILDING.md](docs/BUILDING.md)** - Detailed build instructions (Docker, local, presets)
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System architecture, call chains, AO interaction
- **[EXTENDING_ACTIVE_OBJECTS.md](docs/EXTENDING_ACTIVE_OBJECTS.md)** - How to add custom Active Objects
- **[FLASHING.md](docs/FLASHING.md)** - Programming firmware to hardware
- **[DEBUGGING.md](docs/DEBUGGING.md)** - GDB and OpenOCD setup
- **[MEMORY_OPTIMIZATION.md](docs/MEMORY_OPTIMIZATION.md)** - Map file analysis and optimization

## Project Structure

```
nucleo-753-can-eth/
├── ao/                       # Active Objects
│   ├── blinky/              # LED heartbeat AO
│   ├── mongoose/            # Network manager AO
│   ├── stack_monitor/       # Stack usage monitor AO
│   └── common/              # Shared headers (signals, BSP)
├── frameworks/
│   ├── mongoose/            # Mongoose network stack
│   └── qpc/                 # QP/C adapter with application_init()
├── linker/
│   ├── STM32H753XX_FLASH.ld # Linker script
│   └── startup_stm32h753xx.s # Startup code
├── cmake/                   # CMake modules
├── CMakePresets/            # Build configurations
├── docs/                    # Documentation
└── .github/workflows/       # CI/CD pipelines
```

## Active Objects

The firmware includes three pre-configured Active Objects:

| AO | Priority | Purpose | Signals |
|----|----------|---------|---------|
| **Blinky** | 1 | LED heartbeat (1Hz) | `BLINKY_TIMEOUT_SIG` |
| **Mongoose** | 2 | HTTP/HTTPS server (200Hz polling) | `MONGOOSE_POLL_SIG` |
| **Stack Monitor** | 3 | Stack watermark check (10s interval) | `STACK_CHECK_SIG` |

See [EXTENDING_ACTIVE_OBJECTS.md](docs/EXTENDING_ACTIVE_OBJECTS.md) for adding custom AOs.

## Entry Point

The firmware uses a unique entry point architecture:

```
main() (from hal753 library)
  └─> Peripheral initialization (GPIO, ETH, RNG, etc.)
      └─> mongoose_init()
          └─> application_init() (frameworks/qpc/qpc-adapter.c)
              └─> QF_init()
              └─> Start Active Objects
              └─> QF_run() [never returns - QK kernel takes control]
```

No custom `main()` is needed - the hal753 library provides it.

## Dependencies

- **hal753 library**: v1.0.1 ([GitHub Release](https://github.com/sohal/hal753/releases/tag/v1.0.1))
- **QP/C framework**: 8.1.1 (included in hal753)
- **STM32CubeH7**: 1.12.1 (included in hal753)
- **Mongoose**: Latest embedded version

Dependencies are automatically fetched via CPM during configuration.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) for details.

Copyright (c) 2025, kodezine

## Contributing

1. Fork the repository
2. Create a feature branch
3. Install pre-commit hooks: `pre-commit install`
4. Make your changes (format with `pre-commit run --all-files`)
5. Submit a pull request

## Support

- **Issues**: [GitHub Issues](https://github.com/sohal/nucleo-753-can-eth/issues)
- **Discussions**: [GitHub Discussions](https://github.com/sohal/nucleo-753-can-eth/discussions)
- **hal753 Library**: [kodezine/hal753](https://github.com/sohal/hal753)

## See Also

- [hal753](https://github.com/sohal/hal753) - STM32H753 HAL library
- [QP/C Framework](https://www.state-machine.com/qpc/) - Real-time embedded framework
- [Mongoose](https://mongoose.ws/) - Embedded network library
