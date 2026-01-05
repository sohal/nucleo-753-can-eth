# Memory Optimization

This guide explains how to analyze and optimize memory usage in the nucleo-753-can-eth firmware.

## Table of Contents

- [Memory Overview](#memory-overview)
- [Build Size Analysis](#build-size-analysis)
- [Map File Analysis](#map-file-analysis)
- [Optimization Techniques](#optimization-techniques)
- [Common Memory Issues](#common-memory-issues)
- [Stack and Heap Sizing](#stack-and-heap-sizing)

## Memory Overview

### STM32H753 Memory Architecture

```
┌─────────────────────────────────────────────────┐
│ Memory Region     │ Size    │ Start      │ Use │
├─────────────────────────────────────────────────┤
│ FLASH (ROM)       │ 2048 KB │ 0x08000000 │ R/X │
│ DTCMRAM           │  128 KB │ 0x20000000 │ R/W │
│ AXI SRAM (D1)     │  512 KB │ 0x24000000 │ R/W │
│ SRAM1/2/3 (D2)    │  288 KB │ 0x30000000 │ R/W │
│ SRAM4 (D3)        │   64 KB │ 0x38000000 │ R/W │
│ ITCMRAM           │   64 KB │ 0x00000000 │ R/X │
└─────────────────────────────────────────────────┘

Total FLASH: 2 MB
Total RAM: 1056 KB (128+512+288+64+64)
```

### Typical Firmware Memory Usage

```
Section         Size (bytes)  Percentage  Location
───────────────────────────────────────────────────
.text (code)    ~110,000      ~5% FLASH   Program code
.rodata (const) ~10,000       ~0.5% FLASH Strings, tables
.data (init)    ~2,000        ~0.1% FLASH→RAM  Initialized variables
.bss (zero)     ~8,000        ~0.8% RAM   Uninitialized variables
.heap           16,384        ~1.6% RAM   Dynamic allocation
.stack          65,536        ~6.2% RAM   Main stack
Event pools     ~3,000        ~0.3% RAM   QF event pools
AO queues       ~1,000        ~0.1% RAM   Active Object queues
Mongoose        ~40,000       ~3.8% RAM   Network buffers
───────────────────────────────────────────────────
Total FLASH:    ~122 KB / 2048 KB  (6% used)
Total RAM:      ~136 KB / 1056 KB  (13% used)
```

## Build Size Analysis

### Berkeley Format Size Report

After build, CMake displays:

```
   text    data     bss     dec     hex filename
 112384    2048   67656  182088   2c778 nucleo-753-can-eth.elf
```

**Interpretation:**
- **text**: Code + read-only data in FLASH (112,384 bytes = ~110 KB)
- **data**: Initialized data in FLASH, copied to RAM (2,048 bytes)
- **bss**: Uninitialized data in RAM, zero-filled (67,656 bytes = ~66 KB)
- **dec**: Total (text + data + bss) = 182,088 bytes
- **hex**: Hexadecimal total = 0x2c778

**RAM usage** = data + bss = 2,048 + 67,656 = **69,704 bytes (~68 KB)**

**FLASH usage** = text + data = 112,384 + 2,048 = **114,432 bytes (~112 KB)**

### Detailed Size Analysis

```bash
# Berkeley format (text/data/bss)
arm-none-eabi-size -B nucleo-753-can-eth.elf

# SysV format (section-by-section)
arm-none-eabi-size -A nucleo-753-can-eth.elf

# Output:
section              size      addr
.isr_vector          1160   0x8000000
.text             109524   0x8000298
.rodata             1548   0x801b42c
.ARM                   8   0x801ba38
.init_array           16   0x801ba40
.fini_array            8   0x801ba50
.data                512   0x20000000
.bss               67144   0x20000200
```

### Tracking Size Over Time

```bash
# Save size report
arm-none-eabi-size -B nucleo-753-can-eth.elf > size_report.txt

# Compare with previous build
diff size_report_old.txt size_report.txt
```

## Map File Analysis

The linker generates `nucleo-753-can-eth.map` with detailed memory allocation.

### Map File Structure

```
Memory Configuration
Archive member included
Allocating common symbols
Discarded input sections
Memory map
Cross Reference Table
```

### Key Sections to Analyze

#### 1. Largest Functions

```bash
# Extract and sort functions by size
grep -A 1 "\.text\." nucleo-753-can-eth.map | \
    grep -E "0x[0-9a-f]+ +0x[0-9a-f]+" | \
    awk '{print $2, $0}' | sort -rn | head -20

# Example output:
0x1234 .text.mongoose.c    0x08001000   0x1234  mongoose.o
0x0abc .text.HAL_ETH_Init  0x08002234   0x0abc  stm32h7xx_hal_eth.o
0x0456 .text.mg_http_parse 0x08003000   0x0456  mongoose.o
```

**Largest contributors:**
1. **Mongoose network stack** (~30-40 KB)
2. **STM32 HAL drivers** (~20-30 KB)
3. **QP/C framework** (~10-15 KB)
4. **Application AOs** (~5-10 KB)

#### 2. Largest Data Sections

```bash
# Find large .data and .bss symbols
grep -E "\.bss\.|\.data\." nucleo-753-can-eth.map | \
    grep -E "0x[0-9a-f]+ +0x[0-9a-f]+" | \
    awk '{print $2, $0}' | sort -rn | head -20

# Example:
0x10000 .bss.ucHeap        0x20000000   0x10000  heap.o       (64 KB)
0x4000  .bss.mg_ctx        0x20010000   0x4000   mongoose.o   (16 KB)
0x2000  .bss.eth_rxbuf     0x20014000   0x2000   eth.o        (8 KB)
```

#### 3. Library Contributions

```bash
# Group by library
awk '/^\.text/ {lib=$NF; size=$2; gsub(/0x/,"",size); \
     total[lib]+=strtonum("0x"size)} \
     END {for (l in total) printf "%10d %s\n", total[l], l}' \
     nucleo-753-can-eth.map | sort -rn

# Output:
   45678 mongoose.o
   32456 libnucleo-h753.a
   12345 libqpc.a
    5678 blinky.o
```

### Analyzing Stack Usage

```bash
# Find stack allocation in map file
grep -A 5 "\.stack" nucleo-753-can-eth.map

# Output:
.stack          0x20000000    0x10000
                0x20000000                . = ALIGN (0x8)
                0x20000000                __stack_start__ = .
                0x20010000                . = (. + _Min_Stack_Size)
                0x20010000                . = ALIGN (0x8)
                0x20010000                __stack_end__ = .
```

**Stack size**: 0x10000 = 65,536 bytes = **64 KB**

## Optimization Techniques

### 1. Compiler Optimization Levels

```cmake
# CMakeLists.txt
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")      # Maximum speed
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")   # Minimum size
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")             # No optimization, full debug
```

Rebuild with size optimization:
```bash
cmake --preset nucleo-753-can-eth-gnuarm14.3 -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build --preset nucleo-753-can-eth-gnuarm14.3
```

**Expected savings**: 20-30% code size with `-Os`

### 2. Link-Time Optimization (LTO)

```cmake
# Enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(${PROJECT_NAME} PRIVATE -flto)
target_link_options(${PROJECT_NAME} PRIVATE -flto)
```

**Expected savings**: 10-15% additional size reduction

### 3. Unused Function Elimination

Already enabled in project:
```cmake
target_compile_options(${PROJECT_NAME} PRIVATE
    -ffunction-sections
    -fdata-sections
)
target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--gc-sections      # Garbage collect unused sections
)
```

Verify with:
```bash
# Check for discarded sections
grep "Discarded" nucleo-753-can-eth.map | wc -l
```

### 4. Disable Unused HAL Modules

Edit `stm32h7xx_hal_conf.h` (in hal753 library):

```c
// Disable unused peripherals
//#define HAL_CRYP_MODULE_ENABLED    // Crypto hardware (saves ~5 KB)
//#define HAL_HASH_MODULE_ENABLED    // Hash hardware (saves ~3 KB)
//#define HAL_DCMI_MODULE_ENABLED    // Camera interface (saves ~2 KB)

// Keep only what you need:
#define HAL_GPIO_MODULE_ENABLED
#define HAL_ETH_MODULE_ENABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
```

**Note**: Requires rebuilding hal753 library

### 5. Optimize Mongoose Configuration

Edit `frameworks/mongoose/mongoose_config.h`:

```c
// Disable unused protocols
#define MG_ENABLE_SSI 0          // Server-side includes
#define MG_ENABLE_MBEDTLS 0      // TLS (if not needed)
#define MG_ENABLE_MQTT 0         // MQTT protocol
#define MG_ENABLE_DNS 0          // DNS client

// Reduce buffer sizes
#define MG_IO_SIZE 512           // Default: 2048 (saves 1.5 KB per connection)
#define MG_MAX_HTTP_HEADERS 20   // Default: 40

// Disable file operations if not needed
#define MG_ENABLE_FILE 0
```

**Expected savings**: 10-20 KB depending on features disabled

### 6. Reduce Event Pool Sizes

Edit `frameworks/qpc/qpc-adapter.c`:

```c
void application_init(void) {
    // Reduce pool sizes if events are small and infrequent
    static QEvt const *smallPoolSto[10];      // Was: 20
    static uint8_t mediumPoolSto[10][64];     // Was: 20x128

    QF_poolInit(smallPoolSto, sizeof(smallPoolSto), sizeof(smallPoolSto[0]));
    QF_poolInit(mediumPoolSto, sizeof(mediumPoolSto), sizeof(mediumPoolSto[0]));
}
```

**Savings**: ~1.5 KB RAM per pool reduction

### 7. Optimize String Literals

```c
// Instead of:
printf("This is a very long debug string with lots of information\n");

// Use:
#ifdef DEBUG
    printf("Debug info\n");
#endif

// Or use shorter messages:
printf("DBG\n");
```

### 8. Use const for Read-Only Data

```c
// BAD: Wastes RAM
char lookup_table[] = {0, 1, 2, 3, ...};

// GOOD: Stored in FLASH
const char lookup_table[] = {0, 1, 2, 3, ...};
```

## Common Memory Issues

### 1. Stack Overflow

**Symptoms:**
- Hard fault
- Random crashes
- Stack watermark corrupted (0xDEADBEEF pattern missing)

**Detection:**
```gdb
# Check stack usage at runtime
(gdb) x/256x 0x20000000
# Count 0xDEADBEEF words remaining
```

**Solution:**
```c
// Increase stack in linker script
_Min_Stack_Size = 0x20000;  // 128 KB instead of 64 KB
```

### 2. Heap Exhaustion

**Symptoms:**
- malloc() returns NULL
- QF event allocation fails

**Detection:**
```c
// Monitor heap usage
extern uint8_t ucHeap[];
extern uint8_t *pxEnd;
size_t heap_free = (size_t)(&ucHeap[0] + configTOTAL_HEAP_SIZE - pxEnd);
printf("Heap free: %u bytes\n", heap_free);
```

**Solution:**
- Increase heap size in linker script
- Use static allocation for AOs and events
- Reduce Mongoose buffer sizes

### 3. Global Variable Overflow

**Symptoms:**
- Variables corrupted
- .bss section too large

**Detection:**
```bash
# Check .bss size
arm-none-eabi-size -A nucleo-753-can-eth.elf | grep bss
```

**Solution:**
- Move large buffers to FLASH (`const`)
- Use dynamic allocation
- Reduce array sizes

### 4. Code Size Overflow

**Symptoms:**
- Linker error: "section `.text' will not fit in region `FLASH'"

**Solution:**
- Enable optimization (`-Os` or `-O2`)
- Remove unused code
- Disable unused HAL modules
- Consider external FLASH for large assets

## Stack and Heap Sizing

### Stack Size Calculation

```
Stack = ISR stack + deepest call chain + local variables + margin

Recommended:
- Simple applications: 16-32 KB
- Network applications: 64-128 KB  ← Current setting
- Complex applications: 128-256 KB
```

### Measuring Stack Usage

Runtime measurement using watermark:

```c
// From stack_monitor_ao.c
uint32_t BSP_check_stack_watermark(void) {
    extern uint32_t __stack_start__;
    extern uint32_t __stack_end__;

    uint32_t *stack_ptr = &__stack_start__;
    uint32_t *stack_top = &__stack_end__;
    uint32_t unused_words = 0;

    // Count 0xDEADBEEF words
    while (*stack_ptr == 0xDEADBEEF && stack_ptr < stack_top) {
        unused_words++;
        stack_ptr++;
    }

    uint32_t stack_size = (uint32_t)stack_top - (uint32_t)&__stack_start__;
    uint32_t used_bytes = stack_size - (unused_words * 4);
    uint32_t percent_used = (used_bytes * 100) / stack_size;

    return percent_used;
}
```

### Heap Size Tuning

```c
// In linker script (STM32H753XX_FLASH.ld)
_Min_Heap_Size = 0x4000;  /* 16 KB */

// Adjust based on:
// - Number of dynamic allocations
// - Mongoose connection count
// - Event pool sizes
```

## Memory Layout Customization

### Using Different RAM Regions

STM32H753 has multiple RAM regions. Optimize by placing data strategically:

```c
// In linker script
.dtcm_data (NOLOAD) : {
    *(.dtcm)           // Fast access data (CPU tightly-coupled)
} > DTCMRAM

.eth_buffers (NOLOAD) : {
    *(.eth_rx_buf)     // Ethernet RX buffers
    *(.eth_tx_buf)     // Ethernet TX buffers
} > RAM_D2

.dma_buffers (NOLOAD) : {
    *(.dma_buffer)     // DMA buffers
} > RAM_D2
```

Usage in code:
```c
__attribute__((section(".dtcm"))) uint32_t fast_data[1024];
__attribute__((section(".eth_rx_buf"))) uint8_t eth_rx[2048];
```

### External Memory

For large data sets, use external SDRAM or QSPI FLASH:

```c
// Linker script
MEMORY {
    QSPI (rx)  : ORIGIN = 0x90000000, LENGTH = 8M
}

.qspi_data : {
    *(.qspi)
} > QSPI
```

## Next Steps

- [Build firmware](BUILDING.md)
- [Debug memory issues](DEBUGGING.md)
- [Understand architecture](ARCHITECTURE.md)
