# Flashing Firmware

This guide explains how to program the firmware to your STM32H753 Nucleo board.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Using ST-Link](#using-st-link)
- [Using OpenOCD](#using-openocd)
- [Using STM32CubeProgrammer](#using-stm32cubeprogrammer)
- [Verification](#verification)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Hardware

- **STM32H753ZI Nucleo-144 board**
- **USB cable** (Type-A to Mini-B)
- **ST-Link V2/V3** (onboard or external)

### Software

Choose one of:
- **OpenOCD** (open-source, recommended)
- **STM32CubeProgrammer** (STMicroelectronics official tool)
- **st-flash** (from stlink package)

## Using ST-Link

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install stlink-tools

# macOS
brew install stlink

# Arch Linux
sudo pacman -S stlink
```

### Flash Firmware

```bash
# Flash .bin file
st-flash write build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.bin 0x08000000

# Verify
st-flash read verify.bin 0x08000000 0x20000
cmp build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.bin verify.bin
```

### Reset Board

```bash
st-flash reset
```

## Using OpenOCD

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install openocd

# macOS
brew install openocd

# Arch Linux
sudo pacman -S openocd
```

### Configuration File

Create `nucleo-h753.cfg`:

```tcl
# ST-Link V2/V3
source [find interface/stlink.cfg]

# STM32H753
source [find target/stm32h7x.cfg]

# Flash settings
adapter speed 4000
reset_config srst_only
```

### Flash Commands

#### Flash .elf File

```bash
openocd -f nucleo-h753.cfg \
  -c "program build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.elf verify reset exit"
```

#### Flash .hex File

```bash
openocd -f nucleo-h753.cfg \
  -c "program build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.hex verify reset exit"
```

#### Flash .bin File

```bash
openocd -f nucleo-h753.cfg \
  -c "program build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.bin 0x08000000 verify reset exit"
```

### Interactive Session

```bash
# Start OpenOCD server
openocd -f nucleo-h753.cfg

# In another terminal, connect via telnet
telnet localhost 4444

# OpenOCD commands:
> halt
> flash write_image erase build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.elf
> verify_image build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.elf
> reset run
> exit
```

## Using STM32CubeProgrammer

### Download

Download from: https://www.st.com/en/development-tools/stm32cubeprog.html

### GUI Method

1. **Launch STM32CubeProgrammer**
2. **Connect to Board**:
   - Select "ST-LINK" from connection method dropdown
   - Click "Connect" button
   - Green LED indicates successful connection

3. **Erase Flash**:
   - Go to "Erase & programming" tab
   - Select "Full chip erase"
   - Click "Start"

4. **Program Firmware**:
   - Click "Open file" button
   - Select `nucleo-753-can-eth.hex` or `.bin` file
   - Verify "Start address": `0x08000000` (for .bin)
   - Click "Start Programming"
   - Wait for "File download complete"

5. **Verify**:
   - Click "Verify" button
   - Should show "Download verified successfully"

6. **Reset**:
   - Click "Disconnect"
   - Power cycle board or press reset button

### Command Line Method

```bash
# Set up environment
export PATH=/opt/STM32CubeProgrammer/bin:$PATH

# Flash .hex file
STM32_Programmer_CLI -c port=SWD -w build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.hex -v -rst

# Flash .bin file at address 0x08000000
STM32_Programmer_CLI -c port=SWD -w build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.bin 0x08000000 -v -rst

# Options explained:
# -c port=SWD       : Connect via SWD (ST-Link)
# -w <file> [addr]  : Write file to flash
# -v                : Verify after programming
# -rst              : Reset MCU after programming
```

### Batch Script Example

```bash
#!/bin/bash
# flash.sh - Flash firmware to Nucleo board

FIRMWARE="build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.hex"

if [ ! -f "$FIRMWARE" ]; then
    echo "Error: Firmware not found: $FIRMWARE"
    exit 1
fi

echo "Flashing $FIRMWARE..."
STM32_Programmer_CLI -c port=SWD -w "$FIRMWARE" -v -rst

if [ $? -eq 0 ]; then
    echo "✓ Flashing successful!"
else
    echo "✗ Flashing failed!"
    exit 1
fi
```

## Verification

### Visual Verification

After flashing, you should observe:

1. **Green LED (LD1)** - Blinks at 1Hz (Blinky AO)
2. **Red LED (LD3)** - User LED
3. **Serial console** - Debug messages (if UART enabled)

### UART Verification

Connect to USART3 (ST-Link Virtual COM Port):

```bash
# Linux
sudo screen /dev/ttyACM0 115200

# macOS  
screen /dev/tty.usbmodem* 115200

# Windows (use PuTTY or TeraTerm)
# COM port: Check Device Manager
# Baud: 115200
```

Expected output:
```
QP/C 8.1.1
Application initialized
Blinky started (Priority 1)
Mongoose started (Priority 2)
Stack Monitor started (Priority 3)
System ready.
```

### Network Verification

If Ethernet is connected:

```bash
# Ping the board (check your network configuration)
ping 192.168.1.100

# Access web interface (if Mongoose server is configured)
curl http://192.168.1.100
```

## Troubleshooting

### ST-Link Not Detected

**Symptoms**: "Error: No ST-Link device found"

**Solutions**:

1. **Check USB connection**:
   ```bash
   lsusb | grep -i stlink
   # Should show: STMicroelectronics ST-LINK/V2
   ```

2. **Install udev rules** (Linux):
   ```bash
   sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```

3. **Update ST-Link firmware**:
   - Use STM32CubeProgrammer → "Firmware upgrade" button
   - Or download ST-LINK Upgrade tool from ST website

4. **Check jumpers**:
   - JP1: Should be connected (enables ST-Link)
   - CN4: Should be on pins 1-2 (VDD from ST-Link)

### Flash Operation Failed

**Symptoms**: "Error writing to flash at address 0x08000000"

**Solutions**:

1. **Read protection enabled**:
   ```bash
   # Check protection status
   STM32_Programmer_CLI -c port=SWD -ob displ
   
   # Disable read protection (ERASES FLASH!)
   STM32_Programmer_CLI -c port=SWD -ob RDP=0xAA
   ```

2. **Write protection enabled**:
   ```bash
   # Disable write protection
   STM32_Programmer_CLI -c port=SWD -ob WRP1A=0xFF WRP1B=0xFF
   ```

3. **Mass erase first**:
   ```bash
   st-flash erase
   # Then try flashing again
   ```

### Verification Failed

**Symptoms**: "Verification failed at address 0x08001000"

**Solutions**:

1. **Check firmware size**:
   ```bash
   size -B build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.elf
   # text should be < 2048000 (2MB flash)
   ```

2. **Corrupted firmware file**:
   ```bash
   # Rebuild firmware
   cmake --build --preset nucleo-753-can-eth-gnuarm14.3 --clean-first
   ```

3. **Flash at lower speed**:
   ```tcl
   # In nucleo-h753.cfg
   adapter speed 1000  # Reduce from 4000 to 1000 kHz
   ```

### Board Doesn't Boot

**Symptoms**: No LED activity, board seems dead

**Solutions**:

1. **Check BOOT pins**:
   - BOOT0 jumper (JP1): Should be removed (BOOT0 = 0)
   - This ensures boot from main flash (0x08000000)

2. **Check power**:
   ```bash
   # Verify 3.3V on board
   # Use multimeter on VDD pins
   ```

3. **Reset to factory**:
   - Flash ST's default Blinky example
   - Verify hardware is functional
   - Then flash your firmware again

4. **Check linker script**:
   - Verify `linker/STM32H753XX_FLASH.ld` is correct
   - Entry point should be at 0x08000000

### Permission Denied (Linux)

**Symptoms**: "Error: libusb_open() failed with LIBUSB_ERROR_ACCESS"

**Solutions**:

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Add ST-Link udev rules
sudo tee /etc/udev/rules.d/49-stlinkv2.rules <<EOF
# ST-Link V2
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"
# ST-Link V2-1
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666"
# ST-Link V3
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374d", MODE="0666"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger

# Re-login or reboot
```

## Automated Flashing

### Make Target (Optional)

Add to project Makefile:

```makefile
flash: build
	@echo "Flashing firmware..."
	openocd -f nucleo-h753.cfg \
	  -c "program build/nucleo-753-can-eth-gnuarm14.3/nucleo-753-can-eth.elf verify reset exit"

.PHONY: flash
```

Usage:
```bash
make flash
```

### CMake Custom Target

Add to `CMakeLists.txt`:

```cmake
# Add flash target
add_custom_target(flash
    COMMAND openocd -f ${CMAKE_SOURCE_DIR}/nucleo-h753.cfg
        -c "program ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing firmware to board..."
)
```

Usage:
```bash
cmake --build --preset nucleo-753-can-eth-gnuarm14.3 --target flash
```

## Next Steps

- [Debug with GDB](DEBUGGING.md)
- [Monitor system with UART](DEBUGGING.md#uart-debugging)
- [Extend Active Objects](EXTENDING_ACTIVE_OBJECTS.md)
