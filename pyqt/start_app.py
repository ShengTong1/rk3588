#!/usr/bin/env python3
"""
启动应用程序的简化脚本
用于在没有图形环境时测试核心功能
"""

import sys
import os
import time

# 添加项目路径
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

def test_camera_and_model():
    """测试摄像头和模型加载"""
    print("=== 测试摄像头和模型 ===")
    
    # 测试摄像头
    from utils.camera_manager import CameraManager, list_available_cameras
    
    print("1. 检测摄像头...")
    cameras = list_available_cameras()
    if not cameras:
        print("✗ 未发现可用摄像头")
        return False
    
    print(f"✓ 发现摄像头: {cameras}")
    
    # 测试摄像头打开
    camera_id = cameras[0]
    camera = CameraManager(camera_id=camera_id, width=640, height=480)
    
    if not camera.open_camera():
        print("✗ 摄像头打开失败")
        return False
    
    print("✓ 摄像头打开成功")
    
    # 测试帧获取
    frame = camera.get_frame()
    if frame is None:
        print("✗ 获取帧失败")
        camera.close_camera()
        return False
    
    print(f"✓ 成功获取帧，尺寸: {frame.shape}")
    
    # 测试模型加载
    print("2. 测试模型加载...")
    from utils.rknn_executor import RKNN_model_container
    
    model_path = "models/yolov8n_tomato.rknn"
    if not os.path.exists(model_path):
        print(f"✗ 模型文件不存在: {model_path}")
        camera.close_camera()
        return False
    
    try:
        model = RKNN_model_container(model_path, 'rk3588')
        print("✓ 模型加载成功")
        
        # 测试推理
        print("3. 测试模型推理...")
        from utils.yolov8_postprocess import YOLOv8PostProcessor
        from utils.rknn_executor import COCO_test_helper
        
        # 预处理图像
        helper = COCO_test_helper()
        processed_frame = helper.letter_box(frame, new_shape=(640, 640))

        # 转换颜色空间
        import cv2
        import numpy as np
        processed_frame = cv2.cvtColor(processed_frame, cv2.COLOR_BGR2RGB)

        # 添加batch维度 (1, H, W, C)
        processed_frame = np.expand_dims(processed_frame, axis=0)
        print(f"  预处理后形状: {processed_frame.shape}")

        # 推理
        start_time = time.time()
        outputs = model.run([processed_frame])
        inference_time = (time.time() - start_time) * 1000
        
        print(f"✓ 推理成功，耗时: {inference_time:.2f}ms")
        print(f"  输出数量: {len(outputs)}")
        for i, output in enumerate(outputs):
            print(f"  输出{i}形状: {output.shape}")
        
        # 测试后处理
        print("4. 测试后处理...")
        postprocessor = YOLOv8PostProcessor()
        boxes, classes, scores = postprocessor.post_process(outputs)
        
        if boxes is not None:
            print(f"✓ 检测到 {len(boxes)} 个目标")
            for i, (box, cls, score) in enumerate(zip(boxes, classes, scores)):
                print(f"  目标{i}: 类别{cls}, 置信度{score:.3f}, 位置{box}")
        else:
            print("✓ 后处理完成，未检测到目标")
        
        # 清理
        model.release()
        camera.close_camera()
        
        print("✓ 所有测试通过！")
        return True
        
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        camera.close_camera()
        return False

def start_gui_app():
    """启动GUI应用程序"""
    print("=== 启动GUI应用程序 ===")
    
    try:
        import os
        # 检查是否有显示环境
        if 'DISPLAY' not in os.environ and 'WAYLAND_DISPLAY' not in os.environ:
            print("⚠ 没有图形显示环境")
            print("请在有图形环境的终端中运行以下命令:")
            print("  python3 main.py")
            return False
        
        # 启动GUI应用
        from main import MainApplication
        app = MainApplication()
        return app.run()
        
    except Exception as e:
        print(f"✗ GUI启动失败: {e}")
        return False

def main():
    """主函数"""
    print("RK3588 YOLOv8实时检测系统")
    print("=" * 40)
    
    # 检查命令行参数
    if len(sys.argv) > 1:
        if sys.argv[1] == "--test":
            # 仅测试核心功能
            success = test_camera_and_model()
            sys.exit(0 if success else 1)
        elif sys.argv[1] == "--gui":
            # 强制启动GUI
            success = start_gui_app()
            sys.exit(0 if success else 1)
        elif sys.argv[1] == "--help":
            print("用法:")
            print("  python3 start_app.py          # 自动选择模式")
            print("  python3 start_app.py --test   # 仅测试核心功能")
            print("  python3 start_app.py --gui    # 强制启动GUI")
            print("  python3 start_app.py --help   # 显示帮助")
            sys.exit(0)
    
    # 自动选择模式
    import os
    if 'DISPLAY' in os.environ or 'WAYLAND_DISPLAY' in os.environ:
        print("检测到图形环境，启动GUI应用...")
        success = start_gui_app()
    else:
        print("未检测到图形环境，运行核心功能测试...")
        success = test_camera_and_model()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
