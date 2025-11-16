# 旧文件参考说明

## 📁 文件状态

以下是项目中旧Arduino代码文件的状态和新文件对应关系。

---

## ✅ 已转换文件（可以删除或保留作参考）

### src/ 目录

| 旧文件 | 新文件位置 | 状态 | 操作建议 |
|--------|-----------|------|----------|
| `src/afsk_demod.h` | `Core/Inc/afsk_demod.h` | ✅ 已转换 | 可删除 |
| `src/afsk_demod.cpp` | `Core/Src/afsk_demod.c` | ✅ 已转换 | 可删除 |
| `src/nrzi_decoder.h` | `Core/Inc/nrzi_decoder.h` | ✅ 已转换 | 可删除 |
| `src/nrzi_decoder.cpp` | `Core/Src/nrzi_decoder.c` | ✅ 已转换 | 可删除 |
| `src/ax25_parser.h` | `Core/Inc/ax25_parser.h` | ✅ 已转换 | 可删除 |
| `src/ax25_parser.cpp` | `Core/Src/ax25_parser.c` | ✅ 已转换 | 可删除 |
| `src/aprs_config.h` | `Core/Inc/aprs_config.h` | ✅ 重写 | 可删除 |
| `src/aprs_decoder.h` | `Core/Src/main.c` | ✅ 整合到main | 可删除 |
| `src/aprs_decoder.cpp` | `Core/Src/main.c` | ✅ 整合到main | 可删除 |
| `src/aprs_decoder_enhanced.h` | `Core/Src/afsk_demod.c` | ✅ 整合到afsk | 可删除 |
| `src/aprs_decoder_enhanced.cpp` | `Core/Src/afsk_demod.c` | ✅ 整合到afsk | 可删除 |
| `src/stm32_hal.h` | `Core/Src/main.c` | ✅ 整合到main | 可删除 |
| `src/stm32_hal.cpp` | `Core/Src/main.c` | ✅ 整合到main | 可删除 |

### 根目录

| 旧文件 | 新文件位置 | 状态 | 操作建议 |
|--------|-----------|------|----------|
| `aprs-rf-decoder.ino` | `Core/Src/main.c` | ✅ 替代 | 可删除 |
| `sx1276-aprs-decoder.ino` | （已删除） | - | - |

---

## 🗑️ 可以安全删除的文件

如果您确认新的STM32 HAL代码工作正常，以下文件可以安全删除：

```bash
# 删除旧的Arduino源文件
rm -rf src/

# 删除旧的.ino文件
rm -f aprs-rf-decoder.ino
rm -f sx1276-aprs-decoder.ino  # 如果存在

# 保留参考文件（可选）
mkdir -p reference-file/old-src/
mv src/* reference-file/old-src/  # 如果想保留参考
```

---

## 📂 保留作参考（推荐）

如果您想保留旧代码作为参考，建议这样组织：

```
aprs-rf-decoder/
├── Core/                   # 新的STM32 HAL代码（使用这个）
│   ├── Inc/
│   └── Src/
├── reference-file/         # 参考代码和示例
│   ├── old-src/           # 旧的Arduino代码（仅供参考）
│   │   ├── afsk_demod.cpp
│   │   ├── nrzi_decoder.cpp
│   │   └── ...
│   ├── author-test.ino
│   └── SX127x_*.ino
└── ... (其他文件)
```

---

## ⚠️ 不要删除的文件

以下文件仍然需要或有用：

| 文件 | 原因 |
|------|------|
| `README.md` | 项目总体说明 |
| `reference-file/*.ino` | RadioLib使用参考 |
| `MIGRATION_GUIDE.md` | 迁移文档 |
| `STM32_PROJECT_README.md` | 项目说明 |
| `KEIL_PROJECT_SETUP.md` | 配置指南 |
| 所有 `Core/` 下的文件 | **新代码！** |

---

## 🔄 迁移验证清单

在删除旧文件前，请确认：

- [x] ✅ 所有头文件都已转换
- [x] ✅ 所有源文件都已转换
- [x] ✅ 新代码编译通过
- [ ] ⏳ 硬件测试通过
- [ ] ⏳ 功能验证完成

---

## 📊 文件对应关系图

```
旧Arduino架构                    新STM32 HAL架构
================                 ==================

aprs-rf-decoder.ino    ───→     Core/Src/main.c
                                Core/Inc/main.h

src/afsk_demod.cpp     ───→     Core/Src/afsk_demod.c
src/afsk_demod.h       ───→     Core/Inc/afsk_demod.h

src/nrzi_decoder.cpp   ───→     Core/Src/nrzi_decoder.c
src/nrzi_decoder.h     ───→     Core/Inc/nrzi_decoder.h

src/ax25_parser.cpp    ───→     Core/Src/ax25_parser.c
src/ax25_parser.h      ───→     Core/Inc/ax25_parser.h

src/aprs_decoder.cpp   ───→     Core/Src/main.c (状态机)
src/aprs_decoder.h     ───→     Core/Inc/main.h

src/aprs_config.h      ───→     Core/Inc/aprs_config.h (重写)

RadioLib依赖           ───→     Core/Src/sx127x.c (自实现)
                                Core/Inc/sx127x.h
```

---

## 💡 建议操作

### 选项1: 保留旧代码作参考
```bash
# 创建参考目录
mkdir -p reference-file/old-arduino-src

# 移动旧代码
mv src/* reference-file/old-arduino-src/
mv aprs-rf-decoder.ino reference-file/old-arduino-src/
rmdir src
```

### 选项2: 完全删除旧代码
```bash
# 删除旧代码（确认新代码工作正常后）
rm -rf src/
rm -f aprs-rf-decoder.ino
```

### 选项3: 先备份后删除
```bash
# 创建备份
tar -czf arduino-code-backup-$(date +%Y%m%d).tar.gz src/ aprs-rf-decoder.ino

# 删除旧代码
rm -rf src/
rm -f aprs-rf-decoder.ino
```

---

## 📝 总结

- ✅ **所有代码已转换完成**
- ✅ **新代码位于 Core/ 目录**
- ✅ **旧代码可以安全删除或保留作参考**
- ⚠️ **删除前请确认新代码编译和运行正常**

---

**建议**: 先保留旧代码，测试新代码完全正常后再删除。

