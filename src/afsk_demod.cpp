/**
 * AFSK解调器 - 实现文件
 */

#include "afsk_demod.h"

AFSKDemodulator::AFSKDemodulator() {
    reset();
}

bool AFSKDemodulator::begin() {
    reset();
    DEBUG_INFO("AFSK Demodulator initialized");
    return true;
}

void AFSKDemodulator::reset() {
    // 清空缓冲区
    memset(sampleBuffer, 0, sizeof(sampleBuffer));
    memset(bitBuffer, 0, sizeof(bitBuffer));
    
    bufferIndex = 0;
    sampleCount = 0;
    bitBufferHead = 0;
    bitBufferTail = 0;
    
    // 重置PLL
    pllPhase = 0;
    pllOffset = 0;
    pllLocked = false;
    pllLockCounter = 0;
    
    // 重置解调状态
    lastBit = 0;
    transitionCounter = 0;
    markEnergy = 0;
    spaceEnergy = 0;
    signalQuality = 0;
    
    // 重置统计
    totalBits = 0;
    errorBits = 0;
}

void AFSKDemodulator::processSample(uint8_t sample) {
    // 将采样添加到环形缓冲区
    sampleBuffer[bufferIndex] = sample & 0x01;  // 确保只取最低位
    bufferIndex = (bufferIndex + 1) % DEMOD_WINDOW_SIZE;
    
    if (sampleCount < DEMOD_WINDOW_SIZE) {
        sampleCount++;
        return;  // 等待缓冲区填充
    }
    
    // 更新PLL相位
    pllPhase++;
    
    // 当PLL相位到达采样点时，进行比特判决
    if (pllPhase >= SAMPLES_PER_BIT) {
        pllPhase = 0;
        
        // 计算Mark和Space频率的能量
        calculateEnergyFast();
        
        // 执行比特判决
        uint8_t bit = decideBit();
        
        // 更新PLL
        updatePLL(bit);
        
        // 将比特写入输出缓冲区
        writeBitToBuffer(bit);
        
        // 更新信号质量
        updateSignalQuality();
        
        // 统计
        totalBits++;
        
        lastBit = bit;
    }
}

void AFSKDemodulator::processSamples(const uint8_t* samples, size_t length) {
    for (size_t i = 0; i < length; i++) {
        processSample(samples[i]);
    }
}

void AFSKDemodulator::calculateEnergyFast() {
    // 使用快速相关算法
    // 基本思想：检测采样缓冲区中特定周期模式的出现频率
    
    markEnergy = 0;
    spaceEnergy = 0;
    
    // 对于1200Hz (Mark)：22个采样一个周期
    // 对于2200Hz (Space)：12个采样一个周期
    
    // 使用滑动窗口相关
    // Mark相关：检测周期为22的模式
    uint8_t markCount = 0;
    uint8_t spaceCount = 0;
    
    // 检查最近的采样窗口
    uint8_t windowSize = SAMPLES_PER_BIT;
    
    // 简化的能量检测：统计跳变频率
    // 1200Hz应该有较少的跳变，2200Hz应该有较多的跳变
    uint8_t transitions = 0;
    uint8_t prevSample = sampleBuffer[(bufferIndex - windowSize + DEMOD_WINDOW_SIZE) % DEMOD_WINDOW_SIZE];
    
    for (uint8_t i = 1; i < windowSize; i++) {
        uint8_t idx = (bufferIndex - windowSize + i + DEMOD_WINDOW_SIZE) % DEMOD_WINDOW_SIZE;
        uint8_t currSample = sampleBuffer[idx];
        
        if (currSample != prevSample) {
            transitions++;
        }
        prevSample = currSample;
    }
    
    // 根据跳变次数估计频率
    // 2200Hz: 大约 2200/26400 * 22 ≈ 1.83个周期 ≈ 3-4次跳变
    // 1200Hz: 大约 1200/26400 * 22 = 1个周期 ≈ 2次跳变
    
    // 更精确的方法：使用相关器
    markEnergy = correlate(MARK_PERIOD);
    spaceEnergy = correlate(SPACE_PERIOD);
}

uint16_t AFSKDemodulator::correlate(uint8_t period) {
    uint16_t energy = 0;
    uint8_t matches = 0;
    
    // 生成本地振荡器模式 (方波)
    // 对于给定周期，期望看到周期性的0和1交替
    
    uint8_t windowSize = SAMPLES_PER_BIT;
    uint8_t halfPeriod = period / 2;
    
    for (uint8_t i = 0; i < windowSize; i++) {
        uint8_t idx = (bufferIndex - windowSize + i + DEMOD_WINDOW_SIZE) % DEMOD_WINDOW_SIZE;
        uint8_t sample = sampleBuffer[idx];
        
        // 生成本地振荡器值
        uint8_t localOsc = ((i / halfPeriod) & 1);
        
        // 相关：如果匹配则增加能量
        if (sample == localOsc) {
            matches++;
        }
    }
    
    energy = matches * matches;  // 平方以增强差异
    return energy;
}

uint8_t AFSKDemodulator::decideBit() {
    // 基于能量比较进行比特判决
    // Mark (1200Hz) = 逻辑1
    // Space (2200Hz) = 逻辑0
    
    if (markEnergy > spaceEnergy * ENERGY_THRESHOLD_RATIO) {
        return 1;
    } else {
        return 0;
    }
}

void AFSKDemodulator::updatePLL(uint8_t currentBit) {
    // 检测比特边沿
    if (currentBit != lastBit) {
        transitionCounter++;
        
        // 边沿应该发生在相位为0附近
        // 如果相位偏离较大，则调整PLL
        
        if (pllPhase < SAMPLES_PER_BIT / 4) {
            // 边沿来得太早，减慢时钟
            pllOffset--;
        } else if (pllPhase > SAMPLES_PER_BIT * 3 / 4) {
            // 边沿来得太晚，加快时钟
            pllOffset++;
        } else {
            // 边沿时机正确，PLL工作良好
            if (pllLockCounter < PLL_LOCKED_THRESHOLD) {
                pllLockCounter++;
            }
        }
        
        // 应用偏移调整
        if (pllOffset > 2) {
            pllPhase = (pllPhase + 1) % SAMPLES_PER_BIT;
            pllOffset = 0;
        } else if (pllOffset < -2) {
            pllPhase = (pllPhase - 1 + SAMPLES_PER_BIT) % SAMPLES_PER_BIT;
            pllOffset = 0;
        }
    } else {
        // 无边沿，减少锁定计数
        if (pllLockCounter > 0) {
            pllLockCounter--;
        }
    }
    
    // 更新锁定状态
    pllLocked = (pllLockCounter >= PLL_LOCKED_THRESHOLD / 2);
}

bool AFSKDemodulator::detectTransition() {
    // 检查最近的采样中是否有边沿
    uint8_t prev = sampleBuffer[(bufferIndex - 2 + DEMOD_WINDOW_SIZE) % DEMOD_WINDOW_SIZE];
    uint8_t curr = sampleBuffer[(bufferIndex - 1 + DEMOD_WINDOW_SIZE) % DEMOD_WINDOW_SIZE];
    
    return (prev != curr);
}

void AFSKDemodulator::writeBitToBuffer(uint8_t bit) {
    bitBuffer[bitBufferHead] = bit;
    bitBufferHead = (bitBufferHead + 1) % 256;
    
    // 检查缓冲区溢出
    if (bitBufferHead == bitBufferTail) {
        // 缓冲区满，丢弃最旧的数据
        bitBufferTail = (bitBufferTail + 1) % 256;
        errorBits++;
    }
}

void AFSKDemodulator::updateSignalQuality() {
    // 基于能量差异和PLL锁定状态计算信号质量
    uint16_t energyDiff = (markEnergy > spaceEnergy) ? 
                          (markEnergy - spaceEnergy) : 
                          (spaceEnergy - markEnergy);
    
    uint16_t totalEnergy = markEnergy + spaceEnergy;
    
    if (totalEnergy > 0) {
        // 能量差异越大，信号质量越好
        uint8_t energyQuality = (energyDiff * 100) / totalEnergy;
        
        // PLL锁定状态也影响质量
        uint8_t pllQuality = pllLocked ? 100 : (pllLockCounter * 100 / PLL_LOCKED_THRESHOLD);
        
        // 综合计算
        signalQuality = (energyQuality * 7 + pllQuality * 3) / 10;
    } else {
        signalQuality = 0;
    }
    
    // 限制范围
    if (signalQuality > 100) {
        signalQuality = 100;
    }
}

bool AFSKDemodulator::available() {
    return (bitBufferHead != bitBufferTail);
}

uint8_t AFSKDemodulator::readBit() {
    if (!available()) {
        return 0;
    }
    
    uint8_t bit = bitBuffer[bitBufferTail];
    bitBufferTail = (bitBufferTail + 1) % 256;
    
    return bit;
}

void AFSKDemodulator::getStatus(bool* pllLocked_out, uint8_t* signalQuality_out) {
    if (pllLocked_out != nullptr) {
        *pllLocked_out = pllLocked;
    }
    
    if (signalQuality_out != nullptr) {
        *signalQuality_out = signalQuality;
    }
}

void AFSKDemodulator::getStats(uint32_t* totalBits_out, uint32_t* errorBits_out, float* errorRate_out) {
    if (totalBits_out != nullptr) {
        *totalBits_out = totalBits;
    }
    
    if (errorBits_out != nullptr) {
        *errorBits_out = errorBits;
    }
    
    if (errorRate_out != nullptr) {
        if (totalBits > 0) {
            *errorRate_out = (float)errorBits / (float)totalBits;
        } else {
            *errorRate_out = 0.0f;
        }
    }
}

uint32_t AFSKDemodulator::buildSampleWindow(uint8_t offset) {
    uint32_t window = 0;
    
    for (uint8_t i = 0; i < 32; i++) {
        uint8_t idx = (bufferIndex - offset - i + DEMOD_WINDOW_SIZE) % DEMOD_WINDOW_SIZE;
        if (sampleBuffer[idx]) {
            window |= (1UL << (31 - i));
        }
    }
    
    return window;
}

