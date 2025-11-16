/**
 * APRS解码器配置文件
 * 
 * 定义系统级配置参数和常量
 */

#ifndef APRS_CONFIG_H
#define APRS_CONFIG_H

#include <Arduino.h>

// ============================================================================
// 射频配置
// ============================================================================
#define RF_FREQUENCY        434.0       // MHz - APRS频率 (实际使用时改为144.39/144.8等)
#define RF_BITRATE          26.4        // kbps - 采样率
#define RF_DEVIATION        3.0         // kHz - FSK频率偏移

// ============================================================================
// AFSK参数
// ============================================================================
#define AFSK_MARK_FREQ      2200        // Hz - Mark频率 (逻辑1)
#define AFSK_SPACE_FREQ     1200        // Hz - Space频率 (逻辑0)
#define AFSK_BAUD_RATE      1200        // bps - 波特率
#define AFSK_SAMPLE_RATE    26400       // Hz - 采样频率

// 采样点数计算
#define SAMPLES_PER_BIT     (AFSK_SAMPLE_RATE / AFSK_BAUD_RATE)  // 22
#define SAMPLES_PER_MARK    (AFSK_SAMPLE_RATE / AFSK_MARK_FREQ)  // 12
#define SAMPLES_PER_SPACE   (AFSK_SAMPLE_RATE / AFSK_SPACE_FREQ) // 22

// ============================================================================
// AX.25协议参数
// ============================================================================
#define AX25_FLAG           0x7E        // 帧标志
#define AX25_MIN_FRAME_LEN  18          // 最小帧长度
#define AX25_MAX_FRAME_LEN  330         // 最大帧长度
#define AX25_ADDR_LEN       7           // 地址字段长度
#define AX25_CONTROL        0x03        // UI帧控制字段
#define AX25_PID            0xF0        // 无协议标识

// ============================================================================
// 缓冲区配置
// ============================================================================
#define RX_BUFFER_SIZE      512         // 接收缓冲区大小（字节）
#define SAMPLE_BUFFER_SIZE  256         // 采样缓冲区大小
#define BIT_BUFFER_SIZE     (AX25_MAX_FRAME_LEN * 8 + 64)  // 位缓冲区

// ============================================================================
// DMA配置
// ============================================================================
#define USE_DMA             1           // 启用DMA传输

// ============================================================================
// DSP配置
// ============================================================================
// 检测MCU是否支持FPU和DSP
// 注意：CMSIS-DSP需要在Arduino中手动配置链接库，暂时禁用
#if defined(STM32L4xx) || defined(STM32F4xx) || defined(STM32G4xx)
  #define HAS_FPU           1
  #define HAS_DSP           1
  #define USE_CMSIS_DSP     0  // 暂时禁用，避免链接错误
#else
  #define HAS_FPU           0
  #define HAS_DSP           0
  #define USE_CMSIS_DSP     0
#endif

// ============================================================================
// 信号处理参数
// ============================================================================
#define CORRELATION_WINDOW  22          // 相关窗口大小
#define PLL_LOCK_THRESHOLD  16          // PLL锁定阈值
#define CARRIER_DETECT_THR  10          // 载波检测阈值

// ============================================================================
// UART配置
// ============================================================================
#define UART_BAUDRATE       9600        // UART波特率
#define UART_TX_PIN         PA9         // UART TX引脚
#define UART_RX_PIN         PA10        // UART RX引脚

// ============================================================================
// 调试配置
// ============================================================================
#define DEBUG_ENABLED       1           // 启用调试输出
#define DEBUG_UART          Serial      // 调试串口

#if DEBUG_ENABLED
  #define DEBUG_PRINT(...)    DEBUG_UART.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...)  DEBUG_UART.println(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
#endif

// ============================================================================
// 性能统计
// ============================================================================
#define ENABLE_STATISTICS   1           // 启用统计功能

#endif // APRS_CONFIG_H

