"""
RK3588 YOLOv8实时检测主程序
"""

import sys
import os
import time
import cv2
import numpy as np
from PyQt5.QtWidgets import QApplication, QMessageBox, QFileDialog
from PyQt5.QtCore import QThread, pyqtSignal, QTimer, Qt
from PyQt5.QtGui import QImage, QPixmap

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from ui.main_window import MainWindow
from utils.camera_manager import CameraManager, list_available_cameras
from utils.rknn_executor import RKNN_model_container, COCO_test_helper
from utils.yolov8_postprocess import YOLOv8PostProcessor


class DetectionThread(QThread):
    frame_ready = pyqtSignal(np.ndarray)
    performance_update = pyqtSignal(float, float, int)
    error_occurred = pyqtSignal(str)

    def __init__(self, camera_manager, model_container, postprocessor, class_names):
        super().__init__()
        self.camera_manager = camera_manager
        self.model_container = model_container
        self.postprocessor = postprocessor
        self.class_names = class_names
        self.coco_helper = COCO_test_helper()
        self.is_running = False
        self.fps_counter = 0
        self.fps_start_time = time.time()
        self.current_fps = 0

    def run(self):
        self.is_running = True
        while self.is_running:
            try:
                frame = self.camera_manager.get_frame()
                if frame is None:
                    time.sleep(0.01)
                    continue

                start_time = time.time()
                processed_frame = self.preprocess_frame(frame)
                outputs = self.model_container.run([processed_frame])
                boxes, classes, scores = self.postprocessor.post_process(outputs)
                inference_time = (time.time() - start_time) * 1000

                result_frame = frame.copy()
                detection_count = 0
                if boxes is not None:
                    detection_count = len(boxes)
                    result_frame = self.postprocessor.draw_detections(
                        result_frame, boxes, scores, classes, self.class_names)

                self.update_fps()
                self.frame_ready.emit(result_frame)
                self.performance_update.emit(self.current_fps, inference_time, detection_count)

            except Exception as e:
                self.error_occurred.emit(f"检测过程出错: {str(e)}")
                break

    def preprocess_frame(self, frame):
        img = self.coco_helper.letter_box(frame.copy(), (640, 640), (0, 0, 0))
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        return np.expand_dims(img, axis=0)

    def update_fps(self):
        self.fps_counter += 1
        current_time = time.time()
        if current_time - self.fps_start_time >= 1.0:
            self.current_fps = self.fps_counter / (current_time - self.fps_start_time)
            self.fps_counter = 0
            self.fps_start_time = current_time

    def stop(self):
        self.is_running = False
        self.wait()


class MainApplication:
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.window = MainWindow()
        self.camera_manager = None
        self.model_container = None
        self.postprocessor = None
        self.detection_thread = None
        self.class_names = []
        self.available_cameras = []

        self.connect_signals()
        self.update_camera_list()
        self.set_default_paths()

    def connect_signals(self):
        self.window.open_camera_btn.clicked.connect(self.open_camera)
        self.window.close_camera_btn.clicked.connect(self.close_camera)
        self.window.select_model_btn.clicked.connect(self.select_model_file)
        self.window.select_labels_btn.clicked.connect(self.select_labels_file)
        self.window.start_detection_btn.clicked.connect(self.start_detection)
        self.window.stop_detection_btn.clicked.connect(self.stop_detection)
        self.window.conf_threshold_slider.valueChanged.connect(self.update_detection_params)
        self.window.nms_threshold_slider.valueChanged.connect(self.update_detection_params)

    def update_camera_list(self):
        """更新摄像头列表"""
        try:
            self.window.log_message("正在检测摄像头...")
            available_cameras = list_available_cameras()
            self.window.camera_combo.clear()

            if available_cameras:
                for i, cam_id in enumerate(available_cameras):
                    self.window.camera_combo.addItem(f"摄像头 {cam_id} (/dev/video{cam_id})")
                self.window.log_message(f"发现 {len(available_cameras)} 个可用摄像头: {available_cameras}")

                # 存储摄像头ID列表供后续使用
                self.available_cameras = available_cameras
            else:
                self.window.camera_combo.addItem("未发现摄像头")
                self.window.log_message("未发现可用摄像头")
                self.available_cameras = []

        except Exception as e:
            self.window.log_message(f"检测摄像头失败: {e}")
            self.available_cameras = []

    def set_default_paths(self):
        """设置默认的模型和标签路径"""
        # 设置默认模型路径
        default_model = "models/yolov8n_tomato.rknn"
        if os.path.exists(default_model):
            self.window.model_path_label.setText(default_model)
            self.window.log_message(f"使用默认模型: {default_model}")

        # 设置默认标签路径
        default_labels = "models/coco_80_labels_list.txt"
        if os.path.exists(default_labels):
            self.window.labels_path_label.setText(default_labels)
            self.load_class_names(default_labels)
            self.window.log_message(f"使用默认标签: {default_labels}")

    def open_camera(self):
        """打开摄像头"""
        try:
            # 获取摄像头参数
            camera_index = self.window.camera_combo.currentIndex()
            resolution = self.window.resolution_combo.currentText()
            fps = 30  # 使用默认FPS值

            # 获取实际的摄像头ID
            if not hasattr(self, 'available_cameras') or not self.available_cameras:
                self.window.log_message("重新检测摄像头...")
                self.update_camera_list()

            if not self.available_cameras:
                raise Exception("未发现可用摄像头")

            if camera_index >= len(self.available_cameras):
                camera_index = 0

            camera_id = self.available_cameras[camera_index]
            self.window.log_message(f"尝试打开摄像头ID: {camera_id} (/dev/video{camera_id})")

            # 解析分辨率
            width, height = map(int, resolution.split('x'))

            # 创建摄像头管理器
            self.camera_manager = CameraManager(
                camera_id=camera_id,
                width=width,
                height=height,
                fps=fps
            )

            # 打开摄像头
            if self.camera_manager.open_camera():
                self.camera_manager.start_capture()

                # 获取摄像头信息
                camera_info = self.camera_manager.get_camera_info()
                actual_resolution = f"{camera_info.get('width', 'unknown')}x{camera_info.get('height', 'unknown')}"

                # 更新UI状态
                self.window.open_camera_btn.setEnabled(False)
                self.window.close_camera_btn.setEnabled(True)
                self.window.update_status("摄像头已打开")
                self.window.log_message(f"摄像头打开成功，实际分辨率: {actual_resolution}")

                # 如果实际分辨率与请求不同，给出提示
                if camera_info.get('width') != width or camera_info.get('height') != height:
                    self.window.log_message(f"注意: 请求分辨率{width}x{height}，实际使用{actual_resolution}")

                # 启动预览定时器
                self.start_preview()

            else:
                raise Exception("无法打开摄像头")

        except Exception as e:
            QMessageBox.critical(self.window, "错误", f"打开摄像头失败: {e}")
            self.window.log_message(f"打开摄像头失败: {e}")

    def close_camera(self):
        """关闭摄像头"""
        try:
            # 停止检测
            if self.detection_thread and self.detection_thread.isRunning():
                self.stop_detection()

            # 停止预览
            self.stop_preview()

            # 关闭摄像头
            if self.camera_manager:
                self.camera_manager.close_camera()
                self.camera_manager = None

            # 更新UI状态
            self.window.open_camera_btn.setEnabled(True)
            self.window.close_camera_btn.setEnabled(False)
            self.window.video_label.setText("摄像头未连接")
            self.window.update_status("摄像头已关闭")
            self.window.log_message("摄像头关闭成功")

        except Exception as e:
            self.window.log_message(f"关闭摄像头失败: {e}")

    def start_preview(self):
        """开始预览"""
        self.preview_timer = QTimer()
        self.preview_timer.timeout.connect(self.update_preview)
        self.preview_timer.start(33)  # 约30fps

    def stop_preview(self):
        """停止预览"""
        if hasattr(self, 'preview_timer'):
            self.preview_timer.stop()

    def update_preview(self):
        """更新预览画面"""
        if not self.camera_manager:
            return

        frame = self.camera_manager.get_frame()
        if frame is not None:
            self.display_frame(frame)

    def display_frame(self, frame):
        """显示帧到界面"""
        try:
            # 转换为Qt格式
            rgb_image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            h, w, ch = rgb_image.shape
            bytes_per_line = ch * w

            qt_image = QImage(rgb_image.data, w, h, bytes_per_line, QImage.Format_RGB888)

            # 缩放到标签大小
            label_size = self.window.video_label.size()
            scaled_pixmap = QPixmap.fromImage(qt_image).scaled(
                label_size, Qt.KeepAspectRatio, Qt.SmoothTransformation
            )

            self.window.video_label.setPixmap(scaled_pixmap)

        except Exception as e:
            self.window.log_message(f"显示帧失败: {e}")

    def select_model_file(self):
        """选择模型文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self.window, "选择RKNN模型文件", "", "RKNN Files (*.rknn)"
        )

        if file_path:
            self.window.model_path_label.setText(file_path)
            self.window.log_message(f"选择模型文件: {file_path}")

    def select_labels_file(self):
        """选择标签文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self.window, "选择标签文件", "", "Text Files (*.txt)"
        )

        if file_path:
            self.window.labels_path_label.setText(file_path)
            self.load_class_names(file_path)
            self.window.log_message(f"选择标签文件: {file_path}")

    def load_class_names(self, file_path):
        """加载类别名称"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                self.class_names = [line.strip() for line in f.readlines() if line.strip()]
            self.window.log_message(f"加载了 {len(self.class_names)} 个类别")
        except Exception as e:
            self.window.log_message(f"加载标签文件失败: {e}")

    def start_detection(self):
        """开始检测"""
        try:
            # 检查前置条件
            if not self.camera_manager or not self.camera_manager.is_opened:
                QMessageBox.warning(self.window, "警告", "请先打开摄像头")
                return

            model_path = self.window.model_path_label.text()
            if model_path == "未选择" or not os.path.exists(model_path):
                QMessageBox.warning(self.window, "警告", "请选择有效的模型文件")
                return

            if not self.class_names:
                QMessageBox.warning(self.window, "警告", "请选择标签文件")
                return

            # 初始化模型
            self.window.log_message("正在加载模型...")
            self.window.update_status("加载模型中...")

            platform = self.window.platform_combo.currentText()
            self.model_container = RKNN_model_container(model_path, platform)

            # 初始化后处理器
            conf_thresh = self.window.conf_threshold_slider.value() / 100.0
            nms_thresh = self.window.nms_threshold_slider.value() / 100.0

            self.postprocessor = YOLOv8PostProcessor(
                obj_thresh=conf_thresh,
                nms_thresh=nms_thresh,
                img_size=(640, 640)
            )

            # 停止预览定时器
            self.stop_preview()

            # 创建并启动检测线程
            self.detection_thread = DetectionThread(
                self.camera_manager,
                self.model_container,
                self.postprocessor,
                self.class_names
            )

            # 连接检测线程信号
            self.detection_thread.frame_ready.connect(self.display_frame)
            self.detection_thread.performance_update.connect(
                self.window.update_performance_info
            )
            self.detection_thread.error_occurred.connect(self.handle_detection_error)

            # 启动检测
            self.detection_thread.start()

            # 更新UI状态
            self.window.start_detection_btn.setEnabled(False)
            self.window.stop_detection_btn.setEnabled(True)
            self.window.update_status("检测中...")
            self.window.log_message("开始实时检测")

        except Exception as e:
            QMessageBox.critical(self.window, "错误", f"启动检测失败: {e}")
            self.window.log_message(f"启动检测失败: {e}")

    def stop_detection(self):
        """停止检测"""
        try:
            if self.detection_thread and self.detection_thread.isRunning():
                self.detection_thread.stop()
                self.detection_thread = None

            # 释放模型资源
            if self.model_container:
                self.model_container.release()
                self.model_container = None

            # 重新启动预览
            if self.camera_manager and self.camera_manager.is_opened:
                self.start_preview()

            # 更新UI状态
            self.window.start_detection_btn.setEnabled(True)
            self.window.stop_detection_btn.setEnabled(False)
            self.window.update_status("检测已停止")
            self.window.log_message("停止实时检测")

        except Exception as e:
            self.window.log_message(f"停止检测失败: {e}")

    def update_detection_params(self):
        """更新检测参数"""
        if self.postprocessor:
            conf_thresh = self.window.conf_threshold_slider.value() / 100.0
            nms_thresh = self.window.nms_threshold_slider.value() / 100.0

            self.postprocessor.obj_thresh = conf_thresh
            self.postprocessor.nms_thresh = nms_thresh

    def handle_detection_error(self, error_msg):
        """处理检测错误"""
        self.window.log_message(f"检测错误: {error_msg}")
        self.stop_detection()
        QMessageBox.critical(self.window, "检测错误", error_msg)

    def run(self):
        """运行应用程序"""
        self.window.show()
        return self.app.exec_()


if __name__ == "__main__":
    app = MainApplication()
    sys.exit(app.run())
