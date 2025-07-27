# ESP32-A1S-AudioKit 硬件迁移说明

## 概述

本文档记录了将音频延迟项目从 ESP32-LyraT-Mini 迁移到 AI Thinker ESP32-A1S-AudioKit 的所有更改。

## 硬件差异

### 原硬件 (ESP32-LyraT-Mini)
- 音频编解码器：ES8311
- I2S BCK 引脚：GPIO5
- I2C 接口：未明确指定

### 新硬件 (ESP32-A1S-AudioKit)
- 音频编解码器：ES8388
- I2S BCK 引脚：GPIO27
- I2C 接口：SDA=GPIO33, SCL=GPIO32
- I2C 地址：0x10

## 代码更改

### 1. I2S 引脚配置更新

**文件：** `main/audio_delay.c`

```c
// 原配置 (ESP32-LyraT-Mini)
#define I2S_BCK_PIN GPIO_NUM_5

// 新配置 (ESP32-A1S-AudioKit)
#define I2S_BCK_PIN GPIO_NUM_27
```

其他 I2S 引脚保持不变：
- WS: GPIO25
- DATA_IN: GPIO35
- DATA_OUT: GPIO26

### 2. 新增 ES8388 驱动

**新增文件：**
- `main/include/es8388_driver.h` - ES8388 驱动头文件
- `main/es8388_driver.c` - ES8388 驱动实现

**主要功能：**
- ES8388 初始化和配置
- I2C 通信管理
- 采样率和位宽设置
- 音量控制和静音功能
- 启动/停止控制

### 3. 音频延迟模块集成

**文件：** `main/audio_delay.c`

**主要更改：**
- 添加 ES8388 驱动头文件包含
- 在 `audio_delay_init()` 中初始化 ES8388
- 在 `audio_delay_deinit()` 中清理 ES8388
- 更新错误处理以包含 ES8388 清理

### 4. 构建配置更新

**文件：** `main/CMakeLists.txt`

添加新的源文件：
```cmake
SRCS 
    "main.c"
    "audio_delay.c"
    "ec11_encoder.c"
    "oled_display.c"
    "settings_manager.c"
    "ui_manager.c"
    "es8388_driver.c"  # 新增
```

## ES8388 配置详情

### I2C 配置
- 地址：0x10
- SDA 引脚：GPIO33
- SCL 引脚：GPIO32
- 频率：100kHz

### 支持的采样率
- 8kHz, 11.025kHz, 16kHz, 22.05kHz
- 32kHz, 44.1kHz, 48kHz, 96kHz, 192kHz

### 支持的位宽
- 16位, 18位, 20位, 24位, 32位

### 默认配置
- 采样率：48kHz
- 位宽：16位
- 格式：I2S
- 模式：从模式 (Slave)

## 初始化流程

1. **ES8388 初始化**
   - 创建 I2C 主机总线
   - 添加 ES8388 设备到 I2C 总线
   - 测试与 ES8388 的通信
   - 复位 ES8388
   - 配置电源管理
   - 配置 ADC 和 DAC
   - 配置输出

2. **I2S 初始化**
   - 创建 I2S 通道
   - 配置标准模式
   - 启用 TX/RX 通道

3. **启动 ES8388**
   - 启动 ADC 和 DAC

## 兼容性说明

### GPIO 冲突避免
- ES8388 使用 GPIO32/33 作为 I2C 接口
- 外接设备（EC11 编码器、OLED）使用不同的 GPIO
- 确保没有引脚冲突

### 版本兼容性
- 适用于 ESP32-A1S-AudioKit V2.2 974 及以后版本
- 早期版本使用 AC101 编解码器，不兼容此代码

## 测试建议

1. **硬件连接测试**
   - 验证 I2C 通信 (ES8388)
   - 验证 I2S 音频数据传输
   - 验证外设连接 (编码器、OLED)

2. **功能测试**
   - 音频输入/输出测试
   - 延迟功能测试
   - 采样率切换测试
   - 用户界面交互测试

3. **性能测试**
   - 音频质量测试
   - 延迟精度测试
   - 长时间运行稳定性测试

## 故障排除

### 常见问题
1. **ES8388 初始化失败**
   - 检查 I2C 连接 (GPIO32/33)
   - 验证 I2C 地址 (0x10)
   - 检查电源供应

2. **音频无输出**
   - 验证 I2S 引脚配置
   - 检查 ES8388 启动状态
   - 确认音频插孔连接

3. **编译错误**
   - 确保所有新文件已添加到 CMakeLists.txt
   - 验证头文件路径

## 后续优化建议

1. **音频质量优化**
   - 调整 ES8388 的增益设置
   - 优化 ADC/DAC 配置
   - 添加音频滤波

2. **功能扩展**
   - 利用 ES8388 的高级功能
   - 添加音效处理
   - 支持立体声模式

3. **用户体验**
   - 添加音量控制界面
   - 实现音频格式切换
   - 优化启动时间
