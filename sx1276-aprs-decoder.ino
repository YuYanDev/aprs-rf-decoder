/**
 * ============================================================================
 * SX1276/SX1278 APRS解码器
 * ============================================================================
 * 
 * 高性能APRS解码器，专为STM32平台优化
 * 
 * 支持的MCU：
 * - STM32L412 (推荐)
 * - STM32F401
 * - STM32F411
 * - STM32G431
 * 
 * 功能特性：
 * - AFSK解调（Bell 202标准）
 * - NRZI解码和比特去填充
 * - AX.25帧解析
 * - 自动载波检测
 * - 信号质量监测
 * - CMSIS-DSP加速（FPU/DSP型号）
 * - DMA高速传输
 * - 统计信息输出
 * 
 * 硬件连接：
 * - SX1276 NSS   -> PA4  (可配置)
 * - SX1276 DIO0  -> PA2  (可配置)
 * - SX1276 DIO2  -> PA3  (可配置，采样引脚)
 * - SX1276 RESET -> PA1  (可配置)
 * - UART1 TX     -> PA9  (APRS输出)
 * - UART1 RX     -> PA10
 * - Debug UART   -> USB串口
 * 
 * 作者: APRS解码器项目
 * 版本: 2.0
 * 日期: 2025
 * 
 * ============================================================================
 */

#include <RadioLib.h>
#include "src/aprs_config.h"
#include "src/aprs_decoder.h"
#include "src/stm32_hal.h"

// 根据是否支持DSP选择解码器
#if USE_CMSIS_DSP
  #include "src/aprs_decoder_enhanced.h"
  APRSDecoderEnhanced decoder;
  #define DECODER_TYPE "Enhanced (CMSIS-DSP)"
#else
  APRSDecoder decoder;
  #define DECODER_TYPE "Standard"
#endif

// ============================================================================
// 硬件配置
// ============================================================================

// SX1276/SX1278引脚定义

#define SX127X_DIO0   PA1
#define SX127X_DIO1   PA2
#define SX127X_DIO2   PA3   // 用于直接模式采样
#define SX127X_NSS    PA4
#define SX127X_SCK    PA5
#define SX127X_MISO   PA6
#define SX127X_MOSI   PA7
#define SX127X_RESET  PB7


// RadioLib模块实例
SX1278 radio = new Module(SX127X_NSS, SX127X_DIO0, SX127X_RESET, DIGITALPIN_NC);

// ============================================================================
// 全局变量
// ============================================================================

// 采样缓冲区（双缓冲）
uint8_t sampleBuffer1[SAMPLE_DMA_BUFFER_SIZE];
uint8_t sampleBuffer2[SAMPLE_DMA_BUFFER_SIZE];

// 统计计数器
uint32_t lastStatsTime = 0;
const uint32_t STATS_INTERVAL = 10000;  // 10秒输出一次统计

// ============================================================================
// 中断服务程序和回调函数
// ============================================================================

/**
 * 读取单个比特（由RadioLib直接模式调用）
 */
void IRAM_ATTR readBit(void) {
  // 读取DIO2引脚的比特值
  uint8_t bit = radio.readBit(SX127X_DIO2);
  
  // 直接处理（实时模式）
  decoder.processSample(bit);
}

/**
 * 采样定时器回调（备用方案）
 */
void samplingTimerCallback(void) {
  // 从DIO2读取比特
  uint8_t bit = digitalRead(SX127X_DIO2);
  decoder.processSample(bit);
}

/**
 * DMA传输完成回调
 */
void dmaTransferCallback(uint8_t* buffer, uint16_t size) {
  // 批量处理采样
  #if USE_CMSIS_DSP
    decoder.processSampleBatch(buffer, size);
  #else
    for (uint16_t i = 0; i < size; i++) {
      decoder.processSample(buffer[i]);
    }
  #endif
}

// ============================================================================
// 初始化函数
// ============================================================================

/**
 * 初始化SX1276/SX1278射频模块
 */
bool initRadio() {
  DEBUG_PRINTLN("=================================");
  DEBUG_PRINTLN("初始化SX1278射频模块...");
  
  // 初始化为FSK模式
  // 频率: 434.0 MHz (测试), 比特率: 26.4 kbps
  int state = radio.beginFSK(RF_FREQUENCY, RF_BITRATE, RF_DEVIATION);
  
  if (state != RADIOLIB_ERR_NONE) {
    DEBUG_PRINT("初始化失败，错误代码: ");
    DEBUG_PRINTLN(state);
    return false;
  }
  
  DEBUG_PRINTLN("SX1278初始化成功");
  
  // 启用OOK模式用于直接解调
  radio.setOOK(true);
  
  // 设置直接模式同步字（AX.25前导码模式）
  // 0x3F03F03F 对应26.4kHz采样率下的AFSK模式
  radio.setDirectSyncWord(0x3F03F03F, 32);
  
  // 设置直接模式回调
  radio.setDirectAction(readBit);
  
  // 启动直接模式接收
  radio.receiveDirect();
  
  DEBUG_PRINTLN("直接模式接收已启动");
  DEBUG_PRINTLN("=================================");
  
  return true;
}

/**
 * 初始化APRS解码器
 */
bool initDecoder() {
  DEBUG_PRINTLN("=================================");
  DEBUG_PRINT("初始化APRS解码器 [");
  DEBUG_PRINT(DECODER_TYPE);
  DEBUG_PRINTLN("]");
  
  if (!decoder.begin()) {
    DEBUG_PRINTLN("解码器初始化失败！");
    return false;
  }
  
  DEBUG_PRINTLN("解码器初始化成功");
  
  #if USE_CMSIS_DSP
    // 启用自适应均衡器（可选）
    // decoder.enableAdaptiveEqualizer(true);
  #endif
  
  DEBUG_PRINTLN("=================================");
  
  return true;
}

/**
 * 初始化UART输出
 */
bool initUART() {
  DEBUG_PRINTLN("=================================");
  DEBUG_PRINTLN("初始化UART输出...");
  
  // 初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  // 初始化APRS输出串口（UART1，9600 bps）
  if (!aprsOutput.begin(UART_INSTANCE, UART_BAUDRATE, USE_DMA)) {
    DEBUG_PRINTLN("UART初始化失败！");
    return false;
  }
  
  DEBUG_PRINTLN("UART初始化成功");
  DEBUG_PRINTLN("=================================");
  
  return true;
}

/**
 * 打印系统信息
 */
void printSystemInfo() {
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("╔════════════════════════════════════════╗");
  DEBUG_PRINTLN("║   SX1276/SX1278 APRS解码器 v2.0      ║");
  DEBUG_PRINTLN("╚════════════════════════════════════════╝");
  DEBUG_PRINTLN("");
  
  DEBUG_PRINT("MCU型号: ");
  DEBUG_PRINTLN(MCU_SERIES);
  
  DEBUG_PRINT("解码器类型: ");
  DEBUG_PRINTLN(DECODER_TYPE);
  
  DEBUG_PRINT("采样率: ");
  DEBUG_PRINT(AFSK_SAMPLE_RATE);
  DEBUG_PRINTLN(" Hz");
  
  DEBUG_PRINT("波特率: ");
  DEBUG_PRINT(AFSK_BAUD_RATE);
  DEBUG_PRINTLN(" bps");
  
  #if HAS_FPU
    DEBUG_PRINTLN("FPU: 已启用");
  #endif
  
  #if HAS_DSP
    DEBUG_PRINTLN("DSP: 已启用 (CMSIS-DSP)");
  #endif
  
  #if USE_DMA
    DEBUG_PRINTLN("DMA: 已启用");
  #endif
  
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("正在监听APRS信号...");
  DEBUG_PRINTLN("----------------------------------------");
}

// ============================================================================
// 主程序
// ============================================================================

void setup() {
  // 初始化串口
  if (!initUART()) {
    while (1) { delay(1000); }  // 停止运行
  }
  
  // 打印系统信息
  printSystemInfo();
  
  // 初始化解码器
  if (!initDecoder()) {
    DEBUG_PRINTLN("致命错误: 解码器初始化失败！");
    while (1) { delay(1000); }
  }
  
  // 初始化射频模块
  if (!initRadio()) {
    DEBUG_PRINTLN("致命错误: 射频模块初始化失败！");
    while (1) { delay(1000); }
  }
  
  // 启动采样定时器（备用方案，如果不使用RadioLib的直接回调）
  // samplingTimer.begin(AFSK_SAMPLE_RATE, samplingTimerCallback);
  // samplingTimer.start();
  
  // 初始化DMA（可选，用于批量处理）
  #if USE_DMA
    // dmaManager.begin(sampleBuffer1, sampleBuffer2, 
    //                  SAMPLE_DMA_BUFFER_SIZE, dmaTransferCallback);
    // dmaManager.start();
  #endif
  
  DEBUG_PRINTLN("系统就绪！");
  DEBUG_PRINTLN("");
  
  lastStatsTime = millis();
}

void loop() {
  // 检查是否有解码完成的帧
  if (decoder.available()) {
    // 获取解码后的帧
    AX25Frame* frame = decoder.getFrame();
    
    if (frame != nullptr && frame->valid) {
      // 发送到UART1
      aprsOutput.sendAPRSFrame(frame);
      
      // 调试输出
      DEBUG_PRINTLN("");
      DEBUG_PRINTLN("╔════════════════════════════════════════╗");
      DEBUG_PRINTLN("║       接收到APRS帧！                  ║");
      DEBUG_PRINTLN("╚════════════════════════════════════════╝");
      
      char callsign[16];
      
      // 源呼号
      DEBUG_PRINT("源地址: ");
      memset(callsign, 0, sizeof(callsign));
      for (int i = 0; i < 7 && frame->source.callsign[i]; i++) {
        callsign[i] = frame->source.callsign[i];
      }
      DEBUG_PRINT(callsign);
      if (frame->source.ssid > 0) {
        DEBUG_PRINT("-");
        DEBUG_PRINT(frame->source.ssid);
      }
      DEBUG_PRINTLN("");
      
      // 目标呼号
      DEBUG_PRINT("目标地址: ");
      memset(callsign, 0, sizeof(callsign));
      for (int i = 0; i < 7 && frame->destination.callsign[i]; i++) {
        callsign[i] = frame->destination.callsign[i];
      }
      DEBUG_PRINT(callsign);
      if (frame->destination.ssid > 0) {
        DEBUG_PRINT("-");
        DEBUG_PRINT(frame->destination.ssid);
      }
      DEBUG_PRINTLN("");
      
      // 信息字段
      DEBUG_PRINT("信息字段: ");
      for (uint16_t i = 0; i < frame->infoLen; i++) {
        DEBUG_PRINT((char)frame->info[i]);
      }
      DEBUG_PRINTLN("");
      
      // 信号质量
      DEBUG_PRINT("信号质量: ");
      DEBUG_PRINT(decoder.getSignalQuality());
      DEBUG_PRINTLN("%");
      
      DEBUG_PRINTLN("----------------------------------------");
    }
  }
  
  // 定期输出统计信息
  if (millis() - lastStatsTime >= STATS_INTERVAL) {
    DecoderStatistics* stats = decoder.getStatistics();
    
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("┌─── 统计信息 ───────────────────────┐");
    DEBUG_PRINT("│ 接收帧数: ");
    DEBUG_PRINT(stats->framesReceived);
    DEBUG_PRINTLN("");
    DEBUG_PRINT("│ 有效帧数: ");
    DEBUG_PRINT(stats->framesValid);
    DEBUG_PRINTLN("");
    DEBUG_PRINT("│ CRC错误: ");
    DEBUG_PRINT(stats->framesCRCError);
    DEBUG_PRINTLN("");
    DEBUG_PRINT("│ 接收字节: ");
    DEBUG_PRINT(stats->bytesReceived);
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("└────────────────────────────────────┘");
    DEBUG_PRINTLN("");
    
    lastStatsTime = millis();
  }
  
  // 短暂延迟，避免CPU满载
  delay(1);
}

