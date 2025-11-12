/**
 * AFSK解调器
 * 
 * 实现Bell 202标准的AFSK解调
 * 支持基础相关器和Goertzel算法
 */

#ifndef AFSK_DEMOD_H
#define AFSK_DEMOD_H

#include "aprs_config.h"
#include <stdint.h>

class AFSKDemodulator {
public:
  AFSKDemodulator();
  
  /**
   * 初始化解调器
   */
  virtual bool begin();
  
  /**
   * 处理单个采样点
   * @param sample 采样值 (0或1)
   * @return 如果解调出一个完整比特，返回true
   */
  virtual bool processSample(uint8_t sample);
  
  /**
   * 获取解调后的比特
   * @return 解调后的比特值 (0或1)
   */
  uint8_t getDemodulatedBit();
  
  /**
   * 重置解调器状态
   */
  virtual void reset();
  
  /**
   * 获取信号质量指标
   * @return 信号质量 (0-100)
   */
  uint8_t getSignalQuality();
  
  /**
   * 是否检测到载波
   */
  bool isCarrierDetected();

protected:
  // Goertzel滤波器系数
  float markCoeff;
  float spaceCoeff;
  
  // Goertzel滤波器状态
  float markQ1, markQ2;
  float spaceQ1, spaceQ2;
  
  // 采样计数器
  uint8_t sampleCounter;
  
  // 比特判决
  uint8_t currentBit;
  bool bitReady;
  
  // PLL状态（用于比特同步）
  int16_t pllPhase;
  int16_t pllDPhase;
  
  // 信号检测
  uint16_t markEnergy;
  uint16_t spaceEnergy;
  uint16_t totalEnergy;
  
  // 载波检测
  bool carrierDetected;
  uint8_t carrierLockCount;
  
  /**
   * 计算Goertzel系数
   */
  void calculateCoefficients();
  
  /**
   * Goertzel算法核心
   */
  void goertzelUpdate(float sample, float coeff, float &q1, float &q2);
  
  /**
   * 获取Goertzel能量
   */
  float goertzelMagnitude(float q1, float q2, float coeff);
  
  /**
   * PLL位同步
   */
  void pllUpdate(bool transition);
};

#endif // AFSK_DEMOD_H

