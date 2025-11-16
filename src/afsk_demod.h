/**
 * AFSK解调器 - 头文件
 * 
 * 基于数字相关器和能量检测的AFSK解调实现
 * 针对Bell 202标准 (1200 baud, 1200/2200 Hz)
 */

#ifndef AFSK_DEMOD_H
#define AFSK_DEMOD_H

#include <Arduino.h>
#include "aprs_config.h"

/**
 * AFSK解调器类
 * 
 * 功能：
 * 1. 从26.4kHz采样流中提取1200/2200Hz音频信号
 * 2. 使用相关器检测Mark(1200Hz)和Space(2200Hz)频率
 * 3. 实现数字PLL进行时钟恢复和比特判决
 * 4. 输出解调后的比特流
 */
class AFSKDemodulator {
public:
    /**
     * 构造函数
     */
    AFSKDemodulator();

    /**
     * 初始化解调器
     * @return true: 成功, false: 失败
     */
    bool begin();

    /**
     * 处理新的采样数据
     * @param sample: 输入采样值 (0或1)
     */
    void processSample(uint8_t sample);

    /**
     * 批量处理采样数据 (高效版本)
     * @param samples: 采样数据缓冲区
     * @param length: 数据长度
     */
    void processSamples(const uint8_t* samples, size_t length);

    /**
     * 检查是否有新的比特可用
     * @return true: 有新比特, false: 无新比特
     */
    bool available();

    /**
     * 读取一个解调后的比特
     * @return 比特值 (0或1)
     */
    uint8_t readBit();

    /**
     * 获取解调器状态
     * @param pllLocked: 输出PLL锁定状态
     * @param signalQuality: 输出信号质量 (0-100)
     */
    void getStatus(bool* pllLocked, uint8_t* signalQuality);

    /**
     * 重置解调器状态
     */
    void reset();

    /**
     * 获取性能统计信息
     */
    void getStats(uint32_t* totalBits, uint32_t* errorBits, float* errorRate);

private:
    // 采样缓冲区 (环形缓冲)
    uint8_t sampleBuffer[DEMOD_WINDOW_SIZE];
    uint8_t bufferIndex;
    uint8_t sampleCount;

    // 比特输出缓冲区
    uint8_t bitBuffer[256];
    uint8_t bitBufferHead;
    uint8_t bitBufferTail;

    // PLL状态
    uint8_t pllPhase;           // 当前PLL相位 (0 ~ SAMPLES_PER_BIT-1)
    int8_t pllOffset;           // 相位偏移累积器
    bool pllLocked;             // PLL锁定状态
    uint8_t pllLockCounter;     // PLL锁定计数器

    // 解调状态
    uint8_t lastBit;            // 上一个解调比特
    uint8_t transitionCounter;  // 边沿计数器 (用于PLL调整)

    // 能量检测
    uint16_t markEnergy;        // Mark频率能量
    uint16_t spaceEnergy;       // Space频率能量
    uint8_t signalQuality;      // 信号质量指标 (0-100)

    // 性能统计
    uint32_t totalBits;
    uint32_t errorBits;

    /**
     * 使用相关器计算Mark和Space频率的能量
     * 基于滑动窗口和本地振荡器相关
     */
    void calculateEnergy();

    /**
     * 相关器辅助函数 - 计算与指定频率的相关性
     * @param period: 目标频率的周期 (采样数)
     * @return 相关性强度
     */
    uint16_t correlate(uint8_t period);

    /**
     * 执行比特判决
     * @return 判决结果 (0或1)
     */
    uint8_t decideBit();

    /**
     * 更新PLL状态
     * @param currentBit: 当前判决的比特
     */
    void updatePLL(uint8_t currentBit);

    /**
     * 检测边沿转换 (用于PLL同步)
     * @return true: 检测到边沿, false: 无边沿
     */
    bool detectTransition();

    /**
     * 将比特写入输出缓冲区
     * @param bit: 要写入的比特
     */
    void writeBitToBuffer(uint8_t bit);

    /**
     * 计算信号质量
     */
    void updateSignalQuality();

    /**
     * 使用快速算法计算能量 (优化版)
     * 使用XOR相关和popcount来加速
     */
    void calculateEnergyFast();

    /**
     * 计算popcount (位为1的个数)
     * @param val: 输入值
     * @return 位为1的个数
     */
    inline uint8_t popcount(uint32_t val) {
        uint8_t count = 0;
        while (val) {
            count += val & 1;
            val >>= 1;
        }
        return count;
    }

    /**
     * 构建32位采样窗口 (用于快速相关)
     * @param offset: 相对当前位置的偏移
     * @return 32位采样窗口
     */
    uint32_t buildSampleWindow(uint8_t offset);
};

#endif // AFSK_DEMOD_H

