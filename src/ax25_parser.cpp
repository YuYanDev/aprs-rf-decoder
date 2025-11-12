/**
 * AX.25帧解析器实现
 */

#include "ax25_parser.h"
#include <string.h>

// CRC-16-CCITT多项式: 0x8408 (反转)
#define CRC_POLYNOMIAL  0x8408
#define CRC_INIT        0xFFFF
#define CRC_GOOD        0xF0B8

AX25Parser::AX25Parser() {
  reset();
}

void AX25Parser::begin() {
  reset();
}

void AX25Parser::reset() {
  memset(&currentFrame, 0, sizeof(AX25Frame));
  memset(rawBuffer, 0, sizeof(rawBuffer));
  rawBufferPos = 0;
  crc = CRC_INIT;
}

void AX25Parser::startFrame() {
  reset();
}

void AX25Parser::parseAddress(uint8_t* buffer, AX25Address* address) {
  // AX.25地址格式：每个字符左移1位编码
  memset(address->callsign, 0, sizeof(address->callsign));
  
  for (int i = 0; i < 6; i++) {
    char c = buffer[i] >> 1;  // 右移1位还原
    if (c != ' ') {  // 移除填充空格
      address->callsign[i] = c;
    }
  }
  
  // SSID在第7个字节的高4位
  address->ssid = (buffer[6] >> 1) & 0x0F;
}

void AX25Parser::updateCRC(uint8_t byte) {
  crc ^= byte;
  for (int i = 0; i < 8; i++) {
    if (crc & 0x0001) {
      crc = (crc >> 1) ^ CRC_POLYNOMIAL;
    } else {
      crc = crc >> 1;
    }
  }
}

bool AX25Parser::checkCRC() {
  // AX.25使用CRC-16-CCITT
  // 正确的帧在计算完所有字节（包括CRC）后，结果应该是CRC_GOOD
  return (crc == CRC_GOOD);
}

bool AX25Parser::addByte(uint8_t byte) {
  // 防止缓冲区溢出
  if (rawBufferPos >= AX25_MAX_FRAME_LEN) {
    return false;
  }
  
  // 存储字节并更新CRC
  rawBuffer[rawBufferPos++] = byte;
  updateCRC(byte);
  
  // 检查最小帧长度
  if (rawBufferPos < AX25_MIN_FRAME_LEN) {
    return false;
  }
  
  return false;  // 在调用endFrame之前不完成解析
}

bool AX25Parser::endFrame() {
  // 检查帧长度
  if (rawBufferPos < AX25_MIN_FRAME_LEN) {
    currentFrame.valid = false;
    return false;
  }
  
  // 校验CRC
  currentFrame.valid = checkCRC();
  if (!currentFrame.valid) {
    DEBUG_PRINTLN("AX.25 CRC Error");
    return false;
  }
  
  // 解析地址字段
  uint16_t pos = 0;
  
  // 目标地址
  parseAddress(&rawBuffer[pos], &currentFrame.destination);
  pos += AX25_ADDR_LEN;
  
  // 源地址
  parseAddress(&rawBuffer[pos], &currentFrame.source);
  pos += AX25_ADDR_LEN;
  
  // 解析中继路径（检查地址扩展位）
  currentFrame.numDigipeaters = 0;
  while ((rawBuffer[pos - 1] & 0x01) == 0 && currentFrame.numDigipeaters < 8) {
    parseAddress(&rawBuffer[pos], &currentFrame.digipeaters[currentFrame.numDigipeaters]);
    currentFrame.numDigipeaters++;
    pos += AX25_ADDR_LEN;
    
    // 防止溢出
    if (pos >= rawBufferPos - 2) break;
  }
  
  // 控制字段
  if (pos < rawBufferPos - 2) {
    currentFrame.control = rawBuffer[pos++];
  }
  
  // PID字段
  if (pos < rawBufferPos - 2) {
    currentFrame.pid = rawBuffer[pos++];
  }
  
  // 信息字段（去除最后2字节CRC）
  currentFrame.infoLen = 0;
  while (pos < rawBufferPos - 2 && currentFrame.infoLen < 256) {
    currentFrame.info[currentFrame.infoLen++] = rawBuffer[pos++];
  }
  
  return true;
}

AX25Frame* AX25Parser::getFrame() {
  return &currentFrame;
}

