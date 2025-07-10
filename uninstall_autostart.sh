#!/bin/bash

# 智能温室大棚控制系统开机自启卸载脚本

echo "=== 智能温室大棚控制系统开机自启卸载 ==="

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then
    echo "请使用sudo运行此脚本"
    exit 1
fi

SERVICE_NAME="qt-greenhouse-control"

# 1. 停止服务
echo "停止服务..."
systemctl stop "${SERVICE_NAME}.service" 2>/dev/null

# 2. 禁用服务
echo "禁用开机自启动..."
systemctl disable "${SERVICE_NAME}.service" 2>/dev/null

# 3. 删除服务文件
echo "删除服务文件..."
rm -f "/etc/systemd/system/${SERVICE_NAME}.service"

# 4. 重新加载systemd配置
echo "重新加载systemd配置..."
systemctl daemon-reload
systemctl reset-failed

echo ""
echo "=== 卸载完成 ==="
echo "智能温室大棚控制系统开机自启动已禁用"
echo "如需重新启用，请运行: sudo ./install_autostart.sh"
