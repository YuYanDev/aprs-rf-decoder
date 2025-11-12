/**
 * AFSK解调器实现
 */

#include "afsk_demod.h"
#include <math.h>

// 数学常量
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AFSKDemodulator::AFSKDemodulator() {
  reset();
}

bool AFSKDemodulator::begin() {
  calculateCoefficients();
  reset();
  return true;
}

void AFSKDemodulator::calculateCoefficients() {
  // Goertzel算法系数计算
  // coeff = 2 * cos(2π * freq / sampleRate)
  float omega_mark = (2.0f * M_PI * AFSK_MARK_FREQ) / AFSK_SAMPLE_RATE;
  float omega_space = (2.0f * M_PI * AFSK_SPACE_FREQ) / AFSK_SAMPLE_RATE;
  
  markCoeff = 2.0f * cos(omega_mark);
  spaceCoeff = 2.0f * cos(omega_space);
}

void AFSKDemodulator::reset() {
  markQ1 = markQ2 = 0;
  spaceQ1 = spaceQ2 = 0;
  sampleCounter = 0;
  currentBit = 0;
  bitReady = false;
  pllPhase = 0;
  pllDPhase = 0x10000 / SAMPLES_PER_BIT;  // 固定点数表示
  markEnergy = 0;
  spaceEnergy = 0;
  totalEnergy = 0;
  carrierDetected = false;
  carrierLockCount = 0;
}

void AFSKDemodulator::goertzelUpdate(float sample, float coeff, float &q1, float &q2) {
  float q0 = coeff * q1 - q2 + sample;
  q2 = q1;
  q1 = q0;
}

float AFSKDemodulator::goertzelMagnitude(float q1, float q2, float coeff) {
  // magnitude^2 = q1^2 + q2^2 - q1*q2*coeff
  float real = q1 - q2 * coeff / 2.0f;
  float imag = q2 * sin(acos(coeff / 2.0f));
  return real * real + imag * imag;
}

bool AFSKDemodulator::processSample(uint8_t sample) {
  // 将样本转换为浮点数 (-1 或 +1)
  float fsample = (sample == 0) ? -1.0f : 1.0f;
  
  // 更新Goertzel滤波器
  goertzelUpdate(fsample, markCoeff, markQ1, markQ2);
  goertzelUpdate(fsample, spaceCoeff, spaceQ1, spaceQ2);
  
  // 更新PLL相位
  pllPhase += pllDPhase;
  
  // 采样计数
  sampleCounter++;
  
  // 比特判决时刻
  if (pllPhase >= 0x10000) {
    pllPhase -= 0x10000;
    
    // 计算Mark和Space能量
    float markMag = goertzelMagnitude(markQ1, markQ2, markCoeff);
    float spaceMag = goertzelMagnitude(spaceQ1, spaceQ2, spaceCoeff);
    
    // 判决：Mark能量大于Space能量 -> 比特1，否则 -> 比特0
    uint8_t newBit = (markMag > spaceMag) ? 1 : 0;
    
    // 检测跳变（用于PLL调整）
    bool transition = (newBit != currentBit);
    pllUpdate(transition);
    
    currentBit = newBit;
    bitReady = true;
    
    // 更新能量统计
    markEnergy = (uint16_t)markMag;
    spaceEnergy = (uint16_t)spaceMag;
    totalEnergy = markEnergy + spaceEnergy;
    
    // 载波检测
    if (totalEnergy > CARRIER_DETECT_THR) {
      if (carrierLockCount < 255) carrierLockCount++;
      if (carrierLockCount > 5) {
        carrierDetected = true;
      }
    } else {
      if (carrierLockCount > 0) carrierLockCount--;
      if (carrierLockCount == 0) {
        carrierDetected = false;
      }
    }
    
    // 重置Goertzel状态（每比特周期）
    markQ1 = markQ2 = 0;
    spaceQ1 = spaceQ2 = 0;
    sampleCounter = 0;
    
    return true;
  }
  
  return false;
}

void AFSKDemodulator::pllUpdate(bool transition) {
  // 简单的PLL：在检测到跳变时调整相位
  if (transition) {
    // 如果在比特中间检测到跳变，说明采样时钟偏移
    // 进行相位调整
    if (pllPhase < 0x8000) {
      // 相位超前，减慢时钟
      pllDPhase -= 1;
    } else {
      // 相位滞后，加快时钟
      pllDPhase += 1;
    }
    
    // 限制PLL频率调整范围
    int16_t nominal = 0x10000 / SAMPLES_PER_BIT;
    if (pllDPhase < nominal - 100) pllDPhase = nominal - 100;
    if (pllDPhase > nominal + 100) pllDPhase = nominal + 100;
  }
}

uint8_t AFSKDemodulator::getDemodulatedBit() {
  bitReady = false;
  return currentBit;
}

uint8_t AFSKDemodulator::getSignalQuality() {
  if (totalEnergy == 0) return 0;
  
  // 信号质量基于Mark和Space能量差异
  uint16_t diff = abs(markEnergy - spaceEnergy);
  uint8_t quality = (diff * 100) / (totalEnergy + 1);
  
  // 限制在0-100范围
  if (quality > 100) quality = 100;
  
  return quality;
}

bool AFSKDemodulator::isCarrierDetected() {
  return carrierDetected;
}

