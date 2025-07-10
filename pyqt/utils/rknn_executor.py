"""
RKNN模型执行器
"""

import numpy as np
import cv2

try:
    from rknnlite.api import RKNNLite
    class RKNN:
        def __init__(self, verbose=False):
            self.rknn_lite = RKNNLite(verbose=verbose)
        def load_rknn(self, path):
            return self.rknn_lite.load_rknn(path)
        def init_runtime(self, target=None, device_id=None):
            return self.rknn_lite.init_runtime()
        def inference(self, inputs):
            return self.rknn_lite.inference(inputs=inputs)
        def release(self):
            self.rknn_lite.release()
except ImportError:
    try:
        from rknn.api import RKNN
    except ImportError:
        class RKNN:
            def __init__(self, verbose=False): pass
            def load_rknn(self, path): return 0
            def init_runtime(self, target=None, device_id=None): return 0
            def inference(self, inputs):
                return [np.random.rand(1, 64, 80, 80).astype(np.float32),
                       np.random.rand(1, 9, 80, 80).astype(np.float32),
                       np.random.rand(1, 64, 40, 40).astype(np.float32),
                       np.random.rand(1, 9, 40, 40).astype(np.float32),
                       np.random.rand(1, 64, 20, 20).astype(np.float32),
                       np.random.rand(1, 9, 20, 20).astype(np.float32)]
            def release(self): pass


class RKNN_model_container:
    def __init__(self, model_path, target_platform='rk3588', device_id=None):
        self.model_path = model_path
        self.rknn = RKNN(verbose=False)
        self.is_initialized = False
        self._initialize_model()

    def _initialize_model(self):
        try:
            ret = self.rknn.load_rknn(self.model_path)
            if ret != 0:
                raise RuntimeError(f'Load RKNN model failed! ret={ret}')
            ret = self.rknn.init_runtime()
            if ret != 0:
                raise RuntimeError(f'Init runtime failed! ret={ret}')
            self.is_initialized = True
        except Exception as e:
            if self.rknn:
                self.rknn.release()
            raise

    def run(self, inputs):
        if not self.is_initialized:
            raise RuntimeError('RKNN model not initialized')
        return self.rknn.inference(inputs=inputs)

    def release(self):
        if self.rknn:
            self.rknn.release()
            self.is_initialized = False

    def __del__(self):
        self.release()


class COCO_test_helper:
    def __init__(self, enable_letter_box=True):
        pass

    def letter_box(self, im, new_shape=(640, 640), pad_color=(0, 0, 0)):
        shape = im.shape[:2]
        r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
        new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
        dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]
        dw /= 2
        dh /= 2

        if shape[::-1] != new_unpad:
            im = cv2.resize(im, new_unpad, interpolation=cv2.INTER_LINEAR)

        top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
        left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
        im = cv2.copyMakeBorder(im, top, bottom, left, right, cv2.BORDER_CONSTANT, value=pad_color)
        return im

    def get_real_box(self, boxes):
        return boxes
