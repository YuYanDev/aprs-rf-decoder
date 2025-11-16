/**
 * APRS解码器 - 实现文件
 */

#include "aprs_decoder.h"
#include <math.h>

APRSDecoder::APRSDecoder() {
    reset();
}

bool APRSDecoder::begin() {
    reset();
    DEBUG_INFO("APRS Decoder initialized");
    return true;
}

void APRSDecoder::reset() {
    totalMessages = 0;
    positionReports = 0;
    parseErrors = 0;
}

bool APRSDecoder::decodeFrame(const APRS_AX25Frame* frame, APRSMessage* message) {
    if (frame == nullptr || message == nullptr) {
        return false;
    }
    
    totalMessages++;
    
    // 重置消息结构
    message->reset();
    
    // 1. 提取基本信息 (源、目的、路径)
    extractBasicInfo(frame, message);
    
    // 2. 检查是否有信息字段
    if (frame->infoLength == 0) {
        DEBUG_ERROR("No info field in frame");
        parseErrors++;
        return false;
    }
    
    // 3. 复制原始信息字段
    message->rawInfoLength = (frame->infoLength < 256) ? frame->infoLength : 255;
    memcpy(message->rawInfo, frame->info, message->rawInfoLength);
    message->rawInfo[message->rawInfoLength] = '\0';
    
    // 4. 提取数据类型标识符 (第一个字符)
    message->dataType = (char)frame->info[0];
    message->type = determineMessageType(message->dataType);
    
    // 5. 根据消息类型解析
    switch (message->type) {
        case APRS_TYPE_POSITION:
            if (parsePosition((char*)&frame->info[1], frame->infoLength - 1, &message->position)) {
                positionReports++;
            }
            extractComment((char*)frame->info, frame->infoLength, message->comment, sizeof(message->comment));
            break;
            
        case APRS_TYPE_MESSAGE:
        case APRS_TYPE_STATUS:
        case APRS_TYPE_TELEMETRY:
            extractComment((char*)&frame->info[1], frame->infoLength - 1, message->comment, sizeof(message->comment));
            break;
            
        default:
            // 对于未知类型，尝试提取注释
            extractComment((char*)frame->info, frame->infoLength, message->comment, sizeof(message->comment));
            break;
    }
    
    return true;
}

void APRSDecoder::extractBasicInfo(const APRS_AX25Frame* frame, APRSMessage* message) {
    // 提取源地址
    frame->source.toString(message->source, sizeof(message->source));
    
    // 提取目的地址
    frame->destination.toString(message->destination, sizeof(message->destination));
    
    // 构建路径字符串
    message->path[0] = '\0';
    for (uint8_t i = 0; i < frame->numDigipeaters; i++) {
        char digiStr[16];
        frame->digipeaters[i].toString(digiStr, sizeof(digiStr));
        
        if (i > 0) {
            strncat(message->path, ",", sizeof(message->path) - strlen(message->path) - 1);
        }
        strncat(message->path, digiStr, sizeof(message->path) - strlen(message->path) - 1);
    }
}

APRSMessageType APRSDecoder::determineMessageType(char dataType) {
    switch (dataType) {
        case '!':   // Position without timestamp (no APRS messaging)
        case '=':   // Position without timestamp (with APRS messaging)
        case '/':   // Position with timestamp (no APRS messaging)
        case '@':   // Position with timestamp (with APRS messaging)
            return APRS_TYPE_POSITION;
            
        case ';':   // Object
            return APRS_TYPE_OBJECT;
            
        case ':':   // Message
            return APRS_TYPE_MESSAGE;
            
        case 'T':   // Telemetry
            return APRS_TYPE_TELEMETRY;
            
        case '>':   // Status
            return APRS_TYPE_STATUS;
            
        case '_':   // Weather report (without position)
            return APRS_TYPE_WEATHER;
            
        case '`':   // Mic-E (Current Mic-E data)
        case '\'':  // Mic-E (Old Mic-E data)
            return APRS_TYPE_MIC_E;
            
        default:
            return APRS_TYPE_UNKNOWN;
    }
}

bool APRSDecoder::parsePosition(const char* info, uint16_t length, APRSPosition* position) {
    if (info == nullptr || position == nullptr || length < 19) {
        return false;
    }
    
    // 检测是否为压缩格式
    // 压缩格式特征：第9个字符不是'/'或'\'
    if (length >= 13) {
        char posType = info[8];
        if (posType != '/' && posType != '\\') {
            return parseCompressedPosition(info, position);
        }
    }
    
    // 尝试解析未压缩格式
    return parseUncompressedPosition(info, position);
}

bool APRSDecoder::parseCompressedPosition(const char* info, APRSPosition* position) {
    // 压缩位置格式 (13字节)
    // 格式: /YYYYXXXX$csT
    // 这是一个简化实现，完整实现需要更复杂的解码
    
    if (info[0] == '/') {
        // 压缩格式暂不完全支持，标记为无效
        position->valid = false;
        return false;
    }
    
    position->valid = false;
    return false;
}

bool APRSDecoder::parseUncompressedPosition(const char* info, APRSPosition* position) {
    // 未压缩位置格式
    // 格式: DDMM.MMN/DDDMM.MME$...
    // 例如: 4903.50N/07201.75W>
    
    if (strlen(info) < 19) {
        return false;
    }
    
    // 解析纬度: DDMM.MM
    int latDeg = (info[0] - '0') * 10 + (info[1] - '0');
    float latMin = (info[2] - '0') * 10 + (info[3] - '0') + 
                   (info[5] - '0') * 0.1 + (info[6] - '0') * 0.01;
    char latDir = info[7];  // N or S
    
    if (latDir != 'N' && latDir != 'S') {
        return false;
    }
    
    position->latitude = latDeg + latMin / 60.0;
    if (latDir == 'S') {
        position->latitude = -position->latitude;
    }
    
    // 跳过符号表标识
    position->symbolTable = info[8];
    
    // 解析经度: DDDMM.MM
    int lonDeg = (info[9] - '0') * 100 + (info[10] - '0') * 10 + (info[11] - '0');
    float lonMin = (info[12] - '0') * 10 + (info[13] - '0') + 
                   (info[15] - '0') * 0.1 + (info[16] - '0') * 0.01;
    char lonDir = info[17];  // E or W
    
    if (lonDir != 'E' && lonDir != 'W') {
        return false;
    }
    
    position->longitude = lonDeg + lonMin / 60.0;
    if (lonDir == 'W') {
        position->longitude = -position->longitude;
    }
    
    // 提取符号
    if (strlen(info) > 18) {
        position->symbol = info[18];
    }
    
    // 海拔暂不解析
    position->altitude = -1;
    
    position->valid = true;
    return true;
}

void APRSDecoder::extractComment(const char* info, uint16_t length, char* comment, size_t commentSize) {
    // 跳过位置信息，提取注释
    // 这是一个简化实现，完整实现需要根据不同的数据类型处理
    
    uint16_t offset = 0;
    
    // 对于位置报告，跳过位置数据
    if (length > 19) {
        offset = 19;  // 跳过基本位置信息
    }
    
    // 提取剩余部分作为注释
    uint16_t commentLen = (length > offset) ? (length - offset) : 0;
    if (commentLen > 0 && commentLen < commentSize) {
        memcpy(comment, &info[offset], commentLen);
        comment[commentLen] = '\0';
        
        // 移除不可打印字符
        for (uint16_t i = 0; i < commentLen; i++) {
            if (comment[i] < 32 || comment[i] > 126) {
                comment[i] = ' ';
            }
        }
    } else {
        comment[0] = '\0';
    }
}

void APRSDecoder::getStats(uint32_t* totalMessages_out, uint32_t* positionReports_out, uint32_t* parseErrors_out) {
    if (totalMessages_out != nullptr) {
        *totalMessages_out = totalMessages;
    }
    
    if (positionReports_out != nullptr) {
        *positionReports_out = positionReports;
    }
    
    if (parseErrors_out != nullptr) {
        *parseErrors_out = parseErrors;
    }
}

