#!/bin/bash

# 智能温室硬件权限设置脚本 - 开机自动执行
# 统一管理GPIO和PWM权限设置

echo "=== 智能温室硬件权限设置服务启动 ==="

# 等待硬件子系统就绪
sleep 2

# ==================== GPIO权限设置 ====================
echo "设置GPIO权限..."

# 设置GPIO基本权限和组
chgrp gpio /sys/class/gpio/export /sys/class/gpio/unexport 2>/dev/null
chmod 664 /sys/class/gpio/export /sys/class/gpio/unexport 2>/dev/null

# GPIO引脚定义（保温帘控制 + 电源输出 + 水泵控制 + 施药泵控制）
GPIO_PINS=(116 139 99 96 117 138 105 101 110 103 97)

echo "设置GPIO引脚权限..."
for pin in "${GPIO_PINS[@]}"; do
    # 导出引脚
    if [ ! -d "/sys/class/gpio/gpio$pin" ]; then
        echo $pin > /sys/class/gpio/export 2>/dev/null
    fi

    # 等待文件创建
    sleep 0.1

    # 设置gpio组权限
    if [ -f "/sys/class/gpio/gpio$pin/direction" ]; then
        chgrp gpio /sys/class/gpio/gpio$pin/direction 2>/dev/null
        chmod 664 /sys/class/gpio/gpio$pin/direction 2>/dev/null
    fi

    if [ -f "/sys/class/gpio/gpio$pin/value" ]; then
        chgrp gpio /sys/class/gpio/gpio$pin/value 2>/dev/null
        chmod 664 /sys/class/gpio/gpio$pin/value 2>/dev/null
    fi

    echo "GPIO$pin 权限设置完成"
done

# ==================== PWM权限设置 ====================
echo "设置PWM权限..."

# 检查PWM设备
if [ -d "/sys/class/pwm/pwmchip0" ]; then
    # 导出PWM0（如果未导出）
    if [ ! -d "/sys/class/pwm/pwmchip0/pwm0" ]; then
        echo "导出PWM0设备..."
        echo 0 > /sys/class/pwm/pwmchip0/export 2>/dev/null
        sleep 0.2
    fi

    # 设置PWM文件权限
    if [ -d "/sys/class/pwm/pwmchip0/pwm0" ]; then
        chmod 666 /sys/class/pwm/pwmchip0/pwm0/period 2>/dev/null
        chmod 666 /sys/class/pwm/pwmchip0/pwm0/duty_cycle 2>/dev/null
        chmod 666 /sys/class/pwm/pwmchip0/pwm0/enable 2>/dev/null
        chmod 666 /sys/class/pwm/pwmchip0/pwm0/polarity 2>/dev/null

        # 初始化PWM设备（设置极性、周期和启用）
        echo normal > /sys/class/pwm/pwmchip0/pwm0/polarity 2>/dev/null
        echo 1000000 > /sys/class/pwm/pwmchip0/pwm0/period 2>/dev/null
        echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable 2>/dev/null

        echo "PWM0 权限设置和初始化完成"
    else
        echo "警告: PWM0设备导出失败"
    fi
else
    echo "警告: PWM设备不存在，跳过PWM权限设置"
fi

# ==================== I2C权限设置（传感器用） ====================
echo "设置I2C权限..."

# 设置I2C设备权限（AHT20和GY30传感器）
for i2c_dev in /dev/i2c-*; do
    if [ -e "$i2c_dev" ]; then
        chmod 666 "$i2c_dev" 2>/dev/null
        echo "$(basename $i2c_dev) 权限设置完成"
    fi
done

# 特别确保I2C7权限（GY30光照传感器）
if [ -e "/dev/i2c-7" ]; then
    chmod 666 "/dev/i2c-7" 2>/dev/null
    echo "I2C7设备权限设置完成（GY30光照传感器）"
fi

echo "=== 智能温室硬件权限设置完成 ==="

# 输出设置结果摘要
echo ""
echo "权限设置摘要:"
echo "- GPIO引脚: ${#GPIO_PINS[@]}个引脚已设置"
echo "- PWM设备: $([ -d "/sys/class/pwm/pwmchip0/pwm0" ] && echo "已设置" || echo "未找到")"
echo "- I2C设备: $(ls /dev/i2c-* 2>/dev/null | wc -l)个设备已设置（包含I2C7-GY30）"
