/**
 * STM32硬件抽象层
 * 
 * 管理Timer、DMA和UART配置
 * 支持STM32L412、F401、F411、G431
 */

#ifndef STM32_HAL_H
#define STM32_HAL_H

#include "aprs_config.h"
#include "ax25_parser.h"
#include <stdint.h>

// 检测STM32系列
#if defined(STM32L4xx)
  #define MCU_SERIES "STM32L4"
  #define HAS_ADVANCED_TIMER 1
#elif defined(STM32F4xx)
  #define MCU_SERIES "STM32F4"
  #define HAS_ADVANCED_TIMER 1
#elif defined(STM32G4xx)
  #define MCU_SERIES "STM32G4"
  #define HAS_ADVANCED_TIMER 1
#else
  #define MCU_SERIES "STM32"
  #define HAS_ADVANCED_TIMER 0
#endif

// 采样缓冲区（双缓冲）
#define SAMPLE_DMA_BUFFER_SIZE  128

/**
 * Timer配置类
 * 用于精确的采样时钟生成
 */
class SamplingTimer {
public:
  SamplingTimer();
  
  /**
   * 初始化采样定时器
   * @param frequency 采样频率 (Hz)
   * @param callback 采样回调函数
   * @return 成功返回true
   */
  bool begin(uint32_t frequency, void (*callback)(void));
  
  /**
   * 启动定时器
   */
  void start();
  
  /**
   * 停止定时器
   */
  void stop();
  
  /**
   * 获取采样计数
   */
  uint32_t getSampleCount();
  
  /**
   * 重置采样计数
   */
  void resetSampleCount();

protected:
  HardwareTimer* timer;
  uint32_t sampleCount;
  void (*sampleCallback)(void);
  
  /**
   * 选择合适的Timer实例
   */
  HardwareTimer* selectTimer();
};

/**
 * DMA传输管理类
 */
class DMAManager {
public:
  DMAManager();
  
  /**
   * 初始化DMA
   * @param buffer1 缓冲区1
   * @param buffer2 缓冲区2（双缓冲模式）
   * @param bufferSize 缓冲区大小
   * @param callback 传输完成回调
   * @return 成功返回true
   */
  bool begin(uint8_t* buffer1, uint8_t* buffer2, uint16_t bufferSize, 
             void (*callback)(uint8_t* buffer, uint16_t size));
  
  /**
   * 启动DMA传输
   */
  void start();
  
  /**
   * 停止DMA传输
   */
  void stop();
  
  /**
   * 获取当前活动缓冲区
   */
  uint8_t* getActiveBuffer();
  
  /**
   * DMA中断处理（内部使用）
   */
  void handleInterrupt();

protected:
  uint8_t* dmaBuf1;
  uint8_t* dmaBuf2;
  uint16_t bufSize;
  bool useBuffer1;
  void (*transferCallback)(uint8_t* buffer, uint16_t size);
};

/**
 * UART输出类
 * 支持DMA传输和多UART实例
 */
class UARTOutput {
public:
  UARTOutput();
  
  /**
   * 初始化UART
   * @param uart UART实例 (Serial1, Serial2等)
   * @param baudrate 波特率
   * @param useDMA 是否使用DMA
   * @return 成功返回true
   */
  bool begin(HardwareSerial& uart, uint32_t baudrate, bool useDMA = true);
  
  /**
   * 发送字符串
   * @param str 字符串
   */
  void print(const char* str);
  
  /**
   * 发送字符串并换行
   * @param str 字符串
   */
  void println(const char* str);
  
  /**
   * 发送二进制数据
   * @param data 数据缓冲区
   * @param length 数据长度
   */
  void write(const uint8_t* data, uint16_t length);
  
  /**
   * 发送APRS帧（格式化输出）
   * @param frame AX.25帧
   */
  void sendAPRSFrame(APRS_AX25Frame* frame);
  
  /**
   * 检查是否传输忙
   */
  bool isBusy();
  
  /**
   * 等待传输完成
   */
  void flush();

protected:
  HardwareSerial* uartPort;
  bool useDMATransfer;
  uint8_t txBuffer[512];
  uint16_t txBufferPos;
  bool txBusy;
  
  /**
   * 格式化呼号
   */
  void formatCallsign(char* output, APRS_AX25Address* addr);
};

// 全局单例
extern SamplingTimer samplingTimer;
extern DMAManager dmaManager;
extern UARTOutput aprsOutput;

#endif // STM32_HAL_H

