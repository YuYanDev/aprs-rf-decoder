/**
 * APRS增强解码器实现（CMSIS-DSP优化）
 */

#include "aprs_decoder_enhanced.h"

#if USE_CMSIS_DSP

#include <math.h>

// ============================================================================
// AFSKDemodulatorEnhanced 实现
// ============================================================================

AFSKDemodulatorEnhanced::AFSKDemodulatorEnhanced() : AFSKDemodulator() {
  bufferIndex = 0;
}

bool AFSKDemodulatorEnhanced::begin() {
  // 调用基类初始化
  AFSKDemodulator::begin();
  
  // 初始化FIR滤波器
  initFIRFilters();
  
  return true;
}

void AFSKDemodulatorEnhanced::reset() {
  AFSKDemodulator::reset();
  bufferIndex = 0;
  
  // 清空滤波器状态
  memset(firMarkState, 0, sizeof(firMarkState));
  memset(firSpaceState, 0, sizeof(firSpaceState));
}

void AFSKDemodulatorEnhanced::initFIRFilters() {
  // 设计Mark频率带通滤波器 (2200Hz ± 200Hz)
  designBandpassFilter(firMarkCoeffs, 32, AFSK_MARK_FREQ, 400);
  
  // 设计Space频率带通滤波器 (1200Hz ± 200Hz)
  designBandpassFilter(firSpaceCoeffs, 32, AFSK_SPACE_FREQ, 400);
  
  // 初始化FIR滤波器实例
  arm_fir_init_f32(&firMark, 32, firMarkCoeffs, firMarkState, 1);
  arm_fir_init_f32(&firSpace, 32, firSpaceCoeffs, firSpaceState, 1);
}

void AFSKDemodulatorEnhanced::designBandpassFilter(float32_t* coeffs, 
                                                   uint16_t numTaps, 
                                                   float32_t centerFreq, 
                                                   float32_t bandwidth) {
  // 简化的带通滤波器设计（汉明窗）
  float32_t fc1 = (centerFreq - bandwidth/2.0f) / AFSK_SAMPLE_RATE;
  float32_t fc2 = (centerFreq + bandwidth/2.0f) / AFSK_SAMPLE_RATE;
  
  for (uint16_t i = 0; i < numTaps; i++) {
    float32_t n = (float32_t)i - (numTaps - 1) / 2.0f;
    
    // 理想带通滤波器
    float32_t h;
    if (n == 0) {
      h = 2.0f * (fc2 - fc1);
    } else {
      h = (sin(2.0f * PI * fc2 * n) - sin(2.0f * PI * fc1 * n)) / (PI * n);
    }
    
    // 汉明窗
    float32_t window = 0.54f - 0.46f * cos(2.0f * PI * i / (numTaps - 1));
    
    coeffs[i] = h * window;
  }
  
  // 归一化
  float32_t sum = 0;
  for (uint16_t i = 0; i < numTaps; i++) {
    sum += coeffs[i];
  }
  if (sum != 0) {
    for (uint16_t i = 0; i < numTaps; i++) {
      coeffs[i] /= sum;
    }
  }
}

float32_t AFSKDemodulatorEnhanced::goertzelCMSIS(float32_t* buffer, uint16_t freq) {
  // 使用CMSIS-DSP优化的Goertzel算法
  float32_t omega = (2.0f * PI * freq) / AFSK_SAMPLE_RATE;
  float32_t coeff = 2.0f * cos(omega);
  
  float32_t q0, q1 = 0, q2 = 0;
  
  // 使用CMSIS-DSP的MAC（乘加）指令
  for (uint8_t i = 0; i < SAMPLES_PER_BIT; i++) {
    q0 = coeff * q1 - q2 + buffer[i];
    q2 = q1;
    q1 = q0;
  }
  
  // 计算幅度
  float32_t real = q1 - q2 * coeff / 2.0f;
  float32_t imag = q2 * sin(omega);
  
  float32_t magnitude;
  arm_sqrt_f32(real * real + imag * imag, &magnitude);
  
  return magnitude;
}

bool AFSKDemodulatorEnhanced::processSample(uint8_t sample) {
  // 转换为浮点数
  float32_t fsample = (sample == 0) ? -1.0f : 1.0f;
  
  // 应用FIR滤波器
  float32_t markFiltered, spaceFiltered;
  arm_fir_f32(&firMark, &fsample, &markFiltered, 1);
  arm_fir_f32(&firSpace, &fsample, &spaceFiltered, 1);
  
  // 存入缓冲区用于Goertzel
  markBuffer[bufferIndex] = markFiltered;
  spaceBuffer[bufferIndex] = spaceFiltered;
  bufferIndex++;
  
  // 更新PLL相位
  pllPhase += pllDPhase;
  
  // 比特判决时刻
  if (pllPhase >= 0x10000) {
    pllPhase -= 0x10000;
    
    // 使用CMSIS-DSP优化的Goertzel算法
    float32_t markMag = goertzelCMSIS(markBuffer, AFSK_MARK_FREQ);
    float32_t spaceMag = goertzelCMSIS(spaceBuffer, AFSK_SPACE_FREQ);
    
    // 判决
    uint8_t newBit = (markMag > spaceMag) ? 1 : 0;
    
    // 检测跳变
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
    
    // 重置缓冲区
    bufferIndex = 0;
    
    return true;
  }
  
  return false;
}

// ============================================================================
// APRSDecoderEnhanced 实现
// ============================================================================

APRSDecoderEnhanced::APRSDecoderEnhanced() : APRSDecoder() {
  useEqualizer = false;
}

bool APRSDecoderEnhanced::begin() {
  // 使用增强型AFSK解调器
  if (!afskDemodEnhanced.begin()) {
    return false;
  }
  
  nrziDecoder.begin();
  ax25Parser.begin();
  
  // 初始化FFT
  arm_rfft_fast_init_f32(&fftInstance, 256);
  
  reset();
  
  DEBUG_PRINTLN("Enhanced APRS Decoder initialized with CMSIS-DSP");
  
  return true;
}

void APRSDecoderEnhanced::processSampleBatch(uint8_t* samples, uint16_t length) {
  // 批量处理采样（适用于DMA传输）
  for (uint16_t i = 0; i < length; i++) {
    processSample(samples[i]);
  }
}

void APRSDecoderEnhanced::enableAdaptiveEqualizer(bool enable) {
  useEqualizer = enable;
  
  if (enable) {
    // 初始化自适应均衡器系数（全通）
    for (int i = 0; i < 64; i++) {
      equalizerCoeffs[i] = (i == 32) ? 1.0f : 0.0f;
    }
    arm_fir_init_f32(&equalizer, 64, equalizerCoeffs, equalizerState, 1);
    DEBUG_PRINTLN("Adaptive Equalizer Enabled");
  }
}

void APRSDecoderEnhanced::updateEqualizer(float32_t error) {
  // LMS自适应算法
  const float32_t mu = 0.001f;  // 步长
  
  // 更新系数
  for (int i = 0; i < 64; i++) {
    equalizerCoeffs[i] += mu * error * equalizerState[i];
  }
}

void APRSDecoderEnhanced::getSpectrum(float32_t* spectrum, uint16_t size) {
  if (size != 256) {
    return;  // 仅支持256点FFT
  }
  
  // 执行FFT
  arm_rfft_fast_f32(&fftInstance, fftBuffer, spectrum, 0);
  
  // 计算幅度谱
  arm_cmplx_mag_f32(spectrum, spectrum, size / 2);
}

#endif // USE_CMSIS_DSP

