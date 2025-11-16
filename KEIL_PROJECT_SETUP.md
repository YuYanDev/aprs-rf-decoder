# Keil μVision Project Setup Guide

本文档说明如何在Keil MDK-ARM中创建APRS Decoder项目。

---

## 📋 项目配置文件

由于Keil项目文件（.uvprojx）是二进制格式，这里提供详细的手动配置步骤。

---

## 🔧 创建新项目

### 1. 启动Keil并创建项目

```
Project → New μVision Project...
├─ 项目名称: APRS_Decoder
├─ 保存位置: <project_root>/MDK-ARM/
└─ 点击 "保存"
```

### 2. 选择目标器件

根据您的MCU选择：

```
┌─ STM32F401 ─────────────────────────┐
│ STMicroelectronics                  │
│ ├─ STM32F4 Series                   │
│ │  └─ STM32F401                     │
│ │     └─ STM32F401RE (512KB Flash)  │
└─────────────────────────────────────┘

或

┌─ STM32F411 ─────────────────────────┐
│ STMicroelectronics                  │
│ ├─ STM32F4 Series                   │
│ │  └─ STM32F411                     │
│ │     └─ STM32F411RE (512KB Flash)  │
└─────────────────────────────────────┘

或

┌─ STM32L412 ─────────────────────────┐
│ STMicroelectronics                  │
│ ├─ STM32L4 Series                   │
│ │  └─ STM32L412                     │
│ │     └─ STM32L412KB (128KB Flash)  │
└─────────────────────────────────────┘
```

点击 "OK"，Keil会询问是否添加Startup Code和CMSIS。选择"是"。

---

## 📁 添加源文件

### 3. 创建文件组

在Project窗口右键 → `Manage Project Items`

```
Groups:
├─ Application/User
├─ Application/HAL
├─ Drivers/STM32F4xx_HAL_Driver
├─ Drivers/CMSIS
├─ Middlewares/CMSIS-DSP
└─ Startup
```

### 4. 添加文件到各组

#### Application/User
```
../Core/Src/main.c
../Core/Src/sx127x.c
../Core/Src/afsk_demod.c
../Core/Src/nrzi_decoder.c
../Core/Src/ax25_parser.c
../Core/Src/stm32f4xx_it.c
../Core/Src/stm32f4xx_hal_msp.c
```

#### Application/HAL
```
../Core/Src/system_stm32f4xx.c
```

#### Drivers/STM32F4xx_HAL_Driver
```
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
```

#### Drivers/CMSIS
```
(无.c文件，只有头文件)
```

#### Middlewares/CMSIS-DSP
```
../Middlewares/Third_Party/CMSIS-DSP/Source/BasicMathFunctions/arm_*.c
../Middlewares/Third_Party/CMSIS-DSP/Source/FastMathFunctions/arm_*.c
../Middlewares/Third_Party/CMSIS-DSP/Source/FilteringFunctions/arm_*.c
../Middlewares/Third_Party/CMSIS-DSP/Source/StatisticsFunctions/arm_*.c
../Middlewares/Third_Party/CMSIS-DSP/Source/SupportFunctions/arm_*.c
../Middlewares/Third_Party/CMSIS-DSP/Source/TransformFunctions/arm_*.c
../Middlewares/Third_Party/CMSIS-DSP/Source/CommonTables/arm_*.c

(可以只添加需要的函数，或全部添加)
```

#### Startup
```
../Core/Startup/startup_stm32f401xe.s
(根据MCU型号选择对应的启动文件)
```

---

## ⚙️ 项目配置

### 5. 配置Target选项

`Project → Options for Target 'Target 1'`

#### Target标签页
```
Device: STM32F401RETx (已自动设置)

✅ Use MicroLIB
   (勾选以获得更小的C库)

ARM Compiler: Use default compiler version 6

Code Generation:
  ✅ Use Single Precision
  Floating Point Hardware: FPv4-SP-D16
```

#### C/C++ (AC6)标签页

**Preprocessor Symbols → Define:**
```
STM32F401xE
USE_HAL_DRIVER
ARM_MATH_CM4
__FPU_PRESENT=1U
USE_CMSIS_DSP=1
HSE_VALUE=25000000
```

**Include Paths:**
```
../Core/Inc
../Drivers/STM32F4xx_HAL_Driver/Inc
../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
../Drivers/CMSIS/Device/ST/STM32F4xx/Include
../Drivers/CMSIS/Include
../Middlewares/Third_Party/CMSIS-DSP/Include
```

**Optimization:**
```
Optimization: -O3 (Optimize most)

Warnings: 
  ✅ All Warnings

Language C:
  Language C: c11
  ✅ GNU Extensions
```

**Misc Controls:**
```
-fshort-enums -fshort-wchar
```

#### Linker标签页
```
✅ Use Memory Layout from Target Dialog

Scatter File:
  (自动生成，基于器件)

Misc Controls:
  --diag_suppress=L6312
```

#### Debug标签页
```
Debugger: ST-Link Debugger

Settings → Debug:
  Port: SW (Serial Wire)
  Max Clock: 4 MHz
  
Settings → Flash Download:
  ✅ Reset and Run
  ✅ Download Function
  
Programming Algorithm:
  STM32F4xx Flash
```

---

## 🔍 编译设置优化

### 6. 优化配置（Options → C/C++）

```
Optimization Level: -O3

Optimization for:
  ⚡ Time (Speed优先)

Code Generation:
  ✅ Short enums/wchar
  ✅ One ELF Section per Function
  ✅ Link-Time Optimization (LTO)
```

### 7. 链接器优化（Options → Linker）

```
✅ Disable Warnings: L6312

Misc Controls:
  --summary_stderr --info summarysizes
```

---

## 📊 内存配置

### 8. 设置栈和堆大小

编辑启动文件 `startup_stm32f401xe.s`:

```assembly
; Amount of memory (in bytes) allocated for Stack
; Tailor this value to your application needs
Stack_Size      EQU     0x1000          ; 4KB Stack

; Amount of memory (in bytes) allocated for Heap
; Tailor this value to your application needs
Heap_Size       EQU     0x800           ; 2KB Heap
```

或在Keil中：
```
Options → Target → Linker → 
  Memory Areas:
    IRAM1: 0x20000000, Size: 0x00018000 (96KB for F401)
    IROM1: 0x08000000, Size: 0x00080000 (512KB for F401)
```

---

## 🚀 编译和下载

### 9. 编译项目

```
Project → Build Target (F7)

Expected Output:
  0 Error(s), 0 Warning(s)
  
Program Size: 
  Code=xxxxx 
  RO-data=xxxx 
  RW-data=xxx 
  ZI-data=xxxx
```

### 10. 下载到MCU

```
Flash → Download (F8)

或

Debug → Start/Stop Debug Session (Ctrl+F5)
```

---

## 🔬 调试配置

### 11. 串口重定向（printf支持）

确保以下配置：

1. **使用MicroLIB**（已在Target中勾选）

2. **在main.c中添加重定向代码**（已包含）：
```c
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

3. **Options → Target → Code Generation**
   ```
   ✅ Use MicroLIB
   ```

### 12. 实时查看变量

在Debug模式下：
```
View → Watch Windows → Watch 1

添加监视变量：
  decoder_state
  hafsk.carrier_detected
  decoder_stats.frames_received
  decoder_stats.frames_valid
```

### 13. Logic Analyzer

```
View → Analysis Windows → Logic Analyzer

添加信号：
  GPIOA, Pin 3 (DIO2 - 采样信号)
  Timer2 (采样时钟)
```

---

## 📝 常见问题

### Q1: 编译错误 `cannot open source input file`

**A:** 检查Include Paths设置，确保所有路径正确且使用相对路径。

### Q2: 链接错误 `undefined reference to xxx`

**A:** 
1. 检查是否添加了所有必需的HAL库文件
2. 检查是否启用了MicroLIB
3. 检查启动文件是否正确

### Q3: 下载失败 `No ST-LINK detected`

**A:**
1. 检查ST-Link连接
2. 安装ST-Link驱动
3. 在Keil中选择正确的调试器

### Q4: printf无输出

**A:**
1. 确保启用了MicroLIB
2. 检查UART2初始化
3. 检查_write函数实现
4. 串口工具波特率设为115200

### Q5: Hard Fault错误

**A:**
1. 检查栈大小（建议4KB）
2. 确保FPU已正确配置
3. 检查数组越界
4. 查看Fault Analyzer输出

---

## 🎯 项目完整清单

### 必需文件 ✅

- [x] Core/Inc/*.h - 所有头文件
- [x] Core/Src/*.c - 所有源文件
- [x] Drivers/STM32F4xx_HAL_Driver/* - HAL库
- [x] Drivers/CMSIS/* - CMSIS核心
- [x] Middlewares/Third_Party/CMSIS-DSP/* - DSP库
- [x] Core/Startup/startup_stm32f401xe.s - 启动文件

### 必需配置 ✅

- [x] Target设置（器件、MicroLIB、FPU）
- [x] C/C++设置（宏定义、Include路径、优化）
- [x] Linker设置（Scatter文件、内存布局）
- [x] Debug设置（ST-Link配置）

### 可选优化 ⭐

- [ ] Link-Time Optimization (LTO)
- [ ] 移除未使用的函数
- [ ] 使用-Os优化代码大小
- [ ] 自定义链接脚本

---

## 📚 参考资源

- [Keil MDK-ARM用户指南](https://www.keil.com/support/man/docs/uv4/)
- [STM32F4 HAL Driver User Manual](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
- [CMSIS-DSP Library Documentation](https://arm-software.github.io/CMSIS-DSP/latest/)

---

**项目配置完成！Ready to compile! 🚀**

