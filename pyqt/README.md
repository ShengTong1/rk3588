# RK3588 YOLOv8实时检测系统

基于PyQt5开发的RK3588平台YOLOv8实时目标检测应用程序，使用NPU进行高效推理。

## 功能特性

- 🎯 **实时检测**: 支持摄像头实时目标检测
- 🚀 **NPU加速**: 使用RK3588 NPU进行高效推理
- 🎨 **友好界面**: 基于PyQt5的直观用户界面
- ⚙️ **参数调节**: 支持实时调节检测阈值
- 📊 **性能监控**: 实时显示FPS和推理时间
- 🔧 **灵活配置**: 支持多种模型和摄像头配置

## 系统要求

### 硬件要求
- RK3588开发板或相关设备
- USB摄像头或CSI摄像头
- 至少2GB RAM

### 软件要求
- Ubuntu 20.04+ 或其他Linux发行版
- Python 3.7+
- RKNN Toolkit2
- PyQt5
- OpenCV

## 项目结构

```
rk3588-yolov8-detection/
├── main.py                 # 主程序入口
├── config.py              # 配置文件
├── requirements.txt       # Python依赖
├── run.sh                 # 启动脚本
├── README.md              # 项目说明
├── ui/                    # 用户界面模块
│   └── main_window.py     # 主窗口界面
├── utils/                 # 工具模块
│   ├── camera_manager.py  # 摄像头管理
│   ├── rknn_executor.py   # RKNN执行器
│   └── yolov8_postprocess.py # YOLOv8后处理
├── models/                # 模型文件目录
│   ├── yolov8n_tomato.rknn    # RKNN模型文件
│   └── coco_80_labels_list.txt # 类别标签文件
├── yolov8/               # YOLOv8官方示例代码
└── logs/                 # 日志文件目录
```

## 安装步骤

### 1. 克隆项目
```bash
git clone <repository-url>
cd rk3588-yolov8-detection
```

### 2. 安装RKNN Toolkit2
请从瑞芯微官方获取RKNN Toolkit2并按照官方文档安装。

### 3. 安装Python依赖
```bash
pip3 install -r requirements.txt
```

### 4. 准备模型文件
将您训练好的YOLOv8 RKNN模型文件放入`models/`目录，并确保标签文件正确。

## 使用方法

### 快速启动
```bash
./run.sh
```

### 手动启动
```bash
python3 main.py
```

### 启动选项
```bash
./run.sh --help          # 显示帮助信息
./run.sh --check         # 仅执行环境检查
./run.sh --no-check      # 跳过检查直接启动
```

## 界面说明

### 界面规格
- **界面分辨率**: 1024 x 573 像素
- **适配屏幕**: 专为1024x600分辨率屏幕优化设计
- **布局比例**: 左侧视频区域 650px，右侧控制面板 350px
- **窗口控制**: 支持最小化、最大化和关闭按钮
- **最小尺寸**: 800 x 450 像素（确保界面可用性）
- **自适应布局**: 窗口大小改变时自动调整组件比例
- **字体优化**: 右侧控制面板采用紧凑字体设计，确保文字完整显示

### 主界面布局
- **左上**: 视频显示区域，显示摄像头画面和检测结果 (550x340像素)
- **左下**: 日志信息区域，显示系统状态和操作日志 (高度120像素)
- **右侧**: 控制面板，包含各种设置和控制选项

### 控制面板功能

#### 摄像头控制
- 摄像头选择：选择要使用的摄像头设备
- 分辨率设置：设置摄像头分辨率（帧率固定为30fps）
- 开启/关闭摄像头

#### 模型配置
- 模型文件选择：选择RKNN模型文件
- 标签文件选择：选择类别标签文件
- 目标平台选择：选择NPU平台（默认rk3588）

#### 检测参数
- 置信度阈值：调节检测置信度阈值（0.01-1.00）
- NMS阈值：调节非极大值抑制阈值（0.01-1.00）
- 显示选项：控制标签和置信度的显示

#### 检测控制
- 开始/停止检测
- 性能信息显示（FPS、推理时间、检测数量）

## 配置说明

### 模型配置
在`config.py`中可以修改默认配置：

```python
MODEL_CONFIG = {
    'default_model_path': 'models/yolov8n_tomato.rknn',
    'default_labels_path': 'models/coco_80_labels_list.txt',
    'default_platform': 'rk3588',
    'input_size': (640, 640),
    'default_conf_threshold': 0.25,
    'default_nms_threshold': 0.45,
}
```

### 摄像头配置
```python
CAMERA_CONFIG = {
    'supported_resolutions': [(640, 480), (1280, 720), (1920, 1080)],
    'default_resolution': (640, 480),
    'default_fps': 30,
}
```

## 模型转换

如果您有PyTorch的YOLOv8模型，可以使用以下步骤转换为RKNN格式：

1. 将PyTorch模型转换为ONNX格式
2. 使用RKNN Toolkit2将ONNX模型转换为RKNN格式

参考`yolov8/python/convert.py`中的转换脚本。

## 故障排除

### 常见问题

1. **摄像头无法打开**
   - 检查摄像头是否正确连接
   - 确认摄像头设备权限
   - 尝试不同的摄像头ID

2. **摄像头分辨率问题**
   - 某些摄像头只支持特定分辨率（如640x480）
   - 系统会自动回退到支持的分辨率
   - 使用诊断工具检查: `python3 diagnose_camera.py`
   - 查看摄像头支持的格式: `v4l2-ctl --list-formats-ext -d /dev/video21`

3. **模型加载失败**
   - 确认RKNN模型文件路径正确
   - 检查RKNN Toolkit2是否正确安装
   - 确认模型文件完整性

4. **检测性能差**
   - 调整检测阈值参数
   - 检查NPU是否正常工作
   - 考虑降低输入分辨率

5. **界面显示异常**
   - 确认PyQt5正确安装
   - 检查系统图形环境
   - 尝试更新显卡驱动

### 日志查看
程序运行日志保存在`logs/`目录中，可以查看详细的错误信息。

## 性能优化

1. **模型优化**
   - 使用量化模型减少计算量
   - 选择合适的模型尺寸
   - 优化模型结构

2. **系统优化**
   - 调整系统CPU频率
   - 优化内存使用
   - 减少不必要的后台进程

## 开发说明

### 代码结构
- `main.py`: 主程序，整合各个模块
- `ui/main_window.py`: PyQt5界面定义
- `utils/`: 核心功能模块
  - `camera_manager.py`: 摄像头管理
  - `rknn_executor.py`: RKNN模型执行
  - `yolov8_postprocess.py`: YOLOv8后处理

### 扩展开发
- 添加新的检测模型支持
- 扩展界面功能
- 添加数据记录功能
- 集成其他传感器

## 许可证

本项目基于MIT许可证开源。

## 贡献

欢迎提交Issue和Pull Request来改进项目。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交GitHub Issue
- 发送邮件至项目维护者

---

**注意**: 本项目专为RK3588平台设计，在其他平台上可能需要适当修改。
