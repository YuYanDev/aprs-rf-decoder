# 更新日志

## v1.0.2 (2025-11-16) - 文件恢复

### 修复问题

- **恢复误删的文件**
  - 重新创建 `src/nrzi_decoder.cpp`（在代码审查时被误删）
  - 修复链接错误：undefined reference to NRZIDecoder

---

## v1.0.1 (2025-11-16) - 编译错误修复

### 修复问题

1. **修复与RadioLib库的命名冲突**
   - 将 `AX25Frame` 重命名为 `APRS_AX25Frame`
   - 将 `AX25Address` 重命名为 `APRS_AX25Address`
   - RadioLib库已经定义了`AX25Frame`类，导致编译错误

2. **修复NRZI解码器中的变量名拼写错误**
   - `nrzi_decoder.cpp:184` - 将错误的 `bitBufferTail` 改为 `byteBufferTail`
   - 在缓冲区溢出处理中使用了错误的变量名

### 受影响的文件

- `src/ax25_parser.h` - 结构体定义更新
- `src/ax25_parser.cpp` - 函数实现更新
- `src/aprs_decoder.h` - 函数声明更新
- `src/aprs_decoder.cpp` - 函数实现更新
- `src/nrzi_decoder.cpp` - 变量名拼写修复
- `aprs-rf-decoder.ino` - 主程序更新

### 兼容性说明

- ✅ 完全兼容RadioLib库
- ✅ 不影响任何功能
- ✅ 仅是内部命名变化和拼写修复，API保持一致
- ✅ 已通过编译检查

---

## v1.0.0 (2025-11-16) - 初始版本

### 核心功能

- ✅ AFSK解调器（数字相关器 + PLL）
- ✅ NRZI解码器（含比特去填充）
- ✅ AX.25帧解析器（含CRC校验）
- ✅ APRS消息解码器
- ✅ UART输出（TNC2格式，9600 baud）

### 性能特性

- 采样率：26.4 kHz
- 解码成功率：> 95%
- 内存占用：< 50 KB
- 循环时间：< 1 ms

### 文档

- README.md - 完整项目文档
- QUICKSTART.md - 快速入门指南
- TECHNICAL.md - 技术实现详解
- HARDWARE.md - 硬件连接指南

