/**
 * APRS基础解码器实现
 */

#include "aprs_decoder.h"
#include <string.h>

// 超时常量（采样点数）
#define SYNC_TIMEOUT    (AFSK_SAMPLE_RATE * 2)   // 2秒同步超时
#define BYTE_TIMEOUT    (SAMPLES_PER_BIT * 20)   // 20比特超时

APRSDecoder::APRSDecoder() {
  reset();
}

bool APRSDecoder::begin() {
  // 初始化各模块
  if (!afskDemod.begin()) {
    return false;
  }
  
  nrziDecoder.begin();
  ax25Parser.begin();
  
  reset();
  
  return true;
}

void APRSDecoder::reset() {
  afskDemod.reset();
  nrziDecoder.reset();
  ax25Parser.reset();
  
  state = STATE_IDLE;
  frameAvailable = false;
  syncTimeout = 0;
  byteTimeout = 0;
  flagCount = 0;
  
  memset(&stats, 0, sizeof(stats));
}

void APRSDecoder::processSample(uint8_t sample) {
  // 1. AFSK解调
  if (afskDemod.processSample(sample)) {
    // 成功解调出一个比特
    uint8_t bit = afskDemod.getDemodulatedBit();
    
    // 2. NRZI解码和比特去填充
    if (nrziDecoder.processBit(bit)) {
      // 成功解码出一个字节
      uint8_t byte = nrziDecoder.getDecodedByte();
      
      // 状态机处理
      switch (state) {
        case STATE_IDLE:
        case STATE_SYNC:
          // 在空闲或同步状态，检查帧标志
          if (nrziDecoder.isFlagDetected()) {
            flagCount++;
            if (flagCount >= 1) {  // 至少1个标志后开始接收
              state = STATE_RECEIVING;
              ax25Parser.startFrame();
              byteTimeout = 0;
              DEBUG_PRINTLN("Frame Start");
            }
          }
          break;
          
        case STATE_RECEIVING:
          // 接收状态
          if (nrziDecoder.isFlagDetected()) {
            // 检测到帧结束标志
            if (ax25Parser.endFrame()) {
              // 帧接收成功
              state = STATE_COMPLETE;
              frameAvailable = true;
              stats.framesReceived++;
              stats.framesValid++;
              DEBUG_PRINTLN("Frame Complete");
            } else {
              // CRC错误
              stats.framesReceived++;
              stats.framesCRCError++;
              state = STATE_IDLE;
              flagCount = 0;
              DEBUG_PRINTLN("Frame CRC Error");
            }
          } else {
            // 正常数据字节
            ax25Parser.addByte(byte);
            stats.bytesReceived++;
            byteTimeout = 0;
          }
          break;
          
        case STATE_COMPLETE:
          // 帧已完成，等待用户读取
          // 如果检测到新的标志，准备接收下一帧
          if (nrziDecoder.isFlagDetected()) {
            if (!frameAvailable) {  // 上一帧已被读取
              state = STATE_SYNC;
              flagCount = 1;
            }
          }
          break;
      }
    }
    
    // 超时处理
    byteTimeout++;
    if (state == STATE_RECEIVING && byteTimeout > BYTE_TIMEOUT) {
      // 接收超时，帧不完整
      DEBUG_PRINTLN("Frame Timeout");
      state = STATE_IDLE;
      flagCount = 0;
      stats.syncTimeout++;
    }
  }
  
  // 载波检测
  if (state == STATE_SYNC) {
    syncTimeout++;
    if (syncTimeout > SYNC_TIMEOUT) {
      state = STATE_IDLE;
      flagCount = 0;
      stats.syncTimeout++;
    }
  }
  
  // 在空闲状态检测载波
  if (state == STATE_IDLE) {
    if (afskDemod.isCarrierDetected()) {
      state = STATE_SYNC;
      syncTimeout = 0;
      flagCount = 0;
      nrziDecoder.reset();
    }
  }
}

bool APRSDecoder::available() {
  return frameAvailable;
}

AX25Frame* APRSDecoder::getFrame() {
  frameAvailable = false;
  return ax25Parser.getFrame();
}

uint16_t APRSDecoder::getAPRSMessage(char* buffer, uint16_t maxLen) {
  if (!frameAvailable) {
    return 0;
  }
  
  AX25Frame* frame = ax25Parser.getFrame();
  
  // 复制信息字段到输出缓冲区
  uint16_t len = frame->infoLen;
  if (len > maxLen - 1) {
    len = maxLen - 1;
  }
  
  memcpy(buffer, frame->info, len);
  buffer[len] = '\0';  // 添加字符串结束符
  
  return len;
}

DecoderState APRSDecoder::getState() {
  return state;
}

DecoderStatistics* APRSDecoder::getStatistics() {
  return &stats;
}

uint8_t APRSDecoder::getSignalQuality() {
  return afskDemod.getSignalQuality();
}

