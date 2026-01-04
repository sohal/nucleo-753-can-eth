# Debugging Guide

This guide explains how to debug the nucleo-753-can-eth firmware using GDB and OpenOCD.

## Table of Contents

- [Prerequisites](#prerequisites)
- [GDB with OpenOCD](#gdb-with-openocd)
- [UART Debugging](#uart-debugging)
- [Common Debug Scenarios](#common-debug-scenarios)
- [QP Spy Integration](#qp-spy-integration)
- [Tips and Tricks](#tips-and-tricks)

## Prerequisites

### Required Tools

- **OpenOCD** - Debug server
- **GDB** - GNU Debugger (ARM version)
- **Serial terminal** - screen, minicom, or PuTTY

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install openocd gdb-multiarch screen

# macOS
brew install openocd arm-none-eabi-gdb screen

# Verify installation
arm-none-eabi-gdb --version
openocd --version
```

## GDB with OpenOCD

### Basic Setup

1. **Create OpenOCD configuration** (`nucleo-h753.cfg`):

```tcl
source [find interface/stlink.cfg]
source [find target/stm32h7x.cfg]

adapter speed 4000
reset_config srst_only

# Enable semihosting (optional)
$_TARGETNAME configure -event gdb-attach {
    echo "Debugger connected"
    reset init
}
```

2. **Start OpenOCD server**:

```bash
openocd -f nucleo-h753.cfg
```

Output should show:
```
Info : Listening on port 3333 for gdb connections
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
```

3. **Launch GDB** (in another terminal):

```bash
arm-none-eabi-gdb build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.elf
```

4. **Connect to OpenOCD**:

```gdb
(gdb) target extended-remote localhost:3333
(gdb) monitor reset init
(gdb) load
(gdb) monitor reset halt
```

### GDB Session Example

```gdb
# Connect
(gdb) target extended-remote localhost:3333
Remote debugging using localhost:3333

# Load firmware
(gdb) load
Loading section .isr_vector, size 0x298 lma 0x8000000
Loading section .text, size 0x1b2a4 lma 0x8000298
...
Transfer rate: 21 KB/sec, 5632 bytes/write.

# Set breakpoint in application_init
(gdb) break application_init
Breakpoint 1 at 0x8012a34: file frameworks/qpc/qpc-adapter.c, line 45.

# Run to breakpoint
(gdb) continue
Continuing.

Breakpoint 1, application_init () at frameworks/qpc/qpc-adapter.c:45
45      QF_init();

# Step through code
(gdb) next
46      static QEvt const *smallPoolSto[20];

(gdb) step
(gdb) print l_blinky
$1 = {super = {super = {vptr = 0x8019f20 <QActive_vtable>}, ...}, timer = {...}}

# Examine memory
(gdb) x/16x 0x20000000
0x20000000:     0xdeadbeef      0xdeadbeef      0xdeadbeef      0xdeadbeef

# Backtrace
(gdb) bt
#0  application_init () at frameworks/qpc/qpc-adapter.c:45
#1  0x08001a2c in main () at nucleo-h753/Core/Src/main.c:123

# Continue execution
(gdb) continue
```

### Common GDB Commands

```gdb
# Breakpoints
break function_name                    # Set breakpoint at function
break file.c:123                       # Set breakpoint at line
info breakpoints                       # List all breakpoints
delete 1                               # Delete breakpoint #1
clear                                  # Clear all breakpoints

# Execution control
run                                    # Start program
continue (or c)                        # Continue execution
next (or n)                           # Step over
step (or s)                           # Step into
finish                                # Step out
until                                 # Run until line

# Inspection
print variable                        # Print variable value
print /x variable                     # Print in hex
print *pointer                        # Dereference pointer
display variable                      # Auto-display on each step
info locals                           # Show local variables
info args                             # Show function arguments

# Memory examination
x/16x 0x20000000                      # Examine 16 words in hex
x/s 0x08001000                        # Examine string
set {int}0x20000000 = 0x12345678      # Write to memory

# Registers
info registers                        # Show all registers
print $pc                             # Program counter
print $sp                             # Stack pointer
print $lr                             # Link register

# Watchpoints
watch variable                        # Break when variable changes
rwatch *0x20000100                    # Break on read
awatch *0x40020C00                    # Break on access (GPIO)

# Reset and reload
monitor reset halt                    # Reset and halt MCU
load                                  # Reload firmware
monitor reset run                     # Reset and run
```

### GDB Initialization File

Create `.gdbinit` in project root:

```gdb
# .gdbinit - Auto-load for nucleo-753-can-eth

target extended-remote localhost:3333

# Load symbols
file build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.elf

# Don't ask for confirmation
set confirm off

# Enable pretty-printing
set print pretty on
set print array on

# Reset and halt
monitor reset halt

# Load firmware
load

# Set breakpoint at main
break main

# Common breakpoints (commented out by default)
# break application_init
# break HardFault_Handler
# break assert_failed

# Helper functions
define reset_and_run
    monitor reset halt
    load
    monitor reset run
end

define reset_to_main
    monitor reset halt
    load
    tbreak main
    continue
end

echo \n--- GDB ready for debugging ---\n
echo Type 'reset_to_main' to reset and break at main()\n
echo Type 'reset_and_run' to reset and run freely\n\n
```

Usage:
```bash
arm-none-eabi-gdb
# GDB automatically connects and loads firmware
```

## UART Debugging

### Hardware Setup

STM32H753 Nucleo uses USART3 for Virtual COM Port:
- **TX**: PD8 (connected to ST-Link)
- **RX**: PD9 (connected to ST-Link)
- **Baud**: 115200 (default)

### Connect to Serial Console

```bash
# Linux
sudo screen /dev/ttyACM0 115200

# macOS
screen /dev/tty.usbmodem* 115200

# Exit screen: Ctrl+A, then K
```

### Adding Printf Debugging

Modify `frameworks/qpc/qpc-adapter.c`:

```c
#include <stdio.h>

// Redirect printf to UART (if syscalls.c implements _write)
void application_init(void)
{
    printf("\r\n=== Firmware Starting ===\r\n");
    printf("QP/C version: %s\r\n", QP_VERSION_STR);
    printf("hal753 library: v1.0.1\r\n");
    
    QF_init();
    printf("QF initialized\r\n");
    
    // ... initialize AOs ...
    printf("Blinky AO started (priority %d)\r\n", 1);
    printf("Mongoose AO started (priority %d)\r\n", 2);
    printf("Stack Monitor AO started (priority %d)\r\n", 3);
    
    printf("=== System Ready ===\r\n\r\n");
    
    QF_run();  // Never returns
}
```

### Semihosting (Alternative)

Semihosting routes printf to OpenOCD console:

1. **Enable in code**:
```c
extern void initialise_monitor_handles(void);

void application_init(void) {
    initialise_monitor_handles();  // Enable semihosting
    printf("Debug via semihosting\n");
    // ...
}
```

2. **Enable in GDB**:
```gdb
(gdb) monitor arm semihosting enable
semihosting is enabled
```

3. **View output in OpenOCD terminal**

**Note**: Semihosting is slow - only use for debug builds.

## Common Debug Scenarios

### Hard Fault Debugging

1. **Set breakpoint**:
```gdb
(gdb) break HardFault_Handler
```

2. **When hit, examine fault registers**:
```gdb
(gdb) print/x *(uint32_t*)0xE000ED28   # CFSR
(gdb) print/x *(uint32_t*)0xE000ED2C   # HFSR
(gdb) print/x *(uint32_t*)0xE000ED30   # DFSR
(gdb) print/x *(uint32_t*)0xE000ED34   # MMFAR
(gdb) print/x *(uint32_t*)0xE000ED38   # BFAR
```

3. **Backtrace to find fault location**:
```gdb
(gdb) bt
```

### Stack Overflow Detection

QP/C uses stack watermarking:

```gdb
# Check stack pattern
(gdb) x/256x $sp
# Look for 0xDEADBEEF pattern
# If pattern is corrupted, stack overflow occurred

# Set watchpoint on stack boundary
(gdb) watch *(uint32_t*)0x20001000
```

### Event Queue Overflow

```gdb
# Break in QF assert
(gdb) break Q_onAssert

# When hit:
(gdb) print file
(gdb) print line

# Check AO queue
(gdb) print AO_Blinky->eQueue
```

### Examining Active Objects

```gdb
# Print AO structure
(gdb) print l_blinky
$1 = {
  super = {
    super = {...},
    prio = 1 '\001',
    ...
  },
  timer = {...}
}

# Check AO state
(gdb) print l_blinky.super.state

# Check queue depth
(gdb) print l_blinky.super.eQueue.nUsed
```

### Network Debugging (Mongoose)

```gdb
# Break in Mongoose poll
(gdb) break mg_mgr_poll

# Examine Mongoose manager
(gdb) print mgr
(gdb) print mgr->conns

# Check for HTTP events
(gdb) break mg_http_serve_dir
```

## QP Spy Integration

QP Spy provides runtime tracing. To enable:

### 1. Enable QS in qp_config.h

```c
#define Q_SPY    1
```

### 2. Rebuild with QS

```bash
cmake --preset nucleo-753-can-eth-gnuarm14.3 -DQ_SPY=ON
cmake --build --preset nucleo-753-can-eth-gnuarm14.3
```

### 3. Connect QP Spy

```bash
# Forward QS output via UART or TCP
qspy -c COM3  # Windows
qspy -c /dev/ttyACM0  # Linux
```

### 4. View Trace

QP Spy shows:
- Active Object states and transitions
- Event posting and processing
- Timer arming/disarming
- Publish-subscribe activity
- Memory pool usage

## Tips and Tricks

### 1. Source-Level Debugging

Ensure debug symbols are generated:
```cmake
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
```

### 2. Non-Intrusive Debugging

Use SWO (Serial Wire Output) for printf without UART:
```c
// Initialize SWO in SystemClock_Config()
```

### 3. Watchdog Debugging

Disable IWDG during debug:
```gdb
(gdb) set {int}0x40003000 = 0x00000000  # Disable IWDG
```

### 4. Peripheral Register Access

```gdb
# GPIO GPIOA->ODR (LED control)
(gdb) print/x *(uint32_t*)0x40020014

# ETH registers
(gdb) x/16x 0x40028000
```

### 5. Flash Breakpoint Limit

Cortex-M7 has 8 hardware breakpoints. Use software breakpoints in RAM if needed.

### 6. Real-Time Debugging

Avoid long halts in time-critical code:
- Use conditional breakpoints sparingly
- Prefer watchpoints over single-stepping
- Disable interrupts cautiously

## Next Steps

- [Optimize memory usage](MEMORY_OPTIMIZATION.md)
- [Extend Active Objects](EXTENDING_ACTIVE_OBJECTS.md)
- [Flash firmware](FLASHING.md)
