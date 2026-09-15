# 仪器考试固件使用说明 / Exam Instruments Firmware Usage

[English version below / 英文版本见下方]

## 中文版

### 概述

仪器考试固件是为实践考试场景设计的专用固件。它提供简单可靠的功能用于验证测试设备和连接：
- 启动时通过UART输出启动消息
- 响应PING命令返回PONG
- 在LED1引脚(PG9)上持续输出1 kHz方波用于示波器测量

### 预设名称

**Debug-EXAM-INSTRUMENTS**

### 编译和烧录

#### 编译

```bash
# 配置项目
cmake --preset Debug-EXAM-INSTRUMENTS

# 编译
cmake --build --preset Debug-EXAM-INSTRUMENTS
```

#### 烧录文件

编译完成后，烧录文件位于：

```
build/Debug-EXAM-INSTRUMENTS/wht_slave.elf
build/Debug-EXAM-INSTRUMENTS/wht_slave.bin
build/Debug-EXAM-INSTRUMENTS/wht_slave.hex
```

推荐使用 `.hex` 或 `.bin` 文件进行烧录。

如果需要生成hex文件，使用cmake install目标：

```bash
cmake --build build/Debug-EXAM-INSTRUMENTS --target install
```

### UART参数

考试固件使用项目主调试UART (RS485):

- **端口**: UART7 (RS485)
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无
- **接口**: RS-485

**重要**: 本项目使用RS-485接口，需要RS-485转USB转换器连接到PC。

### 启动消息

设备上电后，会通过UART输出以下启动消息：

```
=== BOOT OK ===
Version: EXAM-1.0.0
Build: Jan 15 2025 10:23:45
FW Ver: 1.2.3
Square Wave: 1 kHz on LED1 (PG9)
Ready for commands (PING)
```

消息包含：
- 启动确认
- 考试固件版本
- 编译日期和时间
- 固件版本号(来自git标签)
- 方波输出引脚说明
- 就绪提示

### PING/PONG命令

固件实现简单的行导向命令协议：

**发送**: `PING\r\n` 或 `ping\r\n`

**接收**: `PONG\r\n`

命令大小写不敏感，支持CR、LF或CRLF作为行结束符。

#### 测试示例 (Linux/macOS)

```bash
# 使用screen
screen /dev/ttyUSB0 115200

# 输入 PING 并按回车
PING
# 应该看到 PONG 响应

# 或使用echo和cat
echo "PING" > /dev/ttyUSB0
cat /dev/ttyUSB0
```

#### 测试示例 (Windows)

使用串口终端软件如PuTTY、TeraTerm或Realterm：
1. 配置串口: COM端口, 115200, 8N1
2. 输入 `PING` 并按回车
3. 应该收到 `PONG` 响应

### 方波输出

固件在 **LED1** 引脚 (PG9) 上持续输出1 kHz 50%占空比方波。

#### 引脚信息

- **引脚**: PG9 (GPIO Port G, Pin 9)
- **电路板标识**: LED1
- **频率**: 1000 Hz (1 kHz)
- **占空比**: 50%
- **电压**: 0V (低) / 3.3V (高)

#### 示波器测量

使用示波器连接LED1引脚(PG9)，应该观察到：
- **频率**: 1 kHz (周期 = 1 ms)
- **占空比**: 50% (高电平500 μs, 低电平500 μs)
- **电压**: 在0V和3.3V之间切换

推荐示波器设置：
- 时基: 500 μs/div 或 200 μs/div
- 电压: 1V/div
- 触发: 上升沿, 1.5V阈值

### 上电注意事项

1. **正常启动**: 设备上电后会自动进入考试模式(当使用Debug-EXAM-INSTRUMENTS固件时)
2. **UART连接**: 确保RS-485转换器正确连接并供电
3. **LED指示**: RUN LED (PC13) 会以1 Hz频率闪烁，表示系统运行正常
4. **自动输出**: 方波会自动开始输出，无需额外命令

### 故障排除

#### 看不到启动消息
- 检查RS-485转换器连接
- 确认波特率设置为115200
- 确认使用正确的COM端口/ttyUSB设备
- 检查RS-485收发器方向(应该设置为接收模式)

#### PING无响应
- 确认固件已正确烧录
- 检查UART连接
- 尝试发送带换行符的PING: `PING\r\n`
- 检查串口终端是否添加回车换行

#### 方波不正确
- 使用示波器确认探头连接到PG9/LED1
- 检查探头接地
- 确认示波器触发设置正确
- 检查时基设置(应该能看到多个波形周期)

---

## English Version

### Overview

The exam instruments firmware is a specialized build designed for practical examination scenarios. It provides simple, reliable functionality for verifying test equipment and connections:
- Boot message output over UART on startup
- PING/PONG command response
- Continuous 1 kHz square wave output on LED1 pin (PG9) for oscilloscope measurement

### Preset Name

**Debug-EXAM-INSTRUMENTS**

### Build and Flash

#### Prerequisites

Building this firmware requires:
- CMake 3.22 or later
- Ninja build system
- ARM GNU Toolchain (arm-none-eabi-gcc)

On the team's Windows environment (C:\byd\WHT\code\wht_slave), these tools should already be configured.

#### Build

```bash
# Configure the project
cmake --preset Debug-EXAM-INSTRUMENTS

# Build
cmake --build --preset Debug-EXAM-INSTRUMENTS
```

#### Flash Files

After building, the flash files are located at:

```
build/Debug-EXAM-INSTRUMENTS/wht_slave.elf
build/Debug-EXAM-INSTRUMENTS/wht_slave.bin
build/Debug-EXAM-INSTRUMENTS/wht_slave.hex
```

Use `.hex` or `.bin` file for flashing.

If hex file generation is needed, use the cmake install target:

```bash
cmake --build build/Debug-EXAM-INSTRUMENTS --target install
```

### UART Parameters

The exam firmware uses the project's primary debug UART (RS485):

- **Port**: UART7 (RS485)
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Interface**: RS-485

**Important**: This project uses an RS-485 interface, so you'll need an RS-485 to USB converter to connect to a PC.

### Boot Message

On power-up, the device outputs the following boot message over UART:

```
=== BOOT OK ===
Version: EXAM-1.0.0
Build: Jan 15 2025 10:23:45
FW Ver: 1.2.3
Square Wave: 1 kHz on LED1 (PG9)
Ready for commands (PING)
```

The message includes:
- Boot confirmation
- Exam firmware version
- Build date and time
- Firmware version number (from git tags)
- Square wave output pin information
- Ready prompt

### PING/PONG Command

The firmware implements a simple line-oriented command protocol:

**Send**: `PING\r\n` or `ping\r\n`

**Receive**: `PONG\r\n`

Commands are case-insensitive and support CR, LF, or CRLF line endings.

#### Test Example (Linux/macOS)

```bash
# Using screen
screen /dev/ttyUSB0 115200

# Type PING and press Enter
PING
# Should see PONG response

# Or using echo and cat
echo "PING" > /dev/ttyUSB0
cat /dev/ttyUSB0
```

#### Test Example (Windows)

Use a serial terminal like PuTTY, TeraTerm, or Realterm:
1. Configure: COM port, 115200, 8N1
2. Type `PING` and press Enter
3. Should receive `PONG` response

### Square Wave Output

The firmware continuously outputs a 1 kHz, 50% duty cycle square wave on the **LED1** pin (PG9).

#### Pin Information

- **Pin**: PG9 (GPIO Port G, Pin 9)
- **Board Label**: LED1
- **Frequency**: 1000 Hz (1 kHz)
- **Duty Cycle**: 50%
- **Voltage Levels**: 0V (Low) / 3.3V (High)

#### Oscilloscope Measurement

Connect an oscilloscope probe to the LED1 pin (PG9). You should observe:
- **Frequency**: 1 kHz (Period = 1 ms)
- **Duty Cycle**: 50% (High for 500 μs, Low for 500 μs)
- **Voltage**: Switching between 0V and 3.3V

Recommended oscilloscope settings:
- Time base: 500 μs/div or 200 μs/div
- Voltage: 1V/div
- Trigger: Rising edge, 1.5V threshold

### Power-On Notes

1. **Normal Boot**: The device automatically enters exam mode on power-up (when using the Debug-EXAM-INSTRUMENTS firmware)
2. **UART Connection**: Ensure RS-485 converter is properly connected and powered
3. **LED Indicator**: The RUN LED (PC13) blinks at 1 Hz to show the system is running
4. **Automatic Output**: The square wave starts automatically, no additional commands needed

### Troubleshooting

#### No Boot Message
- Check RS-485 converter connection
- Verify baud rate is set to 115200
- Confirm correct COM port/ttyUSB device
- Check RS-485 transceiver direction (should be in receive mode)

#### No Response to PING
- Confirm firmware is correctly flashed
- Check UART connection
- Try sending PING with explicit newline: `PING\r\n`
- Check if serial terminal adds CR/LF

#### Incorrect Square Wave
- Use oscilloscope to confirm probe is on PG9/LED1
- Check probe ground connection
- Verify oscilloscope trigger settings
- Check time base setting (should see multiple cycles)

---

## 技术细节 / Technical Details

### Run Mode

此固件使用 `WHT_APP_RUN_MODE=6` 编译标志。主应用启动逻辑会检查此标志，如果设置为6，会跳过工厂测试检测，直接进入考试仪器模式。

This firmware uses the `WHT_APP_RUN_MODE=6` compile flag. The main application startup logic checks this flag and, if set to 6, skips factory test detection and directly enters exam instruments mode.

### GPIO Configuration

方波使用TIM2通道2(TIM2_CH2)生成，该通道复用映射到PG9引脚。定时器配置为：
- 预分频器: 89 (90 MHz / 90 = 1 MHz定时器时钟)
- 自动重载: 999 (1 MHz / 1000 = 1 kHz PWM)
- 脉冲值: 500 (50%占空比)

The square wave is generated using TIM2 Channel 2 (TIM2_CH2), which is mapped to pin PG9 via alternate function. Timer configuration:
- Prescaler: 89 (90 MHz / 90 = 1 MHz timer clock)
- Auto-reload: 999 (1 MHz / 1000 = 1 kHz PWM)
- Pulse value: 500 (50% duty cycle)

### Board Compatibility

此固件为STM32F429/GD32F470 MCU设计(本仓库使用STM32 HAL)。虽然文档提到GD32F470ZI，但代码使用现有的STM32F429 HAL/硬件配置。

This firmware is designed for STM32F429/GD32F470 MCU (this repo uses STM32 HAL). While documentation mentions GD32F470ZI, the code uses the existing STM32F429 HAL/hardware configuration.
