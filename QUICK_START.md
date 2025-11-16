# 🚀 快速开始指南

## STM32 APRS Decoder - 立即开始使用

---

## 📋 前提条件

- ✅ **硬件**: STM32F401/F411/L412开发板 + SX1276/SX1278模块
- ✅ **软件**: Keil MDK-ARM 或 STM32CubeIDE
- ✅ **驱动**: ST-Link驱动
- ✅ **固件包**: STM32CubeF4 或 STM32CubeL4

---

## ⚡ 5分钟快速设置

### 方法1: 使用STM32CubeMX（推荐新手）

#### 步骤1: 打开CubeMX并创建项目
```
1. 启动 STM32CubeMX
2. File → New Project
3. 选择MCU: STM32F401RETx / STM32F411RETx / STM32L412KBUx
4. 点击 Start Project
```

#### 步骤2: 配置外设

**SPI1 (连接SX1276)**:
```
Mode: Full-Duplex Master
Data Size: 8 Bits
Clock: 5 MHz (Prescaler = 16)
```

**USART1 (APRS输出)**:
```
Mode: Asynchronous
Baud Rate: 9600
```

**USART2 (调试)**:
```
Mode: Asynchronous  
Baud Rate: 115200
```

**TIM2 (采样定时器)**:
```
Clock Source: Internal
Prescaler: 0
Counter Period: 计算值 = (时钟频率/26400) - 1
```

**GPIO**:
```
PA1: GPIO_Output (SX127X_RESET)
PA2: GPIO_Input  (SX127X_DIO0)
PA3: GPIO_Input  (SX127X_DIO2)  ← 关键！
PA4: GPIO_Output (SX127X_NSS)
```

#### 步骤3: 配置时钟树

**STM32F401**: 设置为84 MHz  
**STM32F411**: 设置为100 MHz  
**STM32L412**: 设置为80 MHz

#### 步骤4: 生成代码
```
Project → Settings:
  - Project Name: APRS_Decoder
  - Toolchain: MDK-ARM V5 (或 STM32CubeIDE)

Project → Generate Code
```

#### 步骤5: 复制我们的代码
```bash
# 将生成的项目记为 <generated>
# 将本项目记为 <this>

# 复制头文件
cp <this>/Core/Inc/*.h <generated>/Core/Inc/

# 复制源文件
cp <this>/Core/Src/*.c <generated>/Core/Src/

# 如果使用CMSIS-DSP
cp -r <this>/Middlewares/Third_Party/CMSIS-DSP <generated>/Middlewares/Third_Party/
```

#### 步骤6: 配置编译器

在Keil或CubeIDE中添加：

**预定义宏**:
```c
STM32F401xE (或对应MCU型号)
USE_HAL_DRIVER
ARM_MATH_CM4
__FPU_PRESENT=1U
USE_CMSIS_DSP=1
```

**Include路径**:
```
Core/Inc
Middlewares/Third_Party/CMSIS-DSP/Include
```

**优化级别**: -O3

#### 步骤7: 编译并下载
```
Build (F7) → Download (F8) → Run
```

---

### 方法2: 手动配置Keil项目（适合有经验的）

详细步骤请参考 `KEIL_PROJECT_SETUP.md`

**核心步骤**:
1. 创建新项目，选择MCU
2. 添加所有Core/Src/*.c文件
3. 添加HAL库文件
4. 配置Include路径和宏定义
5. 启用FPU
6. 编译

---

## 🔌 硬件连接

### SX1276/SX1278 → STM32

| SX1276引脚 | STM32引脚 | 功能 |
|-----------|----------|------|
| NSS | PA4 | SPI片选 |
| MOSI | PA7 | SPI数据 |
| MISO | PA6 | SPI数据 |
| SCK | PA5 | SPI时钟 |
| **DIO2** | **PA3** | **采样引脚（关键！）** |
| DIO0 | PA2 | 中断 |
| RESET | PA1 | 复位 |
| GND | GND | 地 |
| 3V3 | 3V3 | 电源 |

⚠️ **重要**: DIO2必须正确连接，这是采样信号源！

### UART连接

| 功能 | STM32引脚 | 波特率 |
|------|----------|--------|
| **调试输出** | PA2 (USART2 TX) | 115200 |
| **APRS输出** | PA9 (USART1 TX) | 9600 |

---

## 📱 测试验证

### 1. 串口调试输出

打开串口工具（115200,8N1）：

```
╔════════════════════════════════════════╗
║   SX1276/SX1278 APRS Decoder v2.0    ║
╚════════════════════════════════════════╝

MCU: STM32F401 @ 84 MHz
FPU: Enabled
DSP: CMSIS-DSP Enabled

Initializing SX127x...
SX127x detected (v0x12)
Frequency: 434.00 MHz
Bitrate: 26.4 kbps

Initializing AFSK demodulator...
Sample rate: 26400 Hz

========================================
System ready! Listening for APRS...
========================================
```

### 2. 检查SX1276通信

如果看到 `SX127x detected (v0x12)`，说明SPI通信正常。

### 3. 接收APRS信号

连接天线后，应该看到：

```
╔════════════════════════════════════════╗
║       APRS Frame Received!           ║
╚════════════════════════════════════════╝
From: N7LEM-5
To: APRS
Info: !3745.12N/12205.34W>Test APRS
Quality: 87%
----------------------------------------
```

同时UART1会输出：
```
N7LEM-5>APRS:!3745.12N/12205.34W>Test APRS
```

---

## 🐛 常见问题

### Q: 编译错误 "undefined reference to xxx"

**A**: 检查是否添加了所有HAL库文件：
```c
// 必需的HAL文件
stm32f4xx_hal.c
stm32f4xx_hal_gpio.c
stm32f4xx_hal_spi.c
stm32f4xx_hal_uart.c
stm32f4xx_hal_tim.c
stm32f4xx_hal_cortex.c
stm32f4xx_hal_rcc.c
```

### Q: printf无输出

**A**: 
1. 确保启用了MicroLIB（Keil）
2. 检查USART2是否正确初始化
3. 检查_write函数是否实现（已在main.c中）

### Q: SX127x not found

**A**:
1. 检查SPI连接
2. 检查NSS引脚配置
3. 测量SPI时钟信号
4. 确认SX1276供电正常

### Q: 无信号输出

**A**:
1. 检查DIO2引脚连接（最常见原因）
2. 检查天线连接
3. 检查频率设置
4. 确认有APRS信号源

### Q: 采样率不准确

**A**: 检查定时器配置：
```c
// TIM2配置
uint32_t timer_clock = HAL_RCC_GetPCLK1Freq() * 2;
uint32_t period = (timer_clock / 26400) - 1;
htim2.Init.Period = period;
```

---

## 📊 性能验证

运行后检查统计信息（每10秒输出）：

```
┌─── Statistics ───────────────────────┐
│ Frames Received: 25
│ Valid Frames: 23
│ CRC Errors: 2
│ Bytes Received: 1024
└──────────────────────────────────────┘
```

**良好指标**:
- CRC错误率 < 5%
- CPU占用 < 40%
- 解码延迟 < 100ms

---

## 🎯 下一步

- ✅ **调整频率**: 修改`Core/Inc/aprs_config.h`中的`RF_FREQUENCY`
- ✅ **启用DSP**: 确保`USE_CMSIS_DSP=1`
- ✅ **优化性能**: 使用-O3编译优化
- ✅ **添加功能**: 参考`STM32_PROJECT_README.md`

---

## 📚 完整文档

- **详细迁移**: `MIGRATION_GUIDE.md`
- **项目说明**: `STM32_PROJECT_README.md`  
- **Keil配置**: `KEIL_PROJECT_SETUP.md`
- **重构总结**: `REFACTOR_COMPLETED.md`

---

## 🆘 需要帮助？

1. 查看文档中的故障排除章节
2. 检查代码注释
3. 参考reference-file中的示例

---

**准备好了？开始编译吧！Good luck! 73! 📻**

