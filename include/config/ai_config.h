#ifndef AI_CONFIG_H
#define AI_CONFIG_H

// AI智能决策系统配置参数

// 光照阈值配置
#define AI_LIGHT_OPEN_THRESHOLD   500.0f    // 开帘光照阈值（lux）
#define AI_LIGHT_CLOSE_THRESHOLD  300.0f    // 关帘光照阈值（lux）

// 操作时间配置
#define AI_OPERATION_DURATION     18        // 操作持续时间（秒）
#define AI_DEBOUNCE_INTERVAL      2         // 防抖动间隔（秒）

// 默认状态配置
#define AI_DEFAULT_STATE          false     // 重启默认状态（关闭）

#endif // AI_CONFIG_H
