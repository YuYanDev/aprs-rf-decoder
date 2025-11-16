# STM32 APRS Decoder Project

## 🎯 项目概述

这是一个基于STM32 HAL库的专业APRS解码器项目，从Arduino环境完全重构为STM32原生开发。

### 支持的MCU
- ✅ STM32F401RETx (84 MHz, 512KB Flash, 96KB RAM)
- ✅ STM32F411RETx (100 MHz, 512KB Flash, 128KB RAM)
- ✅ STM32L412KBUx (80 MHz, 128KB Flash, 40KB RAM)

### 核心特性
- ✅ **自实现SX1276/SX1278驱动** - 无需RadioLib依赖
- ✅ **HAL库架构** - 标准STM32开发方式
- ✅ **FPU硬件加速** - 充分利用Cortex-M4F
- ✅ **CMSIS-DSP优化** - 可选的DSP加速
- ✅ **专业代码风格** - 符合工业标准

---

## 📁 项目文件结构

```
aprs-rf-decoder/
├── Core/
│   ├── Inc/                          # 头文件
│   │   ├── main.h                   ✅ 已完成
│   │   ├── aprs_config.h            ✅ 已完成
│   │   ├── sx127x.h                 ✅ 已完成 - SX1276驱动
│   │   ├── afsk_demod.h             ✅ 已完成 - AFSK解调器
│   │   ├── nrzi_decoder.h           ✅ 已完成 - NRZI解码器
│   │   └── ax25_parser.h            ✅ 已完成 - AX.25解析器
│   └── Src/                          # 源文件
│       ├── main.c                   ✅ 已完成 - 主程序
│       ├── sx127x.c                 ✅ 已完成 - SX1276实现
│       ├── afsk_demod.c             ⏳ 需实现
│       ├── nrzi_decoder.c           ⏳ 需实现
│       ├── ax25_parser.c            ⏳ 需实现
│       ├── stm32f4xx_it.c           ⏳ 需添加（从CubeMX生成）
│       ├── stm32f4xx_hal_msp.c      ⏳ 需添加（从CubeMX生成）
│       └── system_stm32f4xx.c       ⏳ 需添加（从CubeMX生成）
├── MIGRATION_GUIDE.md                ✅ 已完成 - 迁移指南
└── STM32_PROJECT_README.md           ✅ 已完成 - 本文件
```

---

## 🚀 快速开始

### 方法1: 使用STM32CubeMX生成基础项目

1. **打开STM32CubeMX**，创建新项目
2. **选择MCU**: STM32F401RETx / STM32F411RETx / STM32L412KBUx
3. **配置外设**（详见 `MIGRATION_GUIDE.md`）:
   - SPI1: 连接SX1276
   - USART1: APRS输出 (9600)
   - USART2: 调试输出 (115200)
   - TIM2: 采样定时器 (26.4 kHz)
   - GPIO: SX1276控制引脚
4. **生成代码**: Project → Generate Code
5. **复制文件**: 将`Core/Inc`和`Core/Src`中的文件复制到生成的项目

### 方法2: 手动配置Keil项目

1. 创建新Keil项目
2. 选择目标MCU
3. 添加所有`Core/Src/*.c`文件
4. 配置Include Paths和宏定义
5. 添加STM32 HAL库和CMSIS-DSP
6. 配置启动文件和链接脚本

详细步骤请参考 **`MIGRATION_GUIDE.md`**

---

## 📝 待完成工作

### 立即需要实现的文件

以下是需要实现的核心算法文件（可参考旧版Arduino代码）：

#### 1. `Core/Src/afsk_demod.c`
```c
/**
  * @brief  AFSK解调器实现
  * @note   使用Goertzel算法检测1200Hz和2200Hz
  * @note   支持CMSIS-DSP加速（FIR滤波）
  */

// 需要实现的函数：
int32_t AFSK_Init(AFSK_Demod_HandleTypeDef *hafsk);
void AFSK_Reset(AFSK_Demod_HandleTypeDef *hafsk);
bool AFSK_ProcessSample(AFSK_Demod_HandleTypeDef *hafsk, uint8_t sample);
uint8_t AFSK_GetBit(AFSK_Demod_HandleTypeDef *hafsk);
uint8_t AFSK_GetSignalQuality(AFSK_Demod_HandleTypeDef *hafsk);
bool AFSK_IsCarrierDetected(AFSK_Demod_HandleTypeDef *hafsk);

// 关键算法：
// - Goertzel滤波器
// - PLL位同步
// - 能量检测
// - 可选：CMSIS-DSP FIR带通滤波
```

#### 2. `Core/Src/nrzi_decoder.c`
```c
/**
  * @brief  NRZI解码器实现
  * @note   NRZI: 无跳变=1, 有跳变=0
  * @note   包含比特去填充（删除连续5个1后的0）
  */

// 需要实现的函数：
int32_t NRZI_Init(NRZI_Decoder_HandleTypeDef *hnrzi);
void NRZI_Reset(NRZI_Decoder_HandleTypeDef *hnrzi);
bool NRZI_ProcessBit(NRZI_Decoder_HandleTypeDef *hnrzi, uint8_t bit);
uint8_t NRZI_GetByte(NRZI_Decoder_HandleTypeDef *hnrzi);
bool NRZI_IsFlagDetected(NRZI_Decoder_HandleTypeDef *hnrzi);

// 关键算法：
// - NRZI解码
// - 比特去填充
// - 帧标志(0x7E)检测
```

#### 3. `Core/Src/ax25_parser.c`
```c
/**
  * @brief  AX.25帧解析器实现
  * @note   解析AX.25 UI帧格式
  * @note   CRC-16-CCITT校验
  */

// 需要实现的函数：
int32_t AX25_Init(AX25_Parser_HandleTypeDef *hax25);
void AX25_Reset(AX25_Parser_HandleTypeDef *hax25);
void AX25_StartFrame(AX25_Parser_HandleTypeDef *hax25);
bool AX25_AddByte(AX25_Parser_HandleTypeDef *hax25, uint8_t byte);
bool AX25_EndFrame(AX25_Parser_HandleTypeDef *hax25);
AX25_Frame_TypeDef* AX25_GetFrame(AX25_Parser_HandleTypeDef *hax25);

// 关键算法：
// - 地址字段解析（呼号+SSID）
// - CRC-16-CCITT计算和校验
// - 信息字段提取
```

### STM32CubeMX生成的文件

这些文件需要从STM32CubeMX生成或从模板复制：

- `Core/Src/stm32f4xx_it.c` - 中断向量表和ISR
- `Core/Src/stm32f4xx_hal_msp.c` - MSP初始化（GPIO、时钟等）
- `Core/Src/system_stm32f4xx.c` - 系统时钟配置
- `Core/Startup/startup_stm32f401xe.s` - 启动文件（汇编）

### HAL库和CMSIS

需要从STM32Cube固件包添加：

```
Drivers/
├── STM32F4xx_HAL_Driver/
│   ├── Inc/
│   │   ├── stm32f4xx_hal.h
│   │   ├── stm32f4xx_hal_gpio.h
│   │   ├── stm32f4xx_hal_spi.h
│   │   ├── stm32f4xx_hal_uart.h
│   │   └── stm32f4xx_hal_tim.h
│   └── Src/
│       ├── stm32f4xx_hal.c
│       ├── stm32f4xx_hal_gpio.c
│       ├── stm32f4xx_hal_spi.c
│       ├── stm32f4xx_hal_uart.c
│       └── stm32f4xx_hal_tim.c
└── CMSIS/
    ├── Device/ST/STM32F4xx/Include/
    │   ├── stm32f4xx.h
    │   └── system_stm32f4xx.h
    └── Include/
        ├── core_cm4.h
        └── arm_math.h

Middlewares/
└── Third_Party/
    └── CMSIS-DSP/
        ├── Include/
        │   └── arm_math.h
        └── Source/
            └── (DSP源文件)
```

---

## 🔧 编译配置

### Keil项目设置

```
Options for Target:
┌─ Target ──────────────────────────────┐
│ Device: STM32F401RETx                 │
│ ✅ Use MicroLIB                       │
│ ✅ Use FPU: Single Precision          │
│ Xtal (MHz): 25.0                      │
└───────────────────────────────────────┘

┌─ C/C++ ───────────────────────────────┐
│ Optimization: -O3 (Level 3)           │
│ Define:                                │
│   STM32F401xE                          │
│   USE_HAL_DRIVER                       │
│   ARM_MATH_CM4                         │
│   __FPU_PRESENT=1                      │
│   USE_CMSIS_DSP=1                      │
│                                        │
│ Include Paths:                         │
│   Core/Inc                             │
│   Drivers/STM32F4xx_HAL_Driver/Inc     │
│   Drivers/CMSIS/Device/.../Include     │
│   Drivers/CMSIS/Include                │
│   Middlewares/.../CMSIS-DSP/Include    │
└───────────────────────────────────────┘

┌─ Linker ──────────────────────────────┐
│ Scatter File: STM32F401RETx_FLASH.sct │
│ ✅ Use Memory Layout from Target      │
└───────────────────────────────────────┘
```

### STM32CubeIDE项目设置

```
Project Properties → C/C++ Build → Settings:

MCU GCC Compiler → Preprocessor:
  STM32F401xE
  USE_HAL_DRIVER
  ARM_MATH_CM4
  __FPU_PRESENT=1U
  USE_CMSIS_DSP=1

MCU GCC Compiler → Include paths:
  ../Core/Inc
  ../Drivers/STM32F4xx_HAL_Driver/Inc
  ../Drivers/CMSIS/Device/ST/STM32F4xx/Include
  ../Drivers/CMSIS/Include
  ../Middlewares/Third_Party/CMSIS-DSP/Include

MCU GCC Compiler → Optimization:
  -O3 (Optimize most)

MCU Settings:
  ✅ Use float with printf from newlib-nano
  Floating Point Unit: FPv4-SP-D16
  Floating Point ABI: FP Instructions (hard)
```

---

## 🎯 核心实现要点

### 1. SX1276驱动 ✅
```c
// 已完成！参考 Core/Src/sx127x.c
// - SPI通信封装
// - 寄存器读写
// - FSK模式配置
// - 直接模式接收
// - DIO2采样
```

### 2. AFSK解调 ⏳
```c
// 关键算法：Goertzel
float omega = 2π × freq / sampleRate;
float coeff = 2 × cos(omega);

for each sample:
    q0 = coeff × q1 - q2 + sample;
    q2 = q1;
    q1 = q0;

magnitude² = q1² + q2² - q1×q2×coeff;

// 比特判决
if (mark_magnitude > space_magnitude)
    bit = 1;
else
    bit = 0;
```

### 3. NRZI解码 ⏳
```c
// NRZI: 跳变=0, 无跳变=1
decoded_bit = (bit == last_bit) ? 1 : 0;
last_bit = bit;

// 比特去填充
if (ones_count == 5) {
    // 跳过下一个0
    continue;
}
```

### 4. AX.25解析 ⏳
```c
// CRC-16-CCITT
for each byte:
    crc ^= byte;
    for (i = 0; i < 8; i++):
        if (crc & 0x0001)
            crc = (crc >> 1) ^ 0x8408;
        else
            crc = crc >> 1;

// 地址解码（每字符左移1位）
callsign[i] = raw_byte >> 1;
ssid = (raw_byte & 0x1E) >> 1;
```

---

## 📊 性能目标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| Flash使用 | < 80 KB | 含HAL库和CMSIS-DSP |
| RAM使用 | < 10 KB | 含缓冲区和状态机 |
| CPU占用 | < 40% | @84MHz, 包含解码 |
| 解码成功率 | > 95% | 良好信号条件 |
| 采样精度 | ±0.1% | 26.4 kHz ±26 Hz |

---

## 🐛 调试建议

### 1. 串口调试
```c
// 使用printf重定向到UART2
printf("Debug: sample=%d, bit=%d\r\n", sample, bit);
```

### 2. 采样率验证
```c
// 在TIM2中断中切换GPIO
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
// 用示波器测量频率应为26.4 kHz
```

### 3. SX1276寄存器读取
```c
uint8_t version = SX127x_ReadRegister(&hsx127x, 0x42);
printf("SX127x Version: 0x%02X\r\n", version);  // 应为0x12
```

### 4. AFSK能量监测
```c
printf("Mark: %d, Space: %d\r\n", 
       hafsk.mark_energy, hafsk.space_energy);
```

---

## 📚 参考资源

### 官方文档
- [STM32F401 Reference Manual (RM0368)](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32F4 HAL User Manual (UM1725)](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
- [CMSIS-DSP Documentation](https://arm-software.github.io/CMSIS-DSP/latest/)
- [SX1276/77/78/79 Datasheet](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1276)

### 协议规范
- [AX.25 Link Access Protocol v2.2](http://www.ax25.net/AX25.2.2-Jul%2098-2.pdf)
- [APRS Protocol Reference v1.0.1](http://www.aprs.org/doc/APRS101.PDF)
- [Bell 202 Modem Specification](https://en.wikipedia.org/wiki/Bell_202_modem)

---

## ✅ 下一步行动

1. **实现核心算法**
   - [ ] 完成 `afsk_demod.c`
   - [ ] 完成 `nrzi_decoder.c`
   - [ ] 完成 `ax25_parser.c`

2. **配置STM32CubeMX**
   - [ ] 生成时钟配置
   - [ ] 生成外设初始化代码
   - [ ] 生成中断处理

3. **集成测试**
   - [ ] 编译通过
   - [ ] 硬件测试
   - [ ] 性能优化

4. **文档完善**
   - [ ] API文档
   - [ ] 使用说明
   - [ ] 示例代码

---

## 📞 支持

如有问题，请参考：
1. `MIGRATION_GUIDE.md` - 详细的迁移指南
2. `README.md` - 项目总体说明
3. 在线文档和社区支持

---

**项目状态**: 🚧 架构完成，核心算法待实现

**最后更新**: 2025-11-12

---

Good luck! 73! 📻

