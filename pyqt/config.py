"""
配置文件
"""

import os

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))

MODEL_CONFIG = {
    'default_model_path': os.path.join(PROJECT_ROOT, 'models', 'yolov8n_tomato.rknn'),
    'default_labels_path': os.path.join(PROJECT_ROOT, 'models', 'coco_80_labels_list.txt'),
    'supported_platforms': ['rk3588', 'rk3566', 'rk3568', 'rk3576'],
    'default_platform': 'rk3588',
    'input_size': (640, 640),
    'default_conf_threshold': 0.25,
    'default_nms_threshold': 0.45,
}

CAMERA_CONFIG = {
    'supported_resolutions': [(640, 480), (1280, 720), (1920, 1080)],
    'default_resolution': (640, 480),
    'default_fps': 30,
    'max_camera_check': 10,
    'buffer_size': 1,
}

PATH_CONFIG = {
    'models_dir': os.path.join(PROJECT_ROOT, 'models'),
    'logs_dir': os.path.join(PROJECT_ROOT, 'logs'),
    'screenshots_dir': os.path.join(PROJECT_ROOT, 'screenshots'),
}

def ensure_directories():
    for dir_path in PATH_CONFIG.values():
        if not os.path.exists(dir_path):
            os.makedirs(dir_path, exist_ok=True)

def validate_config():
    errors = []
    if not os.path.exists(MODEL_CONFIG['default_model_path']):
        errors.append(f"默认模型文件不存在: {MODEL_CONFIG['default_model_path']}")
    if not os.path.exists(MODEL_CONFIG['default_labels_path']):
        errors.append(f"默认标签文件不存在: {MODEL_CONFIG['default_labels_path']}")
    return errors

def get_config_info():
    return {
        'project_root': PROJECT_ROOT,
        'model_path': MODEL_CONFIG['default_model_path'],
        'labels_path': MODEL_CONFIG['default_labels_path'],
        'platform': MODEL_CONFIG['default_platform'],
    }
