/**
 * AX.25解析器 - 实现文件
 */

#include "ax25_parser.h"

AX25Parser::AX25Parser() {
    reset();
}

bool AX25Parser::begin() {
    reset();
    DEBUG_INFO("AX.25 Parser initialized");
    return true;
}

void AX25Parser::reset() {
    // 清空接收缓冲区
    memset(rxBuffer, 0, sizeof(rxBuffer));
    rxLength = 0;
    receivingFrame = false;
    
    // 清空帧缓冲区
    memset(frameBuffer, 0, sizeof(frameBuffer));
    frameBufferHead = 0;
    frameBufferTail = 0;
    
    // 重置统计
    totalFrames = 0;
    validFrames = 0;
    fcsErrors = 0;
}

void AX25Parser::startFrame() {
    rxLength = 0;
    receivingFrame = true;
}

void AX25Parser::addByte(uint8_t byte) {
    if (!receivingFrame) {
        return;
    }
    
    if (rxLength < AX25_MAX_FRAME_LEN) {
        rxBuffer[rxLength++] = byte;
    } else {
        DEBUG_ERROR("Frame buffer overflow");
        receivingFrame = false;
    }
}

bool AX25Parser::endFrame() {
    receivingFrame = false;
    totalFrames++;
    
    // 检查最小帧长度
    if (rxLength < AX25_MIN_FRAME_LEN) {
        DEBUG_ERROR("Frame too short");
        return false;
    }
    
    // 解析帧
    APRS_AX25Frame frame;
    frame.reset();
    
    if (parseFrame(rxBuffer, rxLength, &frame)) {
        // 帧有效，添加到缓冲区
        addFrameToBuffer(&frame);
        validFrames++;
        return true;
    }
    
    return false;
}

bool AX25Parser::parseFrame(const uint8_t* data, uint16_t length, APRS_AX25Frame* frame) {
    uint16_t offset = 0;
    
    // 1. 解析目的地址 (7字节)
    if (offset + AX25_ADDR_LEN > length) {
        return false;
    }
    parseAddress(&data[offset], &frame->destination);
    offset += AX25_ADDR_LEN;
    
    // 2. 解析源地址 (7字节)
    if (offset + AX25_ADDR_LEN > length) {
        return false;
    }
    parseAddress(&data[offset], &frame->source);
    offset += AX25_ADDR_LEN;
    
    // 3. 解析中继器地址 (可选，每个7字节)
    frame->numDigipeaters = 0;
    while (frame->numDigipeaters < 8) {
        // 检查前一个地址的扩展位
        if (data[offset - 1] & AX25_ADDR_EXTENSION_BIT) {
            // 这是最后一个地址
            break;
        }
        
        if (offset + AX25_ADDR_LEN > length) {
            return false;
        }
        
        parseAddress(&data[offset], &frame->digipeaters[frame->numDigipeaters]);
        frame->numDigipeaters++;
        offset += AX25_ADDR_LEN;
    }
    
    // 4. 解析控制字段 (1字节)
    if (offset + 1 > length) {
        return false;
    }
    frame->control = data[offset++];
    
    // 5. 解析PID (1字节) - 仅对UI帧
    if ((frame->control & 0x03) == 0x03) {  // UI帧
        if (offset + 1 > length) {
            return false;
        }
        frame->pid = data[offset++];
    }
    
    // 6. 提取信息字段 (剩余数据 - 2字节FCS)
    if (offset + 2 > length) {
        return false;
    }
    
    frame->infoLength = length - offset - 2;
    if (frame->infoLength > 0 && frame->infoLength <= 256) {
        memcpy(frame->info, &data[offset], frame->infoLength);
    }
    offset += frame->infoLength;
    
    // 7. 提取FCS (最后2字节)
    frame->fcs = data[offset] | (data[offset + 1] << 8);
    
    // 8. 验证FCS
    frame->fcsValid = verifyFCS(data, length);
    
    if (!frame->fcsValid) {
        fcsErrors++;
        DEBUG_ERROR("FCS verification failed");
        return false;
    }
    
    return true;
}

void AX25Parser::parseAddress(const uint8_t* data, APRS_AX25Address* addr) {
    // AX.25地址编码：每个字符左移1位
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t c = data[i] >> 1;  // 右移1位恢复原字符
        
        // 去除填充空格
        if (c == ' ') {
            addr->callsign[i] = '\0';
            break;
        }
        
        addr->callsign[i] = c;
    }
    addr->callsign[6] = '\0';  // 确保字符串终止
    
    // 解析SSID和扩展位
    uint8_t ssidByte = data[6];
    addr->ssid = (ssidByte >> 1) & AX25_SSID_MASK;
    addr->isLast = (ssidByte & AX25_ADDR_EXTENSION_BIT) != 0;
}

uint16_t AX25Parser::calculateFCS(const uint8_t* data, uint16_t length) {
    // CRC-16-CCITT (0xFFFF初始值, 多项式0x1021)
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    
    return crc ^ 0xFFFF;  // 最终异或
}

bool AX25Parser::verifyFCS(const uint8_t* data, uint16_t length) {
    if (length < 2) {
        return false;
    }
    
    // 计算除FCS外的所有数据的CRC
    uint16_t calculatedFCS = calculateFCS(data, length - 2);
    
    // 提取接收到的FCS (小端序)
    uint16_t receivedFCS = data[length - 2] | (data[length - 1] << 8);
    
    return (calculatedFCS == receivedFCS);
}

void AX25Parser::addFrameToBuffer(const APRS_AX25Frame* frame) {
    // 复制帧到缓冲区
    memcpy(&frameBuffer[frameBufferHead], frame, sizeof(APRS_AX25Frame));
    frameBufferHead = (frameBufferHead + 1) % FRAME_BUFFER_COUNT;
    
    // 检查缓冲区溢出
    if (frameBufferHead == frameBufferTail) {
        // 缓冲区满，丢弃最旧的帧
        frameBufferTail = (frameBufferTail + 1) % FRAME_BUFFER_COUNT;
        DEBUG_ERROR("Frame buffer overflow");
    }
}

bool AX25Parser::available() {
    return (frameBufferHead != frameBufferTail);
}

bool AX25Parser::getFrame(APRS_AX25Frame* frame) {
    if (!available()) {
        return false;
    }
    
    // 复制帧
    memcpy(frame, &frameBuffer[frameBufferTail], sizeof(APRS_AX25Frame));
    frameBufferTail = (frameBufferTail + 1) % FRAME_BUFFER_COUNT;
    
    return true;
}

void AX25Parser::getStats(uint32_t* totalFrames_out, uint32_t* validFrames_out, uint32_t* fcsErrors_out) {
    if (totalFrames_out != nullptr) {
        *totalFrames_out = totalFrames;
    }
    
    if (validFrames_out != nullptr) {
        *validFrames_out = validFrames;
    }
    
    if (fcsErrors_out != nullptr) {
        *fcsErrors_out = fcsErrors;
    }
}

