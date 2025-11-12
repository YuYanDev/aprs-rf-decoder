/**
 * STM32硬件抽象层实现
 */

#include "stm32_hal.h"
#include "ax25_parser.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// SamplingTimer 实现
// ============================================================================

SamplingTimer::SamplingTimer() {
  timer = nullptr;
  sampleCount = 0;
  sampleCallback = nullptr;
}

HardwareTimer* SamplingTimer::selectTimer() {
  // 根据不同的STM32系列选择合适的Timer
  // 优先选择高精度Timer
  
#if defined(TIM2)
  return new HardwareTimer(TIM2);
#elif defined(TIM3)
  return new HardwareTimer(TIM3);
#elif defined(TIM4)
  return new HardwareTimer(TIM4);
#else
  #error "No suitable timer found for this STM32 variant"
#endif
}

bool SamplingTimer::begin(uint32_t frequency, void (*callback)(void)) {
  sampleCallback = callback;
  
  // 选择Timer
  timer = selectTimer();
  if (timer == nullptr) {
    return false;
  }
  
  // 配置Timer频率
  // 设置overflow频率为采样频率
  timer->setOverflow(frequency, HERTZ_FORMAT);
  
  // 附加中断回调
  timer->attachInterrupt([this]() {
    this->sampleCount++;
    if (this->sampleCallback != nullptr) {
      this->sampleCallback();
    }
  });
  
  DEBUG_PRINT("Sampling Timer initialized: ");
  DEBUG_PRINT(frequency);
  DEBUG_PRINTLN(" Hz");
  
  return true;
}

void SamplingTimer::start() {
  if (timer != nullptr) {
    timer->resume();
    DEBUG_PRINTLN("Sampling Timer started");
  }
}

void SamplingTimer::stop() {
  if (timer != nullptr) {
    timer->pause();
    DEBUG_PRINTLN("Sampling Timer stopped");
  }
}

uint32_t SamplingTimer::getSampleCount() {
  return sampleCount;
}

void SamplingTimer::resetSampleCount() {
  sampleCount = 0;
}

// ============================================================================
// DMAManager 实现
// ============================================================================

DMAManager::DMAManager() {
  dmaBuf1 = nullptr;
  dmaBuf2 = nullptr;
  bufSize = 0;
  useBuffer1 = true;
  transferCallback = nullptr;
}

bool DMAManager::begin(uint8_t* buffer1, uint8_t* buffer2, uint16_t bufferSize, 
                       void (*callback)(uint8_t* buffer, uint16_t size)) {
  dmaBuf1 = buffer1;
  dmaBuf2 = buffer2;
  bufSize = bufferSize;
  transferCallback = callback;
  useBuffer1 = true;
  
  // DMA配置将在实际的STM32环境中完成
  // 这里提供框架
  
  DEBUG_PRINTLN("DMA Manager initialized (双缓冲模式)");
  
  return true;
}

void DMAManager::start() {
  // 启动DMA传输
  DEBUG_PRINTLN("DMA Transfer started");
}

void DMAManager::stop() {
  // 停止DMA传输
  DEBUG_PRINTLN("DMA Transfer stopped");
}

uint8_t* DMAManager::getActiveBuffer() {
  return useBuffer1 ? dmaBuf1 : dmaBuf2;
}

void DMAManager::handleInterrupt() {
  // DMA传输完成中断
  uint8_t* completedBuffer = useBuffer1 ? dmaBuf1 : dmaBuf2;
  
  // 切换缓冲区
  useBuffer1 = !useBuffer1;
  
  // 调用回调处理完成的缓冲区
  if (transferCallback != nullptr) {
    transferCallback(completedBuffer, bufSize);
  }
}

// ============================================================================
// UARTOutput 实现
// ============================================================================

UARTOutput::UARTOutput() {
  uartPort = nullptr;
  useDMATransfer = false;
  txBufferPos = 0;
  txBusy = false;
}

bool UARTOutput::begin(HardwareSerial& uart, uint32_t baudrate, bool useDMA) {
  uartPort = &uart;
  useDMATransfer = useDMA;
  
  // 初始化UART
  uartPort->begin(baudrate);
  
  DEBUG_PRINT("UART initialized: ");
  DEBUG_PRINT(baudrate);
  DEBUG_PRINTLN(" bps");
  
  return true;
}

void UARTOutput::print(const char* str) {
  if (uartPort != nullptr) {
    uartPort->print(str);
  }
}

void UARTOutput::println(const char* str) {
  if (uartPort != nullptr) {
    uartPort->println(str);
  }
}

void UARTOutput::write(const uint8_t* data, uint16_t length) {
  if (uartPort != nullptr) {
    uartPort->write(data, length);
  }
}

void UARTOutput::formatCallsign(char* output, AX25Address* addr) {
  // 格式化呼号为 "CALL-SSID" 格式
  int len = 0;
  for (int i = 0; i < 7 && addr->callsign[i] != '\0'; i++) {
    output[len++] = addr->callsign[i];
  }
  
  if (addr->ssid > 0) {
    output[len++] = '-';
    if (addr->ssid >= 10) {
      output[len++] = '0' + (addr->ssid / 10);
    }
    output[len++] = '0' + (addr->ssid % 10);
  }
  
  output[len] = '\0';
}

void UARTOutput::sendAPRSFrame(AX25Frame* frame) {
  if (uartPort == nullptr || frame == nullptr || !frame->valid) {
    return;
  }
  
  char buffer[512];
  char srcCall[16], dstCall[16];
  
  // 格式化源和目标呼号
  formatCallsign(srcCall, &frame->source);
  formatCallsign(dstCall, &frame->destination);
  
  // 构建输出字符串
  // 格式: SOURCE>DESTINATION[,PATH]:INFO
  int pos = 0;
  pos += sprintf(buffer + pos, "%s>%s", srcCall, dstCall);
  
  // 添加中继路径
  for (uint8_t i = 0; i < frame->numDigipeaters; i++) {
    char digiCall[16];
    formatCallsign(digiCall, &frame->digipeaters[i]);
    pos += sprintf(buffer + pos, ",%s", digiCall);
  }
  
  // 添加信息字段
  pos += sprintf(buffer + pos, ":");
  for (uint16_t i = 0; i < frame->infoLen && pos < 500; i++) {
    buffer[pos++] = frame->info[i];
  }
  buffer[pos] = '\0';
  
  // 发送
  println(buffer);
}

bool UARTOutput::isBusy() {
  return txBusy;
}

void UARTOutput::flush() {
  if (uartPort != nullptr) {
    uartPort->flush();
  }
}

// ============================================================================
// 全局单例实例
// ============================================================================

SamplingTimer samplingTimer;
DMAManager dmaManager;
UARTOutput aprsOutput;

