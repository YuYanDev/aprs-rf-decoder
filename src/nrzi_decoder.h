/**
 * NRZI解码器和比特去填充
 * 
 * NRZI (Non-Return-to-Zero Inverted):
 * - 没有跳变 = 1
 * - 有跳变 = 0
 * 
 * 比特填充移除：
 * - 在连续5个1后面插入的0需要被移除
 */

#ifndef NRZI_DECODER_H
#define NRZI_DECODER_H

#include "aprs_config.h"
#include <stdint.h>

class NRZIDecoder {
public:
  NRZIDecoder();
  
  /**
   * 初始化解码器
   */
  void begin();
  
  /**
   * 处理输入比特（已解调的AFSK比特）
   * @param bit 输入比特
   * @return 如果成功解码出一个字节，返回true
   */
  bool processBit(uint8_t bit);
  
  /**
   * 获取解码后的字节
   * @return 解码后的字节
   */
  uint8_t getDecodedByte();
  
  /**
   * 检测帧标志 (0x7E)
   * @return 如果检测到帧标志，返回true
   */
  bool isFlagDetected();
  
  /**
   * 重置解码器
   */
  void reset();
  
  /**
   * 获取连续1的计数（用于检测帧结束或错误）
   */
  uint8_t getOnesCount();

protected:
  uint8_t lastBit;          // 上一个比特（用于NRZI解码）
  uint8_t onesCount;        // 连续1的计数
  uint8_t rxByte;           // 接收字节缓冲
  uint8_t rxBitPos;         // 接收比特位置
  bool byteReady;           // 字节准备好标志
  bool flagDetected;        // 帧标志检测标志
  uint8_t flagPattern;      // 滑动窗口用于检测0x7E
  
  /**
   * NRZI解码
   */
  uint8_t nrziDecode(uint8_t bit);
};

#endif // NRZI_DECODER_H

