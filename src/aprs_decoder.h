/**
 * APRS解码器 - 头文件
 * 
 * 实现APRS消息的解析和格式化输出
 */

#ifndef APRS_DECODER_H
#define APRS_DECODER_H

#include <Arduino.h>
#include "aprs_config.h"
#include "ax25_parser.h"

/**
 * APRS消息类型
 */
enum APRSMessageType {
    APRS_TYPE_POSITION,         // 位置报告
    APRS_TYPE_OBJECT,           // 对象报告
    APRS_TYPE_MESSAGE,          // 消息
    APRS_TYPE_TELEMETRY,        // 遥测数据
    APRS_TYPE_STATUS,           // 状态报告
    APRS_TYPE_WEATHER,          // 气象数据
    APRS_TYPE_MIC_E,            // Mic-E编码
    APRS_TYPE_UNKNOWN           // 未知类型
};

/**
 * APRS位置信息
 */
struct APRSPosition {
    float latitude;             // 纬度 (度)
    float longitude;            // 经度 (度)
    int16_t altitude;           // 海拔 (米, -1表示未知)
    char symbol;                // APRS符号
    char symbolTable;           // 符号表标识
    bool valid;                 // 位置是否有效
};

/**
 * APRS消息结构
 */
struct APRSMessage {
    char source[16];            // 源呼号 (CALLSIGN-SSID)
    char destination[16];       // 目的呼号
    char path[64];              // 路径 (中继器列表)
    
    APRSMessageType type;       // 消息类型
    char dataType;              // 数据类型标识符
    
    APRSPosition position;      // 位置信息 (如果有)
    
    char comment[128];          // 注释/附加信息
    char rawInfo[256];          // 原始信息字段
    uint16_t rawInfoLength;     // 原始信息字段长度
    
    // 格式化输出到TNC2格式
    void toTNC2(char* buffer, size_t bufferSize) const {
        // TNC2格式: SOURCE>DEST,PATH:INFO
        int len = snprintf(buffer, bufferSize, "%s>%s", source, destination);
        
        if (path[0] != '\0') {
            len += snprintf(buffer + len, bufferSize - len, ",%s", path);
        }
        
        len += snprintf(buffer + len, bufferSize - len, ":");
        
        // 添加原始信息字段
        if (rawInfoLength > 0 && (len + rawInfoLength < bufferSize - 1)) {
            memcpy(buffer + len, rawInfo, rawInfoLength);
            len += rawInfoLength;
            buffer[len] = '\0';
        }
    }
    
    // 重置消息结构
    void reset() {
        memset(this, 0, sizeof(APRSMessage));
        position.altitude = -1;
    }
};

/**
 * APRS解码器类
 * 
 * 功能：
 * 1. 解析APRS消息格式
 * 2. 提取位置、注释等信息
 * 3. 格式化输出为TNC2格式
 */
class APRSDecoder {
public:
    /**
     * 构造函数
     */
    APRSDecoder();

    /**
     * 初始化解码器
     * @return true: 成功, false: 失败
     */
    bool begin();

    /**
     * 解析AX.25帧为APRS消息
     * @param frame: 输入的AX.25帧
     * @param message: 输出的APRS消息
     * @return true: 解析成功, false: 解析失败
     */
    bool decodeFrame(const APRS_AX25Frame* frame, APRSMessage* message);

    /**
     * 重置解码器状态
     */
    void reset();

    /**
     * 获取统计信息
     */
    void getStats(uint32_t* totalMessages, uint32_t* positionReports, uint32_t* parseErrors);

private:
    // 统计信息
    uint32_t totalMessages;
    uint32_t positionReports;
    uint32_t parseErrors;
    
    /**
     * 从AX.25帧提取基本信息
     * @param frame: AX.25帧
     * @param message: APRS消息
     */
    void extractBasicInfo(const APRS_AX25Frame* frame, APRSMessage* message);
    
    /**
     * 判断APRS消息类型
     * @param dataType: 数据类型标识符
     * @return 消息类型
     */
    APRSMessageType determineMessageType(char dataType);
    
    /**
     * 解析位置报告
     * @param info: 信息字段
     * @param length: 信息字段长度
     * @param position: 输出位置结构
     * @return true: 解析成功, false: 解析失败
     */
    bool parsePosition(const char* info, uint16_t length, APRSPosition* position);
    
    /**
     * 解析压缩位置格式
     * @param info: 信息字段
     * @param position: 输出位置结构
     * @return true: 解析成功, false: 解析失败
     */
    bool parseCompressedPosition(const char* info, APRSPosition* position);
    
    /**
     * 解析未压缩位置格式
     * @param info: 信息字段
     * @param position: 输出位置结构
     * @return true: 解析成功, false: 解析失败
     */
    bool parseUncompressedPosition(const char* info, APRSPosition* position);
    
    /**
     * 提取注释信息
     * @param info: 信息字段
     * @param length: 信息字段长度
     * @param comment: 输出注释缓冲区
     * @param commentSize: 注释缓冲区大小
     */
    void extractComment(const char* info, uint16_t length, char* comment, size_t commentSize);
};

#endif // APRS_DECODER_H

