# 技术实现详解

本文档详细说明APRS RF Decoder的技术实现细节、DSP算法和优化策略。

## 目录

1. [AFSK解调算法](#afsk解调算法)
2. [时钟恢复与PLL](#时钟恢复与pll)
3. [NRZI解码原理](#nrzi解码原理)
4. [CRC校验实现](#crc校验实现)
5. [性能优化技巧](#性能优化技巧)
6. [调试方法](#调试方法)

---

## AFSK解调算法

### Bell 202标准

APRS使用Bell 202 AFSK调制：
- **比特率**：1200 baud
- **Mark频率** (逻辑1)：1200 Hz
- **Space频率** (逻辑0)：2200 Hz

### 采样率选择

选择26.4 kHz作为采样率的原因：

```
26400 Hz = 1200 Hz × 22 = 2200 Hz × 12
```

这意味着：
- 1200 Hz信号每个周期有22个采样点
- 2200 Hz信号每个周期有12个采样点
- 每个比特(1200 baud)有22个采样点

这种整数倍关系简化了频率检测算法。

### 相关器原理

相关器通过将接收信号与本地振荡器信号相乘并积分来检测特定频率。

#### 数学表达式

对于频率 f 的检测：

```
相关度 = Σ(接收信号[i] × 本地振荡器[i])
```

其中本地振荡器是频率为 f 的方波。

#### 实现

```cpp
uint16_t AFSKDemodulator::correlate(uint8_t period) {
    uint16_t energy = 0;
    uint8_t matches = 0;
    
    uint8_t windowSize = SAMPLES_PER_BIT;  // 22 samples
    uint8_t halfPeriod = period / 2;
    
    for (uint8_t i = 0; i < windowSize; i++) {
        uint8_t sample = sampleBuffer[i];
        
        // 生成方波本地振荡器
        uint8_t localOsc = ((i / halfPeriod) & 1);
        
        // 相关：匹配则增加
        if (sample == localOsc) {
            matches++;
        }
    }
    
    // 平方增强差异
    energy = matches * matches;
    return energy;
}
```

### 能量检测与判决

通过比较Mark和Space的能量来判决比特：

```cpp
uint8_t decideBit() {
    if (markEnergy > spaceEnergy * ENERGY_THRESHOLD_RATIO) {
        return 1;  // Mark
    } else {
        return 0;  // Space
    }
}
```

**ENERGY_THRESHOLD_RATIO** (默认1.05)是判决阈值，可根据信噪比调整：
- 信号强时：可设置为1.10（更严格）
- 信号弱时：可设置为1.02（更宽松）

---

## 时钟恢复与PLL

### 为什么需要PLL

即使发射端和接收端都使用1200 baud，实际时钟仍存在微小差异：
- 晶振精度误差（几十ppm）
- 温度漂移
- 传播延迟

PLL动态跟踪这些变化，确保在正确的时刻采样。

### 数字PLL实现

#### 基本原理

1. **相位累积**：每个采样点增加相位
2. **边沿检测**：检测比特跳变
3. **相位调整**：根据边沿位置调整相位

#### 代码实现

```cpp
void AFSKDemodulator::updatePLL(uint8_t currentBit) {
    // 检测边沿
    if (currentBit != lastBit) {
        transitionCounter++;
        
        // 边沿应该在相位≈0时发生
        if (pllPhase < SAMPLES_PER_BIT / 4) {
            // 边沿来得太早，减慢时钟
            pllOffset--;
        } else if (pllPhase > SAMPLES_PER_BIT * 3 / 4) {
            // 边沿来得太晚，加快时钟
            pllOffset++;
        } else {
            // 时机正确，增加锁定计数
            pllLockCounter++;
        }
        
        // 应用调整
        if (pllOffset > 2) {
            pllPhase++;
            pllOffset = 0;
        } else if (pllOffset < -2) {
            pllPhase--;
            pllOffset = 0;
        }
    }
}
```

### PLL性能指标

- **锁定时间**：通常在5-10个比特内锁定
- **跟踪范围**：可跟踪±500 ppm的频率偏差
- **抖动容限**：可容忍±2个采样点的抖动

---

## NRZI解码原理

### NRZI编码规则

AX.25使用NRZI (Non-Return-to-Zero Inverted)：

| 原始比特 | 电平变化 |
|---------|---------|
| 0       | 跳变    |
| 1       | 保持    |

### 解码算法

```cpp
uint8_t NRZIDecoder::decodeNRZI(uint8_t nrziBit) {
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
```

### 比特去填充

AX.25使用比特填充防止数据中出现帧标志(0x7E = 0b01111110)：

**规则**：连续5个1后插入一个0

**示例**：
```
原始数据：   11111100
填充后：     111110100
```

**去填充算法**：

```cpp
bool NRZIDecoder::unstuffBit(uint8_t bit) {
    if (bit == 1) {
        onesCount++;
        return true;  // 保留
    } else {
        if (onesCount == 5) {
            // 这是填充比特，丢弃
            onesCount = 0;
            return false;
        }
        onesCount = 0;
        return true;  // 保留
    }
}
```

---

## CRC校验实现

### CRC-16-CCITT参数

AX.25使用CRC-16-CCITT：
- **多项式**：0x1021 (x^16 + x^12 + x^5 + 1)
- **初始值**：0xFFFF
- **最终异或**：0xFFFF
- **比特顺序**：LSB first

### 实现算法

```cpp
uint16_t AX25Parser::calculateFCS(const uint8_t* data, uint16_t length) {
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
    
    return crc ^ 0xFFFF;
}
```

### 优化：查表法

为提升性能，可使用预计算的查找表：

```cpp
// 预计算CRC表
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, ...
};

uint16_t calculateFCS_fast(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        uint8_t index = (crc >> 8) ^ data[i];
        crc = (crc << 8) ^ crc16_table[index];
    }
    
    return crc ^ 0xFFFF;
}
```

---

## 性能优化技巧

### 1. 减少内存拷贝

使用环形缓冲区代替动态内存分配：

```cpp
uint8_t buffer[256];
uint8_t head = 0;
uint8_t tail = 0;

// 写入
buffer[head] = data;
head = (head + 1) % 256;

// 读取
uint8_t data = buffer[tail];
tail = (tail + 1) % 256;
```

### 2. 位运算优化

使用位操作代替乘除法：

```cpp
// 替代：x / 8
x >> 3

// 替代：x % 256
x & 0xFF

// 替代：x * 2
x << 1
```

### 3. 内联函数

对于频繁调用的小函数使用inline：

```cpp
inline uint8_t popcount(uint32_t val) {
    // 快速位计数
    val = val - ((val >> 1) & 0x55555555);
    val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
    return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
```

### 4. 批量处理

批量处理采样数据减少函数调用开销：

```cpp
void processSamples(const uint8_t* samples, size_t length) {
    for (size_t i = 0; i < length; i++) {
        processSample(samples[i]);
    }
}
```

### 5. 避免浮点运算

使用定点数代替浮点数：

```cpp
// 定点数：16.16格式
int32_t fixedLat = (int32_t)(latitude * 65536.0);

// 转换回浮点
float latitude = (float)fixedLat / 65536.0;
```

---

## 调试方法

### 1. 信号质量监控

添加实时信号质量显示：

```cpp
void printSignalQuality() {
    bool pllLocked;
    uint8_t quality;
    afskDemod.getStatus(&pllLocked, &quality);
    
    Serial.printf("PLL: %s | Quality: %d%% | ", 
                  pllLocked ? "LOCK" : "UNLOCK", quality);
    
    // 绘制简单的质量条
    for (uint8_t i = 0; i < quality / 10; i++) {
        Serial.print("█");
    }
    Serial.println();
}
```

### 2. 原始比特流导出

导出原始比特流用于离线分析：

```cpp
void exportBitStream(uint8_t bit) {
    static uint8_t buffer = 0;
    static uint8_t count = 0;
    
    buffer |= (bit << count);
    count++;
    
    if (count >= 8) {
        Serial.printf("%02X ", buffer);
        buffer = 0;
        count = 0;
    }
}
```

### 3. 能量分布可视化

实时显示Mark/Space能量：

```cpp
void plotEnergy(uint16_t markEnergy, uint16_t spaceEnergy) {
    uint16_t maxEnergy = max(markEnergy, spaceEnergy);
    
    Serial.print("M:");
    for (uint16_t i = 0; i < markEnergy * 40 / maxEnergy; i++) {
        Serial.print("=");
    }
    Serial.println();
    
    Serial.print("S:");
    for (uint16_t i = 0; i < spaceEnergy * 40 / maxEnergy; i++) {
        Serial.print("=");
    }
    Serial.println();
}
```

### 4. 帧错误分析

详细记录帧错误原因：

```cpp
enum FrameError {
    ERR_NONE,
    ERR_TOO_SHORT,
    ERR_FCS_FAIL,
    ERR_INVALID_ADDRESS,
    ERR_BUFFER_OVERFLOW
};

void logFrameError(FrameError err, const uint8_t* data, uint16_t len) {
    Serial.printf("[ERROR] Frame rejected: ");
    
    switch (err) {
        case ERR_TOO_SHORT:
            Serial.printf("Too short (%d bytes)\n", len);
            break;
        case ERR_FCS_FAIL:
            Serial.println("FCS check failed");
            hexDump(data, len);
            break;
        // ...
    }
}
```

### 5. 性能分析器

测量各个模块的执行时间：

```cpp
class Profiler {
    unsigned long startTime;
    const char* label;
    
public:
    Profiler(const char* lbl) : label(lbl) {
        startTime = micros();
    }
    
    ~Profiler() {
        unsigned long elapsed = micros() - startTime;
        Serial.printf("[PROF] %s: %lu us\n", label, elapsed);
    }
};

// 使用
void someFunction() {
    Profiler prof("someFunction");
    // ... 函数代码 ...
}
```

---

## 信号处理流程图

```
采样数据流 (26.4 kHz)
    │
    ├─> 环形缓冲区 (44 samples)
    │
    ├─> Mark相关器 (1200 Hz)
    │       └─> Mark能量
    │
    ├─> Space相关器 (2200 Hz)
    │       └─> Space能量
    │
    ├─> 能量比较 + 阈值判决
    │       └─> 原始比特
    │
    ├─> PLL时钟恢复
    │       └─> 同步比特
    │
    ├─> NRZI解码
    │       └─> 解码比特
    │
    ├─> 比特去填充
    │       └─> 有效比特
    │
    ├─> 字节组装 (LSB first)
    │       └─> 字节流
    │
    ├─> 帧标志检测 (0x7E)
    │       └─> 帧边界
    │
    ├─> AX.25解析
    │       ├─> 地址提取
    │       ├─> FCS校验
    │       └─> 信息字段
    │
    └─> APRS解码
            ├─> 类型判断
            ├─> 位置解析
            └─> TNC2格式化
```

---

## 推荐的参数调整

根据实际环境调整这些参数以获得最佳性能：

### 信号强时 (RSSI > -80 dBm)

```cpp
#define ENERGY_THRESHOLD_RATIO  1.10   // 更严格的判决
#define PLL_LOCKED_THRESHOLD    5      // 快速锁定
```

### 信号弱时 (RSSI < -100 dBm)

```cpp
#define ENERGY_THRESHOLD_RATIO  1.02   // 更宽松的判决
#define PLL_LOCKED_THRESHOLD    10     // 更稳定的锁定
#define DEMOD_WINDOW_SIZE       66     // 更大的窗口
```

### 高干扰环境

```cpp
#define ENERGY_THRESHOLD_RATIO  1.15   // 更高的信噪比要求
#define DEBUG_LEVEL             DEBUG_LEVEL_ERROR  // 减少串口干扰
```

---

## 参考资料

1. **AX.25协议规范**：[AX.25 Link Access Protocol](http://www.ax25.net/AX25.2.2-Jul%2098-2.pdf)
2. **APRS协议**：[APRS Protocol Reference](http://www.aprs.org/doc/APRS101.PDF)
3. **Bell 202标准**：[Bell 202 Modem](https://en.wikipedia.org/wiki/Bell_202_modem)
4. **数字信号处理**：*Understanding Digital Signal Processing* by Richard G. Lyons
5. **RadioLib文档**：[RadioLib GitHub](https://github.com/jgromes/RadioLib)

---

**作者注**：本文档持续更新中。如有技术问题，欢迎在GitHub Issues讨论。

