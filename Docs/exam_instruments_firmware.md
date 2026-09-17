# 仪器考试固件使用说明 / Exam Instruments Firmware Usage

[English version below / 英文版本见下方]

## 中文版

### 概述

仪器考试固件是为实践考试场景设计的专用固件。它提供简单可靠的功能用于验证测试设备和连接：
- 启动时通过**两个UART接口**同时输出启动消息
- 在**TTL UART**和**RS-232**接口上响应PING命令返回PONG
- 在LED1引脚(PG9)上持续输出1 kHz方波用于示波器测量

### 预设名称

**Debug-EXAM-INSTRUMENTS**

### 编译和烧录

#### 前置要求

编译此固件需要:
- CMake 3.22 或更新版本
- Ninja 构建系统
- ARM GNU 工具链 (arm-none-eabi-gcc)

在团队的Windows环境 (C:\byd\WHT\code\wht_slave) 中，这些工具应该已经配置好。

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
build/Debug-EXAM-INSTRUMENTS/wht_slave.hex  ← 推荐用于烧录
```

**推荐使用 `.hex` 文件进行烧录**，它包含完整的地址信息并自动生成。

如果需要手动触发hex文件生成，可以使用cmake install目标：

```bash
cmake --build build/Debug-EXAM-INSTRUMENTS --target install
```

但通常POST_BUILD步骤已自动生成hex文件。

### UART接口

考试固件使用**两个UART接口**，相同的协议和行为：

#### 1. TTL UART (3.3V逻辑电平)

- **外设**: UART4
- **引脚**: PA0 (TX) / PA1 (RX)
- **板上标识**: DEBUG_TX / DEBUG_RX
- **电平**: 3.3V TTL逻辑
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无

**连接方式**: 使用TTL-USB转换器或USB转串口模块（注意3.3V电平，不要使用5V）

#### 2. RS-232

- **外设**: USART1
- **引脚**: PA9 (TX) / PA10 (RX)
- **接口**: RS-232标准电平（±12V）
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无

**连接方式**: 通过板载RS-232收发器连接到RS-232接口

### 启动消息

设备上电后，会**同时**在两个UART接口输出以下启动消息：

```
=== BOOT OK ===
Version: EXAM-1.0.0
Build: Jan 15 2025 10:23:45
FW Ver: 1.2.3
Square Wave: 1 kHz on LED1 (PG9)
TTL UART: UART4 (PA0/PA1) 115200 8N1
RS-232: USART1 (PA9/PA10) 115200 8N1
Ready for commands (PING)
```

消息包含：
- 启动确认（BOOT OK）
- 考试固件版本
- 编译日期和时间
- 固件版本号（来自git标签）
- 方波输出引脚说明
- 两个UART接口的配置信息
- 就绪提示

### PING/PONG命令

固件在**两个UART接口**上都实现相同的简单行导向命令协议：

**发送**: `PING\r\n` 或 `ping\r\n`

**接收**: `PONG\r\n`

命令大小写不敏感，支持CR、LF或CRLF作为行结束符。

#### TTL UART测试示例 (Linux/macOS)

```bash
# 使用screen连接TTL UART
screen /dev/ttyUSB0 115200

# 输入 PING 并按回车
PING
# 应该看到 PONG 响应

# 或使用echo和cat
echo "PING" > /dev/ttyUSB0
cat /dev/ttyUSB0
```

#### RS-232测试示例 (Windows)

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

1. **正常启动**: 设备上电后会自动进入考试模式（当使用Debug-EXAM-INSTRUMENTS固件时）
2. **双UART同时工作**: TTL UART和RS-232同时输出启动消息并接受命令
3. **LED指示**: RUN LED (PC13) 会以1 Hz频率闪烁，表示系统运行正常
4. **自动输出**: 方波会自动开始输出，无需额外命令

### 故障排除

#### TTL UART看不到启动消息
- 检查TTL-USB转换器连接
- 确认转换器支持3.3V电平（不要使用5V）
- 确认波特率设置为115200
- 确认使用正确的COM端口/ttyUSB设备
- 检查TX/RX连接是否交叉（设备TX接转换器RX）

#### RS-232看不到启动消息
- 检查RS-232电缆连接
- 确认波特率设置为115200
- 确认使用正确的COM端口
- 检查板载RS-232收发器供电

#### PING无响应
- 确认固件已正确烧录
- 检查UART连接
- 尝试发送带换行符的PING: `PING\r\n`
- 检查串口终端是否添加回车换行
- 尝试另一个UART接口

#### 方波不正确
- 使用示波器确认探头连接到PG9/LED1
- 检查探头接地
- 确认示波器触发设置正确
- 检查时基设置（应该能看到多个波形周期）

---

## English Version

### Overview

The exam instruments firmware is a specialized build designed for practical examination scenarios. It provides simple, reliable functionality for verifying test equipment and connections:
- Boot message output simultaneously on **two UART interfaces** at startup
- PING/PONG command response on both **TTL UART** and **RS-232** interfaces
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
build/Debug-EXAM-INSTRUMENTS/wht_slave.hex  ← Recommended for flashing
```

**Use `.hex` file for flashing** - it contains complete address information and is automatically generated.

If hex file generation needs to be triggered manually, use the cmake install target:

```bash
cmake --build build/Debug-EXAM-INSTRUMENTS --target install
```

But typically the POST_BUILD step already generates the hex file automatically.

### UART Interfaces

The exam firmware uses **two UART interfaces** with identical protocol and behavior:

#### 1. TTL UART (3.3V Logic Levels)

- **Peripheral**: UART4
- **Pins**: PA0 (TX) / PA1 (RX)
- **Board Labels**: DEBUG_TX / DEBUG_RX
- **Voltage**: 3.3V TTL logic
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None

**Connection**: Use TTL-USB converter or USB-to-Serial module (note 3.3V level, do not use 5V)

#### 2. RS-232

- **Peripheral**: USART1
- **Pins**: PA9 (TX) / PA10 (RX)
- **Interface**: RS-232 standard levels (±12V)
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None

**Connection**: Connected via onboard RS-232 transceiver to RS-232 connector

### Boot Message

On power-up, the device outputs the following boot message **simultaneously on both UART interfaces**:

```
=== BOOT OK ===
Version: EXAM-1.0.0
Build: Jan 15 2025 10:23:45
FW Ver: 1.2.3
Square Wave: 1 kHz on LED1 (PG9)
TTL UART: UART4 (PA0/PA1) 115200 8N1
RS-232: USART1 (PA9/PA10) 115200 8N1
Ready for commands (PING)
```

The message includes:
- Boot confirmation (BOOT OK)
- Exam firmware version
- Build date and time
- Firmware version number (from git tags)
- Square wave output pin information
- Both UART interface configuration details
- Ready prompt

### PING/PONG Command

The firmware implements the same simple line-oriented command protocol on **both UART interfaces**:

**Send**: `PING\r\n` or `ping\r\n`

**Receive**: `PONG\r\n`

Commands are case-insensitive and support CR, LF, or CRLF line endings.

#### TTL UART Test Example (Linux/macOS)

```bash
# Using screen to connect to TTL UART
screen /dev/ttyUSB0 115200

# Type PING and press Enter
PING
# Should see PONG response

# Or using echo and cat
echo "PING" > /dev/ttyUSB0
cat /dev/ttyUSB0
```

#### RS-232 Test Example (Windows)

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
2. **Dual UART Operation**: TTL UART and RS-232 both output boot messages and accept commands simultaneously
3. **LED Indicator**: The RUN LED (PC13) blinks at 1 Hz to show the system is running
4. **Automatic Output**: The square wave starts automatically, no additional commands needed

### Troubleshooting

#### No Boot Message on TTL UART
- Check TTL-USB converter connection
- Verify converter supports 3.3V levels (do not use 5V)
- Confirm baud rate is set to 115200
- Confirm correct COM port/ttyUSB device
- Check TX/RX connection is crossed (device TX to converter RX)

#### No Boot Message on RS-232
- Check RS-232 cable connection
- Verify baud rate is set to 115200
- Confirm correct COM port
- Check onboard RS-232 transceiver power

#### No Response to PING
- Confirm firmware is correctly flashed
- Check UART connection
- Try sending PING with explicit newline: `PING\r\n`
- Check if serial terminal adds CR/LF
- Try the other UART interface

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

### UART Configuration

项目中有三个UART外设，考试固件使用其中两个：

The project has three UART peripherals, the exam firmware uses two of them:

1. **UART4** (PA0/PA1) = TTL UART, 3.3V logic levels
   - 板上标识为 DEBUG_TX / DEBUG_RX
   - Board labeled as DEBUG_TX / DEBUG_RX
   
2. **USART1** (PA9/PA10) = RS-232 interface with ±12V levels
   - 通过板载收发器连接
   - Connected via onboard transceiver
   
3. **UART7** (PF6/PF7) = RS-485 interface (不用于考试模式)
   - Not used in exam mode

### GPIO Configuration

方波使用TIM2通道2(TIM2_CH2)生成，该通道复用映射到PG9引脚。定时器配置为：
- 预分频器: 89 (90 MHz / 90 = 1 MHz定时器时钟)
- 自动重载: 999 (1 MHz / 1000 = 1 kHz PWM)
- 脉冲值: 500 (50%占空比)

The square wave is generated using TIM2 Channel 2 (TIM2_CH2), which is mapped to pin PG9 via alternate function. Timer configuration:
- Prescaler: 89 (90 MHz / 90 = 1 MHz timer clock)
- Auto-reload: 999 (1 MHz / 1000 = 1 kHz PWM)
- Pulse value: 500 (50% duty cycle)

### Build Artifacts

编译后的固件文件路径：

Build artifact paths:

```
build/Debug-EXAM-INSTRUMENTS/
├── wht_slave.elf       # ELF文件（包含调试信息）
├── wht_slave.bin       # 二进制文件
└── wht_slave.hex       # Intel HEX文件（推荐烧录格式）
```

hex文件由`cmake/create_bin.cmake`中的POST_BUILD命令自动生成。

The hex file is automatically generated by POST_BUILD command in `cmake/create_bin.cmake`.

### Board Compatibility

此固件为STM32F429/GD32F470 MCU设计（本仓库使用STM32 HAL）。虽然文档提到GD32F470ZI，但代码使用现有的STM32F429 HAL/硬件配置。

This firmware is designed for STM32F429/GD32F470 MCU (this repo uses STM32 HAL). While documentation mentions GD32F470ZI, the code uses the existing STM32F429 HAL/hardware configuration.

### Changes from Previous Version

**主要变更 / Major Changes:**

从之前仅支持RS-485的版本改为支持TTL UART和RS-232：

Changed from RS-485-only version to support TTL UART and RS-232:

1. ✅ 移除RS-485依赖（UART7, RS485_CTRL控制引脚）
   - Removed RS-485 dependency (UART7, RS485_CTRL control pin)
   
2. ✅ 添加TTL UART支持（UART4, PA0/PA1）
   - Added TTL UART support (UART4, PA0/PA1)
   
3. ✅ 添加RS-232支持（USART1, PA9/PA10）
   - Added RS-232 support (USART1, PA9/PA10)
   
4. ✅ 两个UART接口同时工作，相同协议
   - Both UART interfaces work simultaneously with identical protocol
   
5. ✅ 启动消息包含两个接口的配置信息
   - Boot message includes configuration info for both interfaces
   
6. ✅ hex文件自动生成，位于build/Debug-EXAM-INSTRUMENTS/
   - Hex file automatically generated in build/Debug-EXAM-INSTRUMENTS/
