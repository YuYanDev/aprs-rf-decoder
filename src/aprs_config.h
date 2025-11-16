/**
 * APRS RF Decoder - 配置文件
 * 
 * 针对ESP32C3和SX1276/SX1278优化的APRS解码器配置
 */

#ifndef APRS_CONFIG_H
#define APRS_CONFIG_H

#include <Arduino.h>

// ============================================================================
// 硬件配置
// ============================================================================

// SX1276/SX1278引脚定义 (根据实际硬件修改)
#define SX127X_NSS      5    // NSS/CS引脚
#define SX127X_DIO0     2    // DIO0引脚
#define SX127X_RESET    9    // RESET引脚
#define SX127X_DIO1     3    // DIO1引脚
#define SX127X_DIO2     4    // DIO2引脚 (用于直接模式数据输出)

// UART配置
#define UART_OUTPUT_BAUD    9600
#define UART_TX_PIN         21   // 根据ESP32C3实际引脚修改
#define UART_RX_PIN         20   // 根据ESP32C3实际引脚修改

// ============================================================================
// RF配置
// ============================================================================

// APRS频率 (中国业余频段 144.640 MHz，请根据当地法规调整)
#define APRS_FREQUENCY      144.64  // MHz

// FSK参数
// 采样率选择26.4kHz：26400 = 1200×22 = 2200×12，便于频率检测
#define SAMPLING_RATE       26.4    // kHz (对应beginFSK的比特率参数)

// ============================================================================
// AFSK解调参数
// ============================================================================

// Bell 202标准频率
#define MARK_FREQ           1200    // Hz (逻辑1)
#define SPACE_FREQ          2200    // Hz (逻辑0)

// 每个比特的采样数 (1200 baud)
#define SAMPLES_PER_BIT     22      // 26400 / 1200 = 22

// 解调窗口大小 (采样点数)
#define DEMOD_WINDOW_SIZE   44      // 2倍比特周期，提供更好的频率分辨率

// 相关器参数
// Mark频率：1200Hz，周期 = 26400/1200 = 22 samples
// Space频率：2200Hz，周期 = 26400/2200 = 12 samples
#define MARK_PERIOD         22      // 1200Hz周期对应的采样数
#define SPACE_PERIOD        12      // 2200Hz周期对应的采样数

// PLL时钟恢复参数
#define PLL_LOCKED_THRESHOLD    5   // PLL锁定判定的连续正确采样数
#define PLL_MAX_OFFSET          6   // 最大允许的时钟偏移 (采样点)

// 能量检测阈值调整系数
#define ENERGY_THRESHOLD_RATIO  1.05  // Mark/Space能量比值判决阈值

// ============================================================================
// AX.25参数
// ============================================================================

// AX.25帧标志
#define AX25_FLAG               0x7E

// 地址字段参数
#define AX25_ADDR_LEN           7    // 每个地址字段7字节
#define AX25_SSID_MASK          0x0F
#define AX25_SSID_SPARE_MASK    0x60
#define AX25_ADDR_EXTENSION_BIT 0x01

// 控制字段
#define AX25_CONTROL_UI         0x03  // UI帧 (Unnumbered Information)

// 协议ID
#define AX25_PID_NO_L3          0xF0  // No Layer 3

// 帧长度限制
#define AX25_MAX_FRAME_LEN      330   // 最大AX.25帧长度
#define AX25_MIN_FRAME_LEN      18    // 最小有效帧长度

// ============================================================================
// 缓冲区配置
// ============================================================================

// 采样缓冲区 (环形缓冲)
#define SAMPLE_BUFFER_SIZE      512   // 必须是2的幂次方

// 比特缓冲区
#define BIT_BUFFER_SIZE         4096  // 比特缓冲区大小(字节)

// AX.25帧缓冲区数量
#define FRAME_BUFFER_COUNT      3     // 可同时缓存的帧数量

// ============================================================================
// 调试配置
// ============================================================================

// 调试输出级别
#define DEBUG_LEVEL_NONE        0
#define DEBUG_LEVEL_ERROR       1
#define DEBUG_LEVEL_INFO        2
#define DEBUG_LEVEL_DEBUG       3
#define DEBUG_LEVEL_VERBOSE     4

// 当前调试级别 (设置为DEBUG_LEVEL_NONE可禁用所有调试输出以提升性能)
#define DEBUG_LEVEL             DEBUG_LEVEL_INFO

// 性能监控
#define ENABLE_PERFORMANCE_STATS    true   // 启用性能统计
#define STATS_REPORT_INTERVAL       30000  // 统计报告间隔 (ms)

// ============================================================================
// 性能优化配置
// ============================================================================

// 使用定点数运算替代浮点数 (ESP32C3支持硬件浮点，但定点数更快)
#define USE_FIXED_POINT         false

// 定点数缩放因子
#define FIXED_POINT_SCALE       256

// 启用快速数学函数
#define USE_FAST_MATH           true

// ============================================================================
// 同步字配置
// ============================================================================

// 直接模式同步字 (用于触发接收)
// 这是AX.25前导码的一部分，经过NRZI编码后的模式
// 0x3F03F03F 对应交替的01010模式(2200Hz)和00110011模式(1200Hz)
#define DIRECT_MODE_SYNC_WORD   0x3F03F03F
#define SYNC_WORD_LENGTH        32

// ============================================================================
// 宏定义工具
// ============================================================================

// 调试输出宏
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
  #define DEBUG_ERROR(x) Serial.print("[ERROR] "); Serial.println(x)
#else
  #define DEBUG_ERROR(x)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
  #define DEBUG_INFO(x) Serial.print("[INFO] "); Serial.println(x)
#else
  #define DEBUG_INFO(x)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG
  #define DEBUG_DEBUG(x) Serial.print("[DEBUG] "); Serial.println(x)
#else
  #define DEBUG_DEBUG(x)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
  #define DEBUG_VERBOSE(x) Serial.print("[VERBOSE] "); Serial.println(x)
#else
  #define DEBUG_VERBOSE(x)
#endif

// 性能测量宏
#if ENABLE_PERFORMANCE_STATS
  #define PERF_START(var) unsigned long var = micros()
  #define PERF_END(var, label) Serial.printf("[PERF] %s: %lu us\n", label, micros() - var)
#else
  #define PERF_START(var)
  #define PERF_END(var, label)
#endif

#endif // APRS_CONFIG_H
