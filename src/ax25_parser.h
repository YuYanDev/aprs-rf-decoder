/**
 * AX.25帧解析器
 * 
 * 解析AX.25 UI帧，提取源地址、目标地址、路径和信息字段
 */

#ifndef AX25_PARSER_H
#define AX25_PARSER_H

#include "aprs_config.h"
#include <stdint.h>

// AX.25地址结构
typedef struct {
  char callsign[7];   // 呼号（最多6个字符）
  uint8_t ssid;       // SSID (0-15)
} AX25Address;

// AX.25帧结构
typedef struct {
  AX25Address destination;      // 目标地址
  AX25Address source;            // 源地址
  AX25Address digipeaters[8];    // 中继路径
  uint8_t numDigipeaters;        // 中继数量
  uint8_t control;               // 控制字段
  uint8_t pid;                   // 协议标识
  uint8_t info[256];             // 信息字段
  uint16_t infoLen;              // 信息长度
  bool valid;                    // CRC校验有效
} AX25Frame;

class AX25Parser {
public:
  AX25Parser();
  
  /**
   * 初始化解析器
   */
  void begin();
  
  /**
   * 开始接收新帧
   */
  void startFrame();
  
  /**
   * 添加字节到当前帧
   * @param byte 接收到的字节
   * @return 如果帧接收完成，返回true
   */
  bool addByte(uint8_t byte);
  
  /**
   * 结束当前帧并进行CRC校验
   * @return 如果帧有效，返回true
   */
  bool endFrame();
  
  /**
   * 获取解析后的帧
   * @return 指向解析后帧的指针
   */
  AX25Frame* getFrame();
  
  /**
   * 重置解析器
   */
  void reset();

protected:
  AX25Frame currentFrame;       // 当前帧
  uint8_t rawBuffer[AX25_MAX_FRAME_LEN];  // 原始字节缓冲
  uint16_t rawBufferPos;        // 缓冲位置
  uint16_t crc;                 // CRC累加器
  
  /**
   * 解析地址字段
   * @param buffer 地址字节数组
   * @param address 输出地址结构
   */
  void parseAddress(uint8_t* buffer, AX25Address* address);
  
  /**
   * 更新CRC
   * @param byte 输入字节
   */
  void updateCRC(uint8_t byte);
  
  /**
   * 校验CRC
   * @return 如果CRC正确，返回true
   */
  bool checkCRC();
};

#endif // AX25_PARSER_H

