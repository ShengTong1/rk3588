#!/bin/bash

# RK3588 YOLOv8实时检测系统启动脚本

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查Python版本
check_python() {
    print_info "检查Python环境..."
    
    if command -v python3 &> /dev/null; then
        PYTHON_CMD="python3"
    elif command -v python &> /dev/null; then
        PYTHON_CMD="python"
    else
        print_error "未找到Python解释器"
        exit 1
    fi
    
    # 检查Python版本
    PYTHON_VERSION=$($PYTHON_CMD --version 2>&1 | cut -d' ' -f2)
    print_success "找到Python: $PYTHON_VERSION"
}

# 检查依赖包
check_dependencies() {
    print_info "检查依赖包..."
    
    # 检查关键依赖
    REQUIRED_PACKAGES=("PyQt5" "cv2" "numpy")
    MISSING_PACKAGES=()
    
    for package in "${REQUIRED_PACKAGES[@]}"; do
        if ! $PYTHON_CMD -c "import $package" &> /dev/null; then
            MISSING_PACKAGES+=("$package")
        fi
    done
    
    if [ ${#MISSING_PACKAGES[@]} -ne 0 ]; then
        print_warning "缺少以下依赖包: ${MISSING_PACKAGES[*]}"
        print_info "尝试安装依赖包..."
        
        if [ -f "requirements.txt" ]; then
            $PYTHON_CMD -m pip install -r requirements.txt
        else
            print_error "未找到requirements.txt文件"
            exit 1
        fi
    else
        print_success "所有依赖包已安装"
    fi
}

# 检查模型文件
check_models() {
    print_info "检查模型文件..."
    
    if [ ! -d "models" ]; then
        print_error "未找到models目录"
        exit 1
    fi
    
    # 检查RKNN模型文件
    RKNN_FILES=(models/*.rknn)
    if [ ! -e "${RKNN_FILES[0]}" ]; then
        print_warning "未找到RKNN模型文件"
        print_info "请将训练好的.rknn模型文件放入models目录"
    else
        print_success "找到RKNN模型文件: ${RKNN_FILES[*]}"
    fi
    
    # 检查标签文件
    if [ ! -f "models/coco_80_labels_list.txt" ]; then
        print_warning "未找到标签文件"
        print_info "请确保models目录中有标签文件"
    else
        print_success "找到标签文件"
    fi
}

# 检查摄像头
check_camera() {
    print_info "检查摄像头设备..."
    
    # 检查/dev/video*设备
    VIDEO_DEVICES=(/dev/video*)
    if [ ! -e "${VIDEO_DEVICES[0]}" ]; then
        print_warning "未找到摄像头设备"
        print_info "请确保摄像头已正确连接"
    else
        print_success "找到摄像头设备: ${VIDEO_DEVICES[*]}"
    fi
}

# 设置环境变量
setup_environment() {
    print_info "设置环境变量..."
    
    # 设置PYTHONPATH
    export PYTHONPATH="$SCRIPT_DIR:$PYTHONPATH"
    
    # 设置Qt相关环境变量（如果需要）
    export QT_QPA_PLATFORM_PLUGIN_PATH=""
    
    # 设置RKNN相关环境变量（如果需要）
    # export RKNN_TOOLKIT_ROOT="/path/to/rknn-toolkit2"
    
    print_success "环境变量设置完成"
}

# 创建必要目录
create_directories() {
    print_info "创建必要目录..."
    
    DIRS=("logs")
    for dir in "${DIRS[@]}"; do
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
            print_info "创建目录: $dir"
        fi
    done
    
    print_success "目录创建完成"
}

# 启动应用程序
start_application() {
    print_info "启动RK3588 YOLOv8实时检测系统..."
    
    # 检查主程序文件
    if [ ! -f "main.py" ]; then
        print_error "未找到main.py文件"
        exit 1
    fi
    
    # 启动应用程序
    $PYTHON_CMD main.py "$@"
}

# 显示帮助信息
show_help() {
    echo "RK3588 YOLOv8实时检测系统启动脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  -c, --check    仅执行环境检查，不启动应用"
    echo "  --no-check     跳过环境检查，直接启动应用"
    echo ""
    echo "示例:"
    echo "  $0              # 执行完整检查并启动应用"
    echo "  $0 --check     # 仅执行环境检查"
    echo "  $0 --no-check  # 跳过检查直接启动"
}

# 主函数
main() {
    echo "========================================"
    echo "RK3588 YOLOv8实时检测系统"
    echo "========================================"
    
    # 解析命令行参数
    SKIP_CHECK=false
    CHECK_ONLY=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--check)
                CHECK_ONLY=true
                shift
                ;;
            --no-check)
                SKIP_CHECK=true
                shift
                ;;
            *)
                # 其他参数传递给Python程序
                break
                ;;
        esac
    done
    
    # 执行环境检查
    if [ "$SKIP_CHECK" = false ]; then
        check_python
        check_dependencies
        check_models
        check_camera
        setup_environment
        create_directories
        
        print_success "环境检查完成"
    fi
    
    # 如果只是检查环境，则退出
    if [ "$CHECK_ONLY" = true ]; then
        print_info "环境检查完成，退出"
        exit 0
    fi
    
    # 启动应用程序
    start_application "$@"
}

# 执行主函数
main "$@"
