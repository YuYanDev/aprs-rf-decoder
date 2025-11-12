/**
 * NRZI解码器实现
 */

#include "nrzi_decoder.h"

NRZIDecoder::NRZIDecoder() {
  reset();
}

void NRZIDecoder::begin() {
  reset();
}

void NRZIDecoder::reset() {
  lastBit = 0;
  onesCount = 0;
  rxByte = 0;
  rxBitPos = 0;
  byteReady = false;
  flagDetected = false;
  flagPattern = 0;
}

uint8_t NRZIDecoder::nrziDecode(uint8_t bit) {
  // NRZI解码：如果比特与上一个比特相同，输出1；否则输出0
  uint8_t decoded = (bit == lastBit) ? 1 : 0;
  lastBit = bit;
  return decoded;
}

bool NRZIDecoder::processBit(uint8_t bit) {
  // NRZI解码
  uint8_t decodedBit = nrziDecode(bit);
  
  // 更新标志检测窗口
  flagPattern = (flagPattern << 1) | decodedBit;
  
  // 检测帧标志 0x7E = 01111110
  if (flagPattern == AX25_FLAG) {
    flagDetected = true;
    // 帧标志不计入数据，重置接收器
    rxByte = 0;
    rxBitPos = 0;
    onesCount = 0;
    byteReady = false;
    return false;
  }
  
  flagDetected = false;
  
  // 处理比特填充
  if (decodedBit == 1) {
    onesCount++;
    
    // 如果连续6个1，说明有错误（正常帧应该在5个1后插入0）
    if (onesCount > 6) {
      // 帧错误，重置
      reset();
      return false;
    }
  } else {  // decodedBit == 0
    // 如果前面有5个1，这个0是填充位，丢弃
    if (onesCount == 5) {
      onesCount = 0;
      return false;  // 不将填充位加入数据
    }
    onesCount = 0;
  }
  
  // 正常数据位，加入接收缓冲
  // AX.25使用LSB优先
  rxByte >>= 1;
  if (decodedBit) {
    rxByte |= 0x80;
  }
  
  rxBitPos++;
  
  // 接收到完整字节
  if (rxBitPos >= 8) {
    byteReady = true;
    rxBitPos = 0;
    return true;
  }
  
  return false;
}

uint8_t NRZIDecoder::getDecodedByte() {
  byteReady = false;
  return rxByte;
}

bool NRZIDecoder::isFlagDetected() {
  return flagDetected;
}

uint8_t NRZIDecoder::getOnesCount() {
  return onesCount;
}

