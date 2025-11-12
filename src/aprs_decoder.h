/**
 * APRS基础解码器
 * 
 * 整合AFSK解调、NRZI解码和AX.25解析
 * 适用于所有STM32平台
 */

#ifndef APRS_DECODER_H
#define APRS_DECODER_H

#include "aprs_config.h"
#include "afsk_demod.h"
#include "nrzi_decoder.h"
#include "ax25_parser.h"
#include <stdint.h>

// 解码器状态
enum DecoderState {
  STATE_IDLE,           // 空闲，等待载波
  STATE_SYNC,           // 同步，查找帧标志
  STATE_RECEIVING,      // 接收数据
  STATE_COMPLETE        // 帧接收完成
};

// 统计信息
typedef struct {
  uint32_t framesReceived;      // 接收到的帧数
  uint32_t framesValid;         // 有效帧数
  uint32_t framesCRCError;      // CRC错误帧数
  uint32_t bytesReceived;       // 接收到的字节数
  uint32_t carrierLost;         // 载波丢失次数
  uint32_t syncTimeout;         // 同步超时次数
} DecoderStatistics;

class APRSDecoder {
public:
  APRSDecoder();
  
  /**
   * 初始化解码器
   * @return 成功返回true
   */
  bool begin();
  
  /**
   * 处理来自射频模块的采样数据
   * @param sample 采样值 (0或1)
   */
  void processSample(uint8_t sample);
  
  /**
   * 检查是否有可用的解码帧
   * @return 如果有新帧，返回true
   */
  bool available();
  
  /**
   * 获取解码后的AX.25帧
   * @return 指向帧的指针
   */
  AX25Frame* getFrame();
  
  /**
   * 获取APRS消息（信息字段）
   * @param buffer 输出缓冲区
   * @param maxLen 缓冲区大小
   * @return 消息长度
   */
  uint16_t getAPRSMessage(char* buffer, uint16_t maxLen);
  
  /**
   * 重置解码器
   */
  void reset();
  
  /**
   * 获取当前状态
   */
  DecoderState getState();
  
  /**
   * 获取统计信息
   */
  DecoderStatistics* getStatistics();
  
  /**
   * 获取信号质量
   * @return 信号质量 0-100
   */
  uint8_t getSignalQuality();

protected:
  AFSKDemodulator afskDemod;    // AFSK解调器
  NRZIDecoder nrziDecoder;      // NRZI解码器
  AX25Parser ax25Parser;        // AX.25解析器
  
  DecoderState state;           // 当前状态
  bool frameAvailable;          // 帧可用标志
  uint16_t syncTimeout;         // 同步超时计数
  uint16_t byteTimeout;         // 字节超时计数
  uint8_t flagCount;            // 帧标志计数
  
  DecoderStatistics stats;      // 统计信息
  
  /**
   * 状态机处理
   */
  void stateMachine();
  
  /**
   * 处理空闲状态
   */
  void handleIdleState();
  
  /**
   * 处理同步状态
   */
  void handleSyncState();
  
  /**
   * 处理接收状态
   */
  void handleReceivingState();
};

#endif // APRS_DECODER_H

