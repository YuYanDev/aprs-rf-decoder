/**
 * APRS增强解码器（使用CMSIS-DSP）
 * 
 * 针对带FPU和DSP的STM32（L4, F4, G4系列）优化
 * 使用CMSIS-DSP库加速信号处理
 */

#ifndef APRS_DECODER_ENHANCED_H
#define APRS_DECODER_ENHANCED_H

#include "aprs_config.h"

#if USE_CMSIS_DSP

#include "aprs_decoder.h"

// 需要定义ARM_MATH_CM4才能使用CMSIS-DSP
#define ARM_MATH_CM4
#include "arm_math.h"

/**
 * 增强型AFSK解调器
 * 使用CMSIS-DSP优化的Goertzel算法和FIR滤波
 */
class AFSKDemodulatorEnhanced : public AFSKDemodulator {
public:
  AFSKDemodulatorEnhanced();
  
  /**
   * 初始化增强解调器
   */
  bool begin() override;
  
  /**
   * 处理采样（使用DSP优化）
   */
  bool processSample(uint8_t sample) override;
  
  /**
   * 重置
   */
  void reset() override;

protected:
  // FIR带通滤波器状态
  arm_fir_instance_f32 firMark;
  arm_fir_instance_f32 firSpace;
  float32_t firMarkState[64];
  float32_t firSpaceState[64];
  float32_t firMarkCoeffs[32];
  float32_t firSpaceCoeffs[32];
  
  // Goertzel算法使用的CMSIS-DSP优化版本
  float32_t markBuffer[SAMPLES_PER_BIT];
  float32_t spaceBuffer[SAMPLES_PER_BIT];
  uint8_t bufferIndex;
  
  /**
   * 使用CMSIS-DSP计算Goertzel
   */
  float32_t goertzelCMSIS(float32_t* buffer, uint16_t freq);
  
  /**
   * 初始化FIR滤波器
   */
  void initFIRFilters();
  
  /**
   * 设计带通滤波器系数
   */
  void designBandpassFilter(float32_t* coeffs, uint16_t numTaps, 
                           float32_t centerFreq, float32_t bandwidth);
};

/**
 * 增强型APRS解码器
 * 整合优化的DSP算法
 */
class APRSDecoderEnhanced : public APRSDecoder {
public:
  APRSDecoderEnhanced();
  
  /**
   * 初始化增强解码器
   */
  bool begin() override;
  
  /**
   * 批量处理采样（使用DMA）
   * @param samples 采样缓冲区
   * @param length 采样数量
   */
  void processSampleBatch(uint8_t* samples, uint16_t length);
  
  /**
   * 使用自适应均衡器
   */
  void enableAdaptiveEqualizer(bool enable);
  
  /**
   * 获取FFT频谱（用于调试）
   * @param spectrum 输出频谱数组
   * @param size 数组大小（必须是2的幂）
   */
  void getSpectrum(float32_t* spectrum, uint16_t size);

protected:
  AFSKDemodulatorEnhanced afskDemodEnhanced;
  
  // 自适应均衡器
  bool useEqualizer;
  arm_fir_instance_f32 equalizer;
  float32_t equalizerState[128];
  float32_t equalizerCoeffs[64];
  
  // FFT用于频谱分析
  arm_rfft_fast_instance_f32 fftInstance;
  float32_t fftBuffer[256];
  
  /**
   * 自适应均衡器更新
   */
  void updateEqualizer(float32_t error);
};

#endif // USE_CMSIS_DSP

#endif // APRS_DECODER_ENHANCED_H

