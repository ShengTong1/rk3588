"""
摄像头管理模块
"""

import cv2
import numpy as np
import threading
import time
from queue import Queue, Empty


class CameraManager:
    def __init__(self, camera_id=0, width=640, height=480, fps=30):
        self.camera_id = camera_id
        self.width = width
        self.height = height
        self.fps = fps
        self.cap = None
        self.is_opened = False
        self.is_capturing = False
        self.frame_queue = Queue(maxsize=2)
        self.capture_thread = None
        self.stop_event = threading.Event()
        self.latest_frame = None
        self.frame_lock = threading.Lock()

    def open_camera(self):
        try:
            self.cap = cv2.VideoCapture(self.camera_id)
            if not self.cap.isOpened():
                return False

            # 尝试设置分辨率
            success = self._set_resolution(self.width, self.height)
            if not success:
                print(f"警告: 无法设置分辨率 {self.width}x{self.height}，使用默认分辨率")

            self.cap.set(cv2.CAP_PROP_FPS, self.fps)
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            self.is_opened = True
            return True
        except Exception as e:
            print(f"打开摄像头失败: {e}")
            return False

    def _set_resolution(self, width, height):
        """设置分辨率并验证"""
        try:
            # 设置分辨率
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

            # 等待设置生效
            time.sleep(0.2)

            # 验证设置是否成功
            actual_width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

            # 尝试读取一帧验证
            ret, frame = self.cap.read()
            if ret and frame is not None:
                frame_height, frame_width = frame.shape[:2]
                if frame_width == width and frame_height == height:
                    print(f"成功设置分辨率: {width}x{height}")
                    return True
                else:
                    print(f"分辨率设置失败: 请求{width}x{height}, 实际{frame_width}x{frame_height}")
                    # 尝试回退到支持的分辨率
                    return self._try_fallback_resolutions()
            else:
                print(f"分辨率{width}x{height}设置后无法读取帧")
                return self._try_fallback_resolutions()

        except Exception as e:
            print(f"设置分辨率时出错: {e}")
            return self._try_fallback_resolutions()

    def _try_fallback_resolutions(self):
        """尝试回退分辨率"""
        fallback_resolutions = [
            (640, 480),   # VGA - 最常支持
            (320, 240),   # QVGA - 基本支持
            (800, 600),   # SVGA
            (1024, 768),  # XGA
        ]

        for width, height in fallback_resolutions:
            if width == self.width and height == self.height:
                continue  # 跳过已经尝试过的分辨率

            try:
                self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
                self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
                time.sleep(0.2)

                ret, frame = self.cap.read()
                if ret and frame is not None:
                    frame_height, frame_width = frame.shape[:2]
                    if frame_width == width and frame_height == height:
                        print(f"回退到支持的分辨率: {width}x{height}")
                        self.width = width  # 更新实际使用的分辨率
                        self.height = height
                        return True
            except:
                continue

        print("警告: 无法找到支持的分辨率，使用默认设置")
        return False

    def start_capture(self):
        if not self.is_opened or self.is_capturing:
            return self.is_capturing
        try:
            self.stop_event.clear()
            self.capture_thread = threading.Thread(target=self._capture_frames)
            self.capture_thread.daemon = True
            self.capture_thread.start()
            self.is_capturing = True
            return True
        except Exception as e:
            print(f"开始捕获失败: {e}")
            return False

    def _capture_frames(self):
        while not self.stop_event.is_set():
            try:
                ret, frame = self.cap.read()
                if ret:
                    with self.frame_lock:
                        self.latest_frame = frame.copy()
                    try:
                        self.frame_queue.put(frame, block=False)
                    except:
                        try:
                            self.frame_queue.get_nowait()
                            self.frame_queue.put(frame, block=False)
                        except Empty:
                            pass
                else:
                    time.sleep(0.01)
            except Exception as e:
                time.sleep(0.01)

    def get_frame(self, timeout=0.1):
        if not self.is_capturing:
            if self.cap and self.is_opened:
                ret, frame = self.cap.read()
                return frame if ret else None
            return None

        with self.frame_lock:
            if self.latest_frame is not None:
                return self.latest_frame.copy()
        return None

    def get_frame_from_queue(self, timeout=0.1):
        try:
            return self.frame_queue.get(timeout=timeout)
        except Empty:
            return None

    def stop_capture(self):
        if not self.is_capturing:
            return
        try:
            self.stop_event.set()
            if self.capture_thread and self.capture_thread.is_alive():
                self.capture_thread.join(timeout=1.0)
            while not self.frame_queue.empty():
                try:
                    self.frame_queue.get_nowait()
                except Empty:
                    break
            self.is_capturing = False
        except Exception as e:
            print(f"停止捕获时出错: {e}")

    def close_camera(self):
        try:
            self.stop_capture()
            if self.cap:
                self.cap.release()
                self.cap = None
            self.is_opened = False
        except Exception as e:
            print(f"关闭摄像头时出错: {e}")

    def get_camera_info(self):
        if not self.cap or not self.is_opened:
            return {}
        try:
            return {
                'width': int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH)),
                'height': int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT)),
                'fps': self.cap.get(cv2.CAP_PROP_FPS),
            }
        except:
            return {}

    def get_supported_resolutions(self):
        """获取摄像头支持的分辨率列表"""
        if not self.cap or not self.is_opened:
            return []

        test_resolutions = [
            (320, 240),    # QVGA
            (640, 480),    # VGA
            (800, 600),    # SVGA
            (1024, 768),   # XGA
            (1280, 720),   # HD
            (1280, 960),   # SXGA
            (1600, 1200),  # UXGA
            (1920, 1080),  # Full HD
        ]

        supported = []
        original_width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        original_height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

        for width, height in test_resolutions:
            try:
                self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
                self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
                time.sleep(0.1)

                ret, frame = self.cap.read()
                if ret and frame is not None:
                    frame_height, frame_width = frame.shape[:2]
                    if frame_width == width and frame_height == height:
                        supported.append((width, height))
            except:
                continue

        # 恢复原始分辨率
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, original_width)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, original_height)

        return supported

    def __del__(self):
        self.close_camera()


def list_available_cameras():
    available_cameras = []
    priority_cameras = [21, 22, 0, 1, 2]

    for i in priority_cameras:
        try:
            cap = cv2.VideoCapture(i)
            if cap.isOpened():
                ret, frame = cap.read()
                if ret and frame is not None:
                    available_cameras.append(i)
                cap.release()
            else:
                cap.release()
        except:
            pass

    return available_cameras
