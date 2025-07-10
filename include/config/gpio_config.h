#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

// GPIO引脚配置 - 保温帘控制系统
// 基于RK3588平台的GPIO引脚定义

// 上侧帘控制引脚
#define TOP_CURTAIN_DIR1_PIN    116  // gpio3_c4 - 上侧帘方向控制1
#define TOP_CURTAIN_DIR2_PIN    139  // gpio4_b3 - 上侧帘方向控制2
#define TOP_CURTAIN_ENABLE_PIN   99  // gpio3_a3 - 上侧帘使能控制
#define TOP_CURTAIN_ENABLE2_PIN  96  // gpio3_a0 - 上侧帘使能控制2（步进电机驱动板2）

// 侧面帘控制引脚
#define SIDE_CURTAIN_DIR1_PIN   117  // gpio3_c5 - 侧面帘方向控制1
#define SIDE_CURTAIN_DIR2_PIN   138  // gpio4_b2 - 侧面帘方向控制2
#define SIDE_CURTAIN_ENABLE_PIN 105  // gpio3_b1 - 侧面帘使能控制
#define SIDE_CURTAIN_ENABLE2_PIN 101 // gpio3_a5 - 侧面帘使能控制2（步进电机驱动板2）

// 3.3C引脚不够用，GPIO3_B6配置为常高电平
#define POWER_SUPPLY_PIN        110  // gpio3_b6 - 3.3V电源输出

// 水泵控制引脚
#define PUMP_CONTROL_PIN        103  // gpio3_a7 - 水泵控制

// 施药泵控制引脚
#define FERTILIZER_PUMP_PIN      97  // gpio3_a1 - 施药泵控制

// GPIO电平定义
#define GPIO_HIGH  1
#define GPIO_LOW   0

// 使能信号定义
#define CURTAIN_ENABLE   GPIO_LOW   // 低电平使能运行
#define CURTAIN_DISABLE  GPIO_HIGH  // 高电平暂停

// GPIO操作路径
#define GPIO_BASE_PATH "/sys/class/gpio"
#define GPIO_EXPORT_PATH "/sys/class/gpio/export"
#define GPIO_UNEXPORT_PATH "/sys/class/gpio/unexport"

#endif // GPIO_CONFIG_H
