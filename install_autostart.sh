#!/bin/bash

# 智能温室大棚控制系统开机自启安装脚本
# 设置qt_mainwindow项目开机自启动

echo "=== 智能温室大棚控制系统开机自启安装 ==="

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then
    echo "请使用sudo运行此脚本"
    exit 1
fi

# 获取当前脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_NAME="qt-greenhouse-control"

echo "当前工作目录: $SCRIPT_DIR"

# 1. 创建日志目录
echo "创建日志目录..."
mkdir -p /var/log
touch /var/log/qt-greenhouse.log
chown elf:elf /var/log/qt-greenhouse.log
chmod 644 /var/log/qt-greenhouse.log

# 2. 确保start.sh可执行
echo "设置start.sh权限..."
chmod +x "$SCRIPT_DIR/start.sh"

# 3. 安装systemd服务文件
echo "安装systemd服务文件..."
cp "$SCRIPT_DIR/${SERVICE_NAME}.service" "/etc/systemd/system/"
chmod 644 "/etc/systemd/system/${SERVICE_NAME}.service"

# 4. 重新加载systemd配置
echo "重新加载systemd配置..."
systemctl daemon-reload

# 5. 启用服务
echo "启用开机自启动服务..."
systemctl enable "${SERVICE_NAME}.service"

# 6. 检查硬件权限服务状态
echo "检查硬件权限服务..."
if [ -f "/etc/systemd/system/hardware-setup.service" ]; then
    systemctl enable hardware-setup.service
    echo "✅ 硬件权限服务已启用"
else
    echo "⚠️  硬件权限服务不存在，将使用脚本内置权限设置"
fi

# 7. 安装必要的工具
echo "检查并安装必要工具..."
if ! command -v xdotool >/dev/null 2>&1; then
    echo "安装xdotool..."
    apt-get update && apt-get install -y xdotool
fi

# 8. 设置用户组权限
echo "设置用户组权限..."
usermod -a -G gpio elf 2>/dev/null || echo "gpio组不存在，跳过"
usermod -a -G dialout elf 2>/dev/null || echo "dialout组不存在，跳过"

# 9. 显示服务状态
echo ""
echo "=== 安装完成 ==="
echo "服务名称: ${SERVICE_NAME}"
echo "服务状态:"
systemctl status "${SERVICE_NAME}.service" --no-pager -l

echo ""
echo "=== 使用说明 ==="
echo "启动服务: sudo systemctl start ${SERVICE_NAME}"
echo "停止服务: sudo systemctl stop ${SERVICE_NAME}"
echo "重启服务: sudo systemctl restart ${SERVICE_NAME}"
echo "查看状态: sudo systemctl status ${SERVICE_NAME}"
echo "查看日志: sudo journalctl -u ${SERVICE_NAME} -f"
echo "查看应用日志: tail -f /var/log/qt-greenhouse.log"
echo "禁用自启: sudo systemctl disable ${SERVICE_NAME}"
echo ""
echo "系统重启后将自动启动智能温室大棚控制系统"
echo "建议重启系统测试自启动功能: sudo reboot"
