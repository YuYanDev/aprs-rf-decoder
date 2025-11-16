/**
 * APRS RF Decoder for ESP32C3
 * 
 * 高性能APRS解码器，基于SX1276/SX1278和ESP32C3
 * 
 * 功能：
 * - AFSK解调 (Bell 202, 1200/2200Hz)
 * - NRZI解码和比特去填充
 * - AX.25帧解析
 * - APRS消息解码
 * - UART输出 (TNC2格式)
 * 
 * 硬件连接：
 * - SX1276/SX1278 -> ESP32C3 (详见aprs_config.h)
 * - UART1输出 -> 9600 baud
 * 
 * 作者: [Your Name]
 * 日期: 2025
 * 许可: MIT
 */

#include <RadioLib.h>
#include "src/aprs_config.h"
#include "src/afsk_demod.h"
#include "src/nrzi_decoder.h"
#include "src/ax25_parser.h"
#include "src/aprs_decoder.h"

// ============================================================================
// 全局对象
// ============================================================================

// SX1278模块实例
SX1278 radio = new Module(SX127X_NSS, SX127X_DIO0, SX127X_RESET, SX127X_DIO1);

// 解码器链
AFSKDemodulator afskDemod;
NRZIDecoder nrziDecoder;
AX25Parser ax25Parser;
APRSDecoder aprsDecoder;

// UART输出
HardwareSerial UARTOutput(1);  // UART1

// 性能统计
unsigned long lastStatsTime = 0;
unsigned long lastProcessTime = 0;

// ============================================================================
// 中断服务函数 - DIO2数据接收
// ============================================================================

/**
 * 直接模式接收中断处理函数
 * 在每个采样点被调用 (26.4kHz频率)
 */
void IRAM_ATTR readBit(void) {
    // 读取DIO2引脚的数据位
    radio.readBit(SX127X_DIO2);
}

// ============================================================================
// 初始化函数
// ============================================================================

void setup() {
    // 初始化串口监视器
    Serial.begin(115200);
    delay(100);
    
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("  APRS RF Decoder for ESP32C3"));
    Serial.println(F("  Version 1.0"));
    Serial.println(F("========================================"));
    Serial.println();
    
    // 初始化UART输出
    UARTOutput.begin(UART_OUTPUT_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    DEBUG_INFO("UART output initialized at 9600 baud");
    
    // 初始化SX1278
    Serial.print(F("[SX1278] Initializing ... "));
    
    // 使用FSK模式，采样率26.4kHz
    // beginFSK(freq, br, freqDev, rxBw, power, preambleLength)
    int state = radio.beginFSK(APRS_FREQUENCY, SAMPLING_RATE);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        Serial.println(F("Check wiring and configuration!"));
        while (true) { delay(100); }
    }
    
    // 启用OOK模式用于直接模式接收
    radio.setOOK(true);
    
    // 设置直接模式同步字
    // 这是AX.25前导码的一部分，用于触发接收
    radio.setDirectSyncWord(DIRECT_MODE_SYNC_WORD, SYNC_WORD_LENGTH);
    
    // 设置中断回调函数
    radio.setDirectAction(readBit);
    
    DEBUG_INFO("SX1278 configured for direct mode reception");
    
    // 初始化解码器链
    Serial.println(F("[Decoders] Initializing decoder chain ..."));
    
    if (!afskDemod.begin()) {
        Serial.println(F("  AFSK Demodulator initialization failed!"));
        while (true) { delay(100); }
    }
    
    if (!nrziDecoder.begin()) {
        Serial.println(F("  NRZI Decoder initialization failed!"));
        while (true) { delay(100); }
    }
    
    if (!ax25Parser.begin()) {
        Serial.println(F("  AX.25 Parser initialization failed!"));
        while (true) { delay(100); }
    }
    
    if (!aprsDecoder.begin()) {
        Serial.println(F("  APRS Decoder initialization failed!"));
        while (true) { delay(100); }
    }
    
    Serial.println(F("  All decoders initialized successfully!"));
    Serial.println();
    
    // 启动直接模式接收
    radio.receiveDirect();
    
    Serial.println(F("[Status] Listening for APRS packets..."));
    Serial.println(F("========================================"));
    Serial.println();
    
    lastStatsTime = millis();
}

// ============================================================================
// 主循环
// ============================================================================

void loop() {
    unsigned long loopStart = micros();
    
    // ========================================================================
    // 1. 处理来自SX1278的采样数据
    // ========================================================================
    
    // 检查是否有足够的采样数据可用
    if (radio.available() > SAMPLES_PER_BIT) {
        // 暂停接收以读取数据
        radio.standby();
        
        // 读取所有可用的采样数据
        while (radio.available()) {
            uint8_t sample = radio.read();
            
            // 将采样数据逐字节处理
            // 每个字节包含8个采样点
            for (int8_t bit = 7; bit >= 0; bit--) {
                uint8_t sampleBit = (sample >> bit) & 0x01;
                
                // 送入AFSK解调器
                afskDemod.processSample(sampleBit);
            }
        }
        
        // 恢复接收
        radio.receiveDirect();
    }
    
    // ========================================================================
    // 2. 从AFSK解调器读取解调后的比特
    // ========================================================================
    
    while (afskDemod.available()) {
        uint8_t bit = afskDemod.readBit();
        
        // 送入NRZI解码器
        nrziDecoder.processBit(bit);
    }
    
    // ========================================================================
    // 3. 从NRZI解码器读取字节并进行帧解析
    // ========================================================================
    
    // 检查帧开始
    if (nrziDecoder.isFrameStart()) {
        ax25Parser.startFrame();
        nrziDecoder.clearFrameFlags();
        
        DEBUG_DEBUG("AX.25 frame start");
    }
    
    // 读取字节并添加到帧解析器
    while (nrziDecoder.available()) {
        uint8_t byte = nrziDecoder.readByte();
        ax25Parser.addByte(byte);
    }
    
    // 检查帧结束
    if (nrziDecoder.isFrameEnd()) {
        bool frameValid = ax25Parser.endFrame();
        nrziDecoder.clearFrameFlags();
        
        if (frameValid) {
            DEBUG_INFO("Valid AX.25 frame received");
        } else {
            DEBUG_ERROR("Invalid AX.25 frame");
        }
    }
    
    // ========================================================================
    // 4. 从AX.25解析器读取帧并解码APRS消息
    // ========================================================================
    
    if (ax25Parser.available()) {
        APRS_AX25Frame frame;
        
        if (ax25Parser.getFrame(&frame)) {
            APRSMessage message;
            
            if (aprsDecoder.decodeFrame(&frame, &message)) {
                // 成功解码APRS消息
                
                // 格式化为TNC2格式
                char tnc2Buffer[512];
                message.toTNC2(tnc2Buffer, sizeof(tnc2Buffer));
                
                // 输出到UART
                UARTOutput.println(tnc2Buffer);
                
                // 同时输出到串口监视器
                Serial.println();
                Serial.println(F("========== APRS MESSAGE =========="));
                Serial.print(F("From: "));
                Serial.println(message.source);
                Serial.print(F("To: "));
                Serial.println(message.destination);
                
                if (message.path[0] != '\0') {
                    Serial.print(F("Path: "));
                    Serial.println(message.path);
                }
                
                Serial.print(F("Type: "));
                Serial.println((int)message.type);
                
                if (message.position.valid) {
                    Serial.print(F("Position: "));
                    Serial.print(message.position.latitude, 6);
                    Serial.print(F(", "));
                    Serial.println(message.position.longitude, 6);
                }
                
                if (message.comment[0] != '\0') {
                    Serial.print(F("Comment: "));
                    Serial.println(message.comment);
                }
                
                Serial.println(F("TNC2: "));
                Serial.println(tnc2Buffer);
                Serial.println(F("=================================="));
                Serial.println();
            }
        }
    }
    
    // ========================================================================
    // 5. 性能统计和监控
    // ========================================================================
    
    unsigned long currentTime = millis();
    
    if (ENABLE_PERFORMANCE_STATS && (currentTime - lastStatsTime >= STATS_REPORT_INTERVAL)) {
        printStatistics();
        lastStatsTime = currentTime;
    }
    
    // 计算循环时间
    unsigned long loopTime = micros() - loopStart;
    lastProcessTime = loopTime;
    
    // 短暂延迟以避免过度占用CPU
    // ESP32C3足够快，可以不需要延迟
    // delay(1);
}

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * 打印性能统计信息
 */
void printStatistics() {
    Serial.println();
    Serial.println(F("========== STATISTICS =========="));
    
    // AFSK解调器统计
    uint32_t totalBits, errorBits;
    float errorRate;
    afskDemod.getStats(&totalBits, &errorBits, &errorRate);
    
    bool pllLocked;
    uint8_t signalQuality;
    afskDemod.getStatus(&pllLocked, &signalQuality);
    
    Serial.println(F("[AFSK Demodulator]"));
    Serial.print(F("  Total bits: "));
    Serial.println(totalBits);
    Serial.print(F("  Error bits: "));
    Serial.println(errorBits);
    Serial.print(F("  Error rate: "));
    Serial.print(errorRate * 100.0, 2);
    Serial.println(F("%"));
    Serial.print(F("  PLL locked: "));
    Serial.println(pllLocked ? F("YES") : F("NO"));
    Serial.print(F("  Signal quality: "));
    Serial.print(signalQuality);
    Serial.println(F("%"));
    
    // NRZI解码器统计
    uint32_t nrziTotalBits, stuffedBits, frameCount;
    nrziDecoder.getStats(&nrziTotalBits, &stuffedBits, &frameCount);
    
    Serial.println(F("[NRZI Decoder]"));
    Serial.print(F("  Total bits: "));
    Serial.println(nrziTotalBits);
    Serial.print(F("  Stuffed bits: "));
    Serial.println(stuffedBits);
    Serial.print(F("  Frame count: "));
    Serial.println(frameCount);
    
    // AX.25解析器统计
    uint32_t totalFrames, validFrames, fcsErrors;
    ax25Parser.getStats(&totalFrames, &validFrames, &fcsErrors);
    
    Serial.println(F("[AX.25 Parser]"));
    Serial.print(F("  Total frames: "));
    Serial.println(totalFrames);
    Serial.print(F("  Valid frames: "));
    Serial.println(validFrames);
    Serial.print(F("  FCS errors: "));
    Serial.println(fcsErrors);
    
    if (totalFrames > 0) {
        float successRate = (float)validFrames / (float)totalFrames * 100.0;
        Serial.print(F("  Success rate: "));
        Serial.print(successRate, 2);
        Serial.println(F("%"));
    }
    
    // APRS解码器统计
    uint32_t totalMessages, positionReports, parseErrors;
    aprsDecoder.getStats(&totalMessages, &positionReports, &parseErrors);
    
    Serial.println(F("[APRS Decoder]"));
    Serial.print(F("  Total messages: "));
    Serial.println(totalMessages);
    Serial.print(F("  Position reports: "));
    Serial.println(positionReports);
    Serial.print(F("  Parse errors: "));
    Serial.println(parseErrors);
    
    // 系统资源
    Serial.println(F("[System]"));
    Serial.print(F("  Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    Serial.print(F("  Loop time: "));
    Serial.print(lastProcessTime);
    Serial.println(F(" us"));
    Serial.print(F("  Uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" seconds"));
    
    Serial.println(F("================================"));
    Serial.println();
}

