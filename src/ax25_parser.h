/**
 * AX.25解析器 - 头文件
 * 
 * 实现AX.25协议帧的解析和验证
 */

#ifndef AX25_PARSER_H
#define AX25_PARSER_H

#include <Arduino.h>
#include "aprs_config.h"

/**
 * AX.25地址结构（重命名以避免与RadioLib冲突）
 */
struct APRS_AX25Address {
    char callsign[7];       // 呼号 (6字符 + null终止)
    uint8_t ssid;           // SSID (0-15)
    bool isLast;            // 是否是地址列表中的最后一个
    
    // 辅助函数：转换为字符串格式 "CALLSIGN-SSID"
    void toString(char* buffer, size_t bufferSize) const {
        if (ssid > 0) {
            snprintf(buffer, bufferSize, "%s-%d", callsign, ssid);
        } else {
            snprintf(buffer, bufferSize, "%s", callsign);
        }
    }
};

/**
 * AX.25帧结构（重命名以避免与RadioLib冲突）
 */
struct APRS_AX25Frame {
    APRS_AX25Address destination;    // 目的地址
    APRS_AX25Address source;         // 源地址
    APRS_AX25Address digipeaters[8]; // 中继器地址列表
    uint8_t numDigipeaters;     // 中继器数量
    
    uint8_t control;            // 控制字段
    uint8_t pid;                // 协议ID
    
    uint8_t info[256];          // 信息字段
    uint16_t infoLength;        // 信息字段长度
    
    uint16_t fcs;               // 帧校验序列
    bool fcsValid;              // FCS校验是否通过
    
    // 重置帧结构
    void reset() {
        memset(this, 0, sizeof(APRS_AX25Frame));
    }
};

/**
 * AX.25解析器类
 * 
 * 功能：
 * 1. 解析AX.25帧结构
 * 2. 提取源地址、目的地址、中继器信息
 * 3. CRC/FCS校验
 * 4. 提取信息字段
 */
class AX25Parser {
public:
    /**
     * 构造函数
     */
    AX25Parser();

    /**
     * 初始化解析器
     * @return true: 成功, false: 失败
     */
    bool begin();

    /**
     * 添加接收到的字节
     * @param byte: 输入字节
     */
    void addByte(uint8_t byte);

    /**
     * 开始新帧
     */
    void startFrame();

    /**
     * 结束当前帧并进行解析
     * @return true: 帧有效, false: 帧无效
     */
    bool endFrame();

    /**
     * 检查是否有完整的有效帧可用
     * @return true: 有新帧, false: 无新帧
     */
    bool available();

    /**
     * 获取解析后的帧
     * @param frame: 输出帧结构的指针
     * @return true: 成功, false: 无可用帧
     */
    bool getFrame(APRS_AX25Frame* frame);

    /**
     * 重置解析器状态
     */
    void reset();

    /**
     * 获取统计信息
     */
    void getStats(uint32_t* totalFrames, uint32_t* validFrames, uint32_t* fcsErrors);

private:
    // 接收缓冲区
    uint8_t rxBuffer[AX25_MAX_FRAME_LEN];
    uint16_t rxLength;
    bool receivingFrame;
    
    // 解析后的帧缓冲区
    APRS_AX25Frame frameBuffer[FRAME_BUFFER_COUNT];
    uint8_t frameBufferHead;
    uint8_t frameBufferTail;
    
    // 统计信息
    uint32_t totalFrames;
    uint32_t validFrames;
    uint32_t fcsErrors;
    
    /**
     * 解析AX.25帧
     * @param data: 帧数据
     * @param length: 数据长度
     * @param frame: 输出帧结构
     * @return true: 解析成功, false: 解析失败
     */
    bool parseFrame(const uint8_t* data, uint16_t length, APRS_AX25Frame* frame);
    
    /**
     * 解析AX.25地址字段
     * @param data: 地址数据 (7字节)
     * @param addr: 输出地址结构
     */
    void parseAddress(const uint8_t* data, APRS_AX25Address* addr);
    
    /**
     * 计算FCS (CRC-16-CCITT)
     * @param data: 数据
     * @param length: 数据长度
     * @return CRC值
     */
    uint16_t calculateFCS(const uint8_t* data, uint16_t length);
    
    /**
     * 验证FCS
     * @param data: 完整帧数据 (包含FCS)
     * @param length: 数据长度
     * @return true: FCS正确, false: FCS错误
     */
    bool verifyFCS(const uint8_t* data, uint16_t length);
    
    /**
     * 将帧添加到输出缓冲区
     * @param frame: 要添加的帧
     */
    void addFrameToBuffer(const APRS_AX25Frame* frame);
};

#endif // AX25_PARSER_H

