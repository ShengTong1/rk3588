"""
PyQt主界面
RK3588 YOLOv8实时检测应用程序的主界面
"""

import sys
import os
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                             QHBoxLayout, QLabel, QPushButton, QComboBox,
                             QSlider, QSpinBox, QDoubleSpinBox, QGroupBox,
                             QGridLayout, QTextEdit, QSplitter, QFrame,
                             QMessageBox, QFileDialog, QCheckBox)
from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QThread, pyqtSlot
from PyQt5.QtGui import QPixmap, QImage, QFont, QPalette, QColor


class MainWindow(QMainWindow):
    """主窗口类"""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("RK3588 YOLOv8实时检测系统")
        self.setGeometry(100, 100, 1024, 573)

        # 设置窗口标志，启用最小化和最大化按钮
        self.setWindowFlags(Qt.Window | Qt.WindowMinimizeButtonHint | Qt.WindowMaximizeButtonHint | Qt.WindowCloseButtonHint)

        # 设置最小窗口尺寸，确保界面可用性
        self.setMinimumSize(800, 450)

        # 设置样式
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f0f0f0;
            }
            QGroupBox {
                font-weight: bold;
                font-size: 12px;
                border: 2px solid #cccccc;
                border-radius: 5px;
                margin-top: 1ex;
                padding-top: 8px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
                font-size: 12px;
            }
            QLabel {
                font-size: 11px;
            }
            QComboBox {
                font-size: 11px;
                padding: 2px;
            }
            QSlider {
                font-size: 11px;
            }
            QCheckBox {
                font-size: 11px;
            }
            QPushButton {
                background-color: #4CAF50;
                border: none;
                color: white;
                padding: 6px 12px;
                text-align: center;
                font-size: 12px;
                border-radius: 4px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:pressed {
                background-color: #3d8b40;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
        """)

        # 初始化UI
        self.init_ui()

        # 状态变量
        self.is_camera_opened = False
        self.is_detecting = False

    def resizeEvent(self, event):
        """窗口大小改变事件处理"""
        super().resizeEvent(event)
        # 当窗口大小改变时，自动调整分割器比例
        if hasattr(self, 'splitter'):
            total_width = self.width() - 40  # 减去边距
            left_width = int(total_width * 0.65)  # 左侧占65%
            right_width = total_width - left_width  # 右侧占35%
            self.splitter.setSizes([left_width, right_width])

    def init_ui(self):
        """初始化用户界面"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        # 主布局
        main_layout = QHBoxLayout(central_widget)

        # 创建分割器
        self.splitter = QSplitter(Qt.Horizontal)
        main_layout.addWidget(self.splitter)

        # 左侧：视频显示区域
        self.create_video_area(self.splitter)

        # 右侧：控制面板
        self.create_control_panel(self.splitter)

        # 设置分割器比例 (适配1024*573分辨率)
        self.splitter.setSizes([650, 350])

    def create_video_area(self, parent):
        """创建视频显示区域"""
        video_widget = QWidget()
        video_layout = QVBoxLayout(video_widget)

        # 视频显示标签 (适中尺寸)
        self.video_label = QLabel()
        self.video_label.setMinimumSize(550, 340)  # 适中摄像头显示区域，避免重叠
        self.video_label.setStyleSheet("""
            QLabel {
                border: 2px solid #cccccc;
                background-color: #000000;
                color: white;
                font-size: 16px;
            }
        """)
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setText("摄像头未连接")

        video_layout.addWidget(self.video_label)

        # 添加日志显示区域到左下角
        self.create_log_area(video_layout)

        parent.addWidget(video_widget)

    def create_control_panel(self, parent):
        """创建控制面板"""
        control_widget = QWidget()
        control_layout = QVBoxLayout(control_widget)

        # 摄像头控制组
        self.create_camera_control_group(control_layout)

        # 模型配置组
        self.create_model_config_group(control_layout)

        # 检测参数组
        self.create_detection_params_group(control_layout)

        # 检测控制组
        self.create_detection_control_group(control_layout)

        # 添加弹性空间
        control_layout.addStretch()

        parent.addWidget(control_widget)

    def create_camera_control_group(self, parent_layout):
        """创建摄像头控制组"""
        group = QGroupBox("摄像头控制")
        layout = QGridLayout(group)

        # 摄像头选择
        layout.addWidget(QLabel("摄像头:"), 0, 0)
        self.camera_combo = QComboBox()
        self.camera_combo.addItems(["摄像头 0", "摄像头 1", "摄像头 2"])
        layout.addWidget(self.camera_combo, 0, 1)

        # 分辨率选择
        layout.addWidget(QLabel("分辨率:"), 1, 0)
        self.resolution_combo = QComboBox()
        self.resolution_combo.addItems(["640x480", "1280x720", "1920x1080"])
        layout.addWidget(self.resolution_combo, 1, 1)

        # 摄像头控制按钮
        button_layout = QHBoxLayout()
        self.open_camera_btn = QPushButton("打开摄像头")
        self.close_camera_btn = QPushButton("关闭摄像头")
        self.close_camera_btn.setEnabled(False)

        # 设置摄像头按钮的紧凑样式
        camera_button_style = "font-size: 10px; padding: 4px 6px;"
        self.open_camera_btn.setStyleSheet(camera_button_style)
        self.close_camera_btn.setStyleSheet(camera_button_style)

        button_layout.addWidget(self.open_camera_btn)
        button_layout.addWidget(self.close_camera_btn)
        layout.addLayout(button_layout, 2, 0, 1, 2)

        parent_layout.addWidget(group)

    def create_model_config_group(self, parent_layout):
        """创建模型配置组"""
        group = QGroupBox("模型配置")
        layout = QGridLayout(group)

        # 模型文件选择
        layout.addWidget(QLabel("模型文件:"), 0, 0)
        self.model_path_label = QLabel("未选择")
        self.model_path_label.setStyleSheet("border: 1px solid #cccccc; padding: 3px; font-size: 10px;")
        layout.addWidget(self.model_path_label, 0, 1)

        self.select_model_btn = QPushButton("选择模型")
        self.select_model_btn.setStyleSheet("font-size: 10px; padding: 4px 6px;")
        layout.addWidget(self.select_model_btn, 0, 2)

        # 标签文件选择
        layout.addWidget(QLabel("标签文件:"), 1, 0)
        self.labels_path_label = QLabel("未选择")
        self.labels_path_label.setStyleSheet("border: 1px solid #cccccc; padding: 3px; font-size: 10px;")
        layout.addWidget(self.labels_path_label, 1, 1)

        self.select_labels_btn = QPushButton("选择标签")
        self.select_labels_btn.setStyleSheet("font-size: 10px; padding: 4px 6px;")
        layout.addWidget(self.select_labels_btn, 1, 2)

        # 目标平台
        layout.addWidget(QLabel("目标平台:"), 2, 0)
        self.platform_combo = QComboBox()
        self.platform_combo.addItems(["rk3588", "rk3566", "rk3568", "rk3576"])
        layout.addWidget(self.platform_combo, 2, 1, 1, 2)

        parent_layout.addWidget(group)

    def create_detection_params_group(self, parent_layout):
        """创建检测参数组"""
        group = QGroupBox("检测参数")
        layout = QGridLayout(group)

        # 置信度阈值
        layout.addWidget(QLabel("置信度阈值:"), 0, 0)
        self.conf_threshold_slider = QSlider(Qt.Horizontal)
        self.conf_threshold_slider.setRange(1, 100)
        self.conf_threshold_slider.setValue(25)
        layout.addWidget(self.conf_threshold_slider, 0, 1)

        self.conf_threshold_label = QLabel("0.25")
        layout.addWidget(self.conf_threshold_label, 0, 2)

        # NMS阈值
        layout.addWidget(QLabel("NMS阈值:"), 1, 0)
        self.nms_threshold_slider = QSlider(Qt.Horizontal)
        self.nms_threshold_slider.setRange(1, 100)
        self.nms_threshold_slider.setValue(45)
        layout.addWidget(self.nms_threshold_slider, 1, 1)

        self.nms_threshold_label = QLabel("0.45")
        layout.addWidget(self.nms_threshold_label, 1, 2)

        # 显示选项
        self.show_labels_checkbox = QCheckBox("显示标签")
        self.show_labels_checkbox.setChecked(True)
        layout.addWidget(self.show_labels_checkbox, 2, 0)

        self.show_conf_checkbox = QCheckBox("显示置信度")
        self.show_conf_checkbox.setChecked(True)
        layout.addWidget(self.show_conf_checkbox, 2, 1)

        # 连接信号
        self.conf_threshold_slider.valueChanged.connect(
            lambda v: self.conf_threshold_label.setText(f"{v/100:.2f}"))
        self.nms_threshold_slider.valueChanged.connect(
            lambda v: self.nms_threshold_label.setText(f"{v/100:.2f}"))

        parent_layout.addWidget(group)

    def create_detection_control_group(self, parent_layout):
        """创建检测控制组"""
        group = QGroupBox("检测控制")
        layout = QVBoxLayout(group)

        # 检测控制按钮 (缩小尺寸)
        button_layout = QHBoxLayout()
        self.start_detection_btn = QPushButton("开始检测")
        self.stop_detection_btn = QPushButton("停止检测")
        self.stop_detection_btn.setEnabled(False)

        # 设置按钮的紧凑样式
        compact_button_style = """
            QPushButton {
                background-color: #4CAF50;
                border: none;
                color: white;
                padding: 4px 8px;
                text-align: center;
                font-size: 12px;
                border-radius: 3px;
                max-height: 28px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:pressed {
                background-color: #3d8b40;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
        """
        self.start_detection_btn.setStyleSheet(compact_button_style)
        self.stop_detection_btn.setStyleSheet(compact_button_style)

        button_layout.addWidget(self.start_detection_btn)
        button_layout.addWidget(self.stop_detection_btn)
        layout.addLayout(button_layout)

        # 性能信息
        perf_layout = QGridLayout()
        perf_layout.addWidget(QLabel("FPS:"), 0, 0)
        self.fps_label = QLabel("0")
        perf_layout.addWidget(self.fps_label, 0, 1)

        perf_layout.addWidget(QLabel("推理时间:"), 1, 0)
        self.inference_time_label = QLabel("0 ms")
        perf_layout.addWidget(self.inference_time_label, 1, 1)

        perf_layout.addWidget(QLabel("检测数量:"), 2, 0)
        self.detection_count_label = QLabel("0")
        perf_layout.addWidget(self.detection_count_label, 2, 1)

        layout.addLayout(perf_layout)

        parent_layout.addWidget(group)

    def create_log_area(self, parent_layout):
        """创建日志显示区域"""
        group = QGroupBox("日志信息")
        layout = QVBoxLayout(group)

        self.log_text = QTextEdit()
        self.log_text.setMaximumHeight(120)  # 缩小高度，为摄像头区域让出空间
        self.log_text.setReadOnly(True)
        self.log_text.setStyleSheet("""
            QTextEdit {
                background-color: #f8f8f8;
                border: 1px solid #cccccc;
                font-family: 'Courier New', monospace;
                font-size: 10px;
            }
        """)
        layout.addWidget(self.log_text)

        # 清除日志按钮 (紧凑样式)
        clear_log_btn = QPushButton("清除日志")
        clear_log_btn.setStyleSheet("font-size: 10px; padding: 3px 6px; max-height: 24px;")
        clear_log_btn.clicked.connect(self.log_text.clear)
        layout.addWidget(clear_log_btn)

        parent_layout.addWidget(group)

    def log_message(self, message):
        """添加日志消息"""
        import datetime
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")

        # 自动滚动到底部
        scrollbar = self.log_text.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

    def update_status(self, status):
        """更新状态显示 - 现在通过日志显示状态"""
        self.log_message(f"状态: {status}")

    def update_performance_info(self, fps, inference_time, detection_count):
        """更新性能信息"""
        self.fps_label.setText(f"{fps:.1f}")
        self.inference_time_label.setText(f"{inference_time:.1f} ms")
        self.detection_count_label.setText(str(detection_count))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
