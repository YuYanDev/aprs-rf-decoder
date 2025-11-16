/**
 * NRZI解码器 - 头文件
 * 
 * 实现NRZI (Non-Return-to-Zero Inverted)解码和比特去填充
 * AX.25使用NRZI编码：比特0表示电平跳变，比特1表示电平不变
 */

#ifndef NRZI_DECODER_H
#define NRZI_DECODER_H

#include <Arduino.h>
#include "aprs_config.h"

/**
 * NRZI解码器类
 * 
 * 功能：
 * 1. NRZI解码：将电平跳变模式转换为原始比特流
 * 2. 比特去填充：移除AX.25比特填充 (连续5个1后插入的0)
 * 3. 帧标志检测：检测0x7E帧标志
 * 4. 字节对齐：将比特流组装成字节
 */
class NRZIDecoder {
public:
    /**
     * 构造函数
     */
    NRZIDecoder();

    /**
     * 初始化解码器
     * @return true: 成功, false: 失败
     */
    bool begin();

    /**
     * 处理新的AFSK解调比特
     * @param bit: 输入比特 (0或1)
     */
    void processBit(uint8_t bit);

    /**
     * 批量处理比特
     * @param bits: 比特数组
     * @param length: 数组长度
     */
    void processBits(const uint8_t* bits, size_t length);

    /**
     * 检查是否有完整的字节可用
     * @return true: 有新字节, false: 无新字节
     */
    bool available();

    /**
     * 读取一个解码后的字节
     * @return 字节值
     */
    uint8_t readByte();

    /**
     * 检查是否检测到帧开始
     * @return true: 检测到帧开始, false: 未检测到
     */
    bool isFrameStart();

    /**
     * 检查是否检测到帧结束
     * @return true: 检测到帧结束, false: 未检测到
     */
    bool isFrameEnd();

    /**
     * 清除帧标志
     */
    void clearFrameFlags();

    /**
     * 重置解码器状态
     */
    void reset();

    /**
     * 获取解码统计信息
     */
    void getStats(uint32_t* totalBits, uint32_t* stuffedBits, uint32_t* frameCount);

private:
    // NRZI解码状态
    uint8_t lastLevel;          // 上一个电平状态
    
    // 比特去填充状态
    uint8_t onesCount;          // 连续1的计数
    bool inStuffing;            // 是否在比特填充中
    
    // 字节组装状态
    uint8_t bitBuffer;          // 当前正在组装的字节
    uint8_t bitCount;           // 已组装的比特数
    
    // 帧标志检测
    uint16_t flagDetector;      // 滑动窗口用于检测0x7E标志
    bool frameStartDetected;    // 帧开始标志
    bool frameEndDetected;      // 帧结束标志
    bool inFrame;               // 是否在帧内
    
    // 字节输出缓冲区
    uint8_t byteBuffer[256];
    uint8_t byteBufferHead;
    uint8_t byteBufferTail;
    
    // 统计信息
    uint32_t totalBits;
    uint32_t stuffedBits;
    uint32_t frameCount;
    
    /**
     * NRZI解码单个比特
     * @param nrziBit: NRZI编码的比特
     * @return 解码后的比特
     */
    uint8_t decodeNRZI(uint8_t nrziBit);
    
    /**
     * 比特去填充
     * @param bit: 解码后的比特
     * @return true: 比特有效, false: 比特被去填充
     */
    bool unstuffBit(uint8_t bit);
    
    /**
     * 检测帧标志 (0x7E = 0b01111110)
     * @param bit: 当前比特
     * @return true: 检测到标志, false: 未检测到
     */
    bool detectFlag(uint8_t bit);
    
    /**
     * 将比特添加到字节缓冲区
     * @param bit: 要添加的比特
     */
    void addBitToByte(uint8_t bit);
    
    /**
     * 将完整字节写入输出缓冲区
     * @param byte: 要写入的字节
     */
    void writeByteToBuffer(uint8_t byte);
};

#endif // NRZI_DECODER_H

