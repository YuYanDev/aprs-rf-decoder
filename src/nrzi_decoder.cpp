/**
 * NRZI解码器 - 实现文件
 */

#include "nrzi_decoder.h"

NRZIDecoder::NRZIDecoder() {
    reset();
}

bool NRZIDecoder::begin() {
    reset();
    DEBUG_INFO("NRZI Decoder initialized");
    return true;
}

void NRZIDecoder::reset() {
    // 重置NRZI状态
    lastLevel = 0;
    
    // 重置去填充状态
    onesCount = 0;
    inStuffing = false;
    
    // 重置字节组装
    bitBuffer = 0;
    bitCount = 0;
    
    // 重置帧检测
    flagDetector = 0;
    frameStartDetected = false;
    frameEndDetected = false;
    inFrame = false;
    
    // 清空缓冲区
    memset(byteBuffer, 0, sizeof(byteBuffer));
    byteBufferHead = 0;
    byteBufferTail = 0;
    
    // 重置统计
    totalBits = 0;
    stuffedBits = 0;
    frameCount = 0;
}

void NRZIDecoder::processBit(uint8_t nrziBit) {
    totalBits++;
    
    // 1. NRZI解码
    uint8_t bit = decodeNRZI(nrziBit & 0x01);
    
    // 2. 检测帧标志
    bool isFlag = detectFlag(bit);
    
    if (isFlag) {
        // 检测到0x7E标志
        if (!inFrame) {
            // 帧开始
            frameStartDetected = true;
            inFrame = true;
            frameCount++;
            
            // 重置字节组装
            bitBuffer = 0;
            bitCount = 0;
            onesCount = 0;
            
            DEBUG_DEBUG("Frame start detected");
        } else {
            // 帧结束
            frameEndDetected = true;
            inFrame = false;
            
            DEBUG_DEBUG("Frame end detected");
        }
        
        // 重置标志检测器
        flagDetector = 0;
        onesCount = 0;
        
        return;
    }
    
    // 只处理帧内的数据
    if (!inFrame) {
        return;
    }
    
    // 3. 比特去填充
    bool validBit = unstuffBit(bit);
    
    if (validBit) {
        // 4. 字节组装
        addBitToByte(bit);
    }
}

void NRZIDecoder::processBits(const uint8_t* bits, size_t length) {
    for (size_t i = 0; i < length; i++) {
        processBit(bits[i]);
    }
}

uint8_t NRZIDecoder::decodeNRZI(uint8_t nrziBit) {
    // NRZI编码规则：
    // - 比特0：电平跳变
    // - 比特1：电平保持
    
    uint8_t decodedBit;
    
    if (nrziBit == lastLevel) {
        // 电平保持 -> 比特1
        decodedBit = 1;
    } else {
        // 电平跳变 -> 比特0
        decodedBit = 0;
    }
    
    lastLevel = nrziBit;
    
    return decodedBit;
}

bool NRZIDecoder::unstuffBit(uint8_t bit) {
    if (bit == 1) {
        onesCount++;
        
        if (onesCount > 5) {
            // 连续6个1 - 这是错误，应该被比特填充打断
            DEBUG_ERROR("Invalid bit stuffing detected");
            onesCount = 0;
            return false;
        }
        
        return true;
    } else {
        // bit == 0
        if (onesCount == 5) {
            // 这是填充比特，丢弃它
            stuffedBits++;
            onesCount = 0;
            return false;
        }
        
        onesCount = 0;
        return true;
    }
}

bool NRZIDecoder::detectFlag(uint8_t bit) {
    // 将比特添加到滑动窗口
    flagDetector = (flagDetector << 1) | (bit & 0x01);
    
    // 检查是否匹配0x7E (0b01111110)
    if ((flagDetector & 0xFF) == AX25_FLAG) {
        return true;
    }
    
    return false;
}

void NRZIDecoder::addBitToByte(uint8_t bit) {
    // AX.25使用LSB first (最低位先传输)
    bitBuffer |= (bit & 0x01) << bitCount;
    bitCount++;
    
    if (bitCount >= 8) {
        // 完整的字节
        writeByteToBuffer(bitBuffer);
        
        // 重置字节缓冲
        bitBuffer = 0;
        bitCount = 0;
    }
}

void NRZIDecoder::writeByteToBuffer(uint8_t byte) {
    byteBuffer[byteBufferHead] = byte;
    byteBufferHead = (byteBufferHead + 1) % 256;
    
    // 检查缓冲区溢出
    if (byteBufferHead == byteBufferTail) {
        // 缓冲区满，丢弃最旧的数据
        byteBufferTail = (byteBufferTail + 1) % 256;
        DEBUG_ERROR("Byte buffer overflow");
    }
}

bool NRZIDecoder::available() {
    return (byteBufferHead != byteBufferTail);
}

uint8_t NRZIDecoder::readByte() {
    if (!available()) {
        return 0;
    }
    
    uint8_t byte = byteBuffer[byteBufferTail];
    byteBufferTail = (byteBufferTail + 1) % 256;
    
    return byte;
}

bool NRZIDecoder::isFrameStart() {
    return frameStartDetected;
}

bool NRZIDecoder::isFrameEnd() {
    return frameEndDetected;
}

void NRZIDecoder::clearFrameFlags() {
    frameStartDetected = false;
    frameEndDetected = false;
}

void NRZIDecoder::getStats(uint32_t* totalBits_out, uint32_t* stuffedBits_out, uint32_t* frameCount_out) {
    if (totalBits_out != nullptr) {
        *totalBits_out = totalBits;
    }
    
    if (stuffedBits_out != nullptr) {
        *stuffedBits_out = stuffedBits;
    }
    
    if (frameCount_out != nullptr) {
        *frameCount_out = frameCount;
    }
}

