# Exam Instruments Firmware

Pre-built firmware for tester practical exams.

## Files

- **`wht_slave.hex`** - Intel HEX format (recommended for flashing)
  - Size: ~162 KB
  - Contains complete address information
  - Compatible with most flashing tools (ST-LINK, J-Link, OpenOCD, etc.)

- **`wht_slave.bin`** - Binary format
  - Size: ~58 KB
  - Raw binary data
  - May require manual address specification (0x08008000) when flashing

## Usage

### Flashing with ST-LINK (STM32CubeProgrammer)

```bash
# Using hex file (recommended)
STM32_Programmer_CLI -c port=SWD -w wht_slave.hex -v -rst

# Or using bin file
STM32_Programmer_CLI -c port=SWD -w wht_slave.bin 0x08008000 -v -rst
```

### Flashing with OpenOCD

```bash
# Using hex file
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program wht_slave.hex verify reset exit"

# Or using bin file
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program wht_slave.bin 0x08008000 verify reset exit"
```

### Flashing with J-Link

```bash
# Using hex file
JLinkExe -device STM32F429ZI -if SWD -speed 4000 -CommanderScript flash.jlink
# Where flash.jlink contains:
#   loadfile wht_slave.hex
#   r
#   g
#   q
```

## Features

- **TTL UART** (UART4, PA0/PA1): 115200 8N1, 3.3V logic
- **RS-232** (USART1, PA9/PA10): 115200 8N1, ±12V levels
- **Square Wave**: 1 kHz 50% on LED1 (PG9)
- **Boot Message**: Includes version and configuration
- **PING/PONG**: Simple connectivity test on both UARTs

## Documentation

See [exam_instruments_firmware.md](../exam_instruments_firmware.md) for complete usage instructions.

## Build Information

- **Build Date**: September 17, 2026
- **Preset**: Debug-EXAM-INSTRUMENTS
- **Compiler**: arm-none-eabi-gcc 13.2.1
- **WHT_APP_RUN_MODE**: 6
- **Git Branch**: cursor/exam-instruments-firmware-21a8

## Direct Download

You can download these files directly from the GitHub PR branch:

```bash
# Download hex file
wget https://raw.githubusercontent.com/ylongw/wht_slave/cursor/exam-instruments-firmware-21a8/Docs/exam/firmware/wht_slave.hex

# Download bin file
wget https://raw.githubusercontent.com/ylongw/wht_slave/cursor/exam-instruments-firmware-21a8/Docs/exam/firmware/wht_slave.bin
```

Or browse them at: https://github.com/ylongw/wht_slave/tree/cursor/exam-instruments-firmware-21a8/Docs/exam/firmware
