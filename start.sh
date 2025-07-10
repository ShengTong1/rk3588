#!/bin/bash

# 智能温室大棚控制系统启动脚本 (开机自启版本)
# 集成PWM和GPIO权限设置，自动启动应用

# 日志记录函数
log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/qt-greenhouse.log
}

log_message "=== 基于RK3588的智能温室大棚控制系统启动 ==="

# 等待图形界面完全启动
log_message "等待图形界面启动..."
for i in {1..30}; do
    if [ -n "$DISPLAY" ] && xset q >/dev/null 2>&1; then
        log_message "图形界面已就绪 (尝试 $i/30)"
        break
    fi
    sleep 1
done

# 系统初始化延时
log_message "系统初始化中，请稍候..."

# ==================== 硬件权限检查 ====================
log_message "检查硬件权限设置..."

# 检查统一硬件权限服务是否运行
if systemctl is-active --quiet hardware-setup.service; then
    log_message "✅ 硬件权限服务已运行（GPIO + PWM + I2C）"
else
    log_message "⚠️  硬件权限服务未运行，手动设置权限..."

    # ==================== GPIO权限设置 ====================
    log_message "设置GPIO权限..."

    # 设置GPIO基本权限和组
    sudo chgrp gpio /sys/class/gpio/export /sys/class/gpio/unexport 2>/dev/null
    sudo chmod 664 /sys/class/gpio/export /sys/class/gpio/unexport 2>/dev/null

    # GPIO引脚定义（保温帘控制 + 电源输出 + 水泵控制）
    GPIO_PINS=(116 139 99 117 138 105 110 103)

    for pin in "${GPIO_PINS[@]}"; do
        # 导出引脚（如果尚未导出）
        if [ ! -d "/sys/class/gpio/gpio$pin" ]; then
            echo $pin | sudo tee /sys/class/gpio/export > /dev/null 2>&1
        fi

        # 等待文件创建
        sleep 0.1

        # 设置gpio组权限
        if [ -f "/sys/class/gpio/gpio$pin/direction" ]; then
            sudo chgrp gpio /sys/class/gpio/gpio$pin/direction 2>/dev/null
            sudo chmod 664 /sys/class/gpio/gpio$pin/direction 2>/dev/null
        fi

        if [ -f "/sys/class/gpio/gpio$pin/value" ]; then
            sudo chgrp gpio /sys/class/gpio/gpio$pin/value 2>/dev/null
            sudo chmod 664 /sys/class/gpio/gpio$pin/value 2>/dev/null
        fi
    done

    # ==================== PWM权限设置 ====================
    log_message "设置PWM权限..."

    # 检查PWM设备
    if [ ! -d "/sys/class/pwm/pwmchip0" ]; then
        log_message "错误: PWM设备不存在，请检查硬件配置"
        exit 1
    fi

    # 导出PWM0（如果未导出）
    if [ ! -d "/sys/class/pwm/pwmchip0/pwm0" ]; then
        log_message "导出PWM0设备..."
        echo 0 | sudo tee /sys/class/pwm/pwmchip0/export > /dev/null 2>&1
        sleep 0.1
    fi

    # 设置PWM文件权限
    sudo chmod 666 /sys/class/pwm/pwmchip0/pwm0/period 2>/dev/null
    sudo chmod 666 /sys/class/pwm/pwmchip0/pwm0/duty_cycle 2>/dev/null
    sudo chmod 666 /sys/class/pwm/pwmchip0/pwm0/enable 2>/dev/null
    sudo chmod 666 /sys/class/pwm/pwmchip0/pwm0/polarity 2>/dev/null

    # 初始化PWM设备（设置极性、周期和启用）
    echo normal > /sys/class/pwm/pwmchip0/pwm0/polarity 2>/dev/null
    echo 1000000 > /sys/class/pwm/pwmchip0/pwm0/period 2>/dev/null
    echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable 2>/dev/null

    # ==================== I2C权限设置 ====================
    log_message "设置I2C权限..."

    # 设置I2C设备权限（AHT20和GY30传感器）
    for i2c_dev in /dev/i2c-*; do
        if [ -e "$i2c_dev" ]; then
            sudo chmod 666 "$i2c_dev" 2>/dev/null
        fi
    done

    # 特别确保I2C7权限（GY30光照传感器）
    if [ -e "/dev/i2c-7" ]; then
        sudo chmod 666 "/dev/i2c-7" 2>/dev/null
        log_message "I2C7设备权限设置完成（GY30光照传感器）"
    fi


fi

log_message "✅ 设备初始化完成"

# ==================== 启动应用 ====================
log_message "启动Qt应用..."

# ==================== 时区设置 ====================
log_message "设置北京时间时区..."

# 设置系统时区为北京时间
export TZ='Asia/Shanghai'
export LC_TIME='zh_CN.UTF-8'

# 确保时区设置生效
log_message "当前系统时间: $(date '+%Y-%m-%d %H:%M:%S %Z')"

# 设置Qt应用环境变量
export QT_QPA_PLATFORM=xcb
export QT_AUTO_SCREEN_SCALE_FACTOR=0
export QT_SCALE_FACTOR=1

# 启动主程序并自动最大化
log_message "启动智能温室大棚控制系统..."
./wonderfulnewworld &
APP_PID=$!

# 等待应用窗口出现并最大化（非全屏）
sleep 3
if [ -n "$DISPLAY" ] && command -v xdotool >/dev/null 2>&1; then
    # 查找应用窗口并最大化
    for i in {1..15}; do
        WINDOW_ID=$(xdotool search --name "智能温室大棚控制系统" 2>/dev/null | head -1)
        if [ -n "$WINDOW_ID" ]; then
            log_message "找到应用窗口，等待窗口稳定..."
            # 等待窗口完全加载和稳定
            sleep 2

            # 激活窗口
            xdotool windowactivate "$WINDOW_ID" 2>/dev/null
            sleep 0.5

            # 获取屏幕尺寸
            SCREEN_GEOMETRY=$(xdotool getdisplaygeometry)
            SCREEN_WIDTH=$(echo "$SCREEN_GEOMETRY" | awk '{print $1}')
            SCREEN_HEIGHT=$(echo "$SCREEN_GEOMETRY" | awk '{print $2}')

            # 计算最大化尺寸（保留边距）
            MAX_WIDTH=$((SCREEN_WIDTH - 20))
            MAX_HEIGHT=$((SCREEN_HEIGHT - 60))

            log_message "执行最大化（保留标题栏）: ${MAX_WIDTH}x${MAX_HEIGHT}..."

            # 先设置窗口大小和位置
            xdotool windowsize "$WINDOW_ID" "$MAX_WIDTH" "$MAX_HEIGHT" 2>/dev/null
            sleep 0.3
            xdotool windowmove "$WINDOW_ID" 10 30 2>/dev/null
            sleep 0.3

            # 确保窗口保持最大化状态
            xdotool windowactivate "$WINDOW_ID" 2>/dev/null

            log_message "窗口最大化完成"
            break
        fi
        sleep 1
    done
fi

# 等待应用进程结束
wait $APP_PID
log_message "应用已退出"

# ==================== 清理资源 ====================
log_message "清理系统资源..."

# 清理GPIO引脚（如果不是systemd服务管理的）
if ! systemctl is-active --quiet hardware-setup.service; then
    GPIO_PINS=(116 139 99 117 138 105 110 103)
    for pin in "${GPIO_PINS[@]}"; do
        if [ -d "/sys/class/gpio/gpio$pin" ]; then
            echo $pin | sudo tee /sys/class/gpio/unexport > /dev/null 2>&1
        fi
    done
fi

log_message "系统资源清理完成"
