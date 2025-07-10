#ifndef ALIYUN_CONFIG_H
#define ALIYUN_CONFIG_H

/**
 * @brief 阿里云物联网平台配置文件
 *
 * 包含阿里云三元组信息和MQTT连接参数
 * 用户需要根据实际设备信息填写相关配置
 */

// ==================== 阿里云三元组配置 ====================
// 请在阿里云物联网平台创建设备后，将对应信息填入以下宏定义

#define ALIYUN_PRODUCT_KEY    "k1zfks5ATvF"     // 产品密钥
#define ALIYUN_DEVICE_NAME    "rk3588"     // 设备名称
#define ALIYUN_DEVICE_SECRET  "56578f00afd1d6a96acc3df4ceccb18b"   // 设备密钥

// ==================== 阿里云服务器配置 ====================
#define ALIYUN_REGION_ID      "cn-shanghai"               // 区域ID
#define ALIYUN_MQTT_HOST      ALIYUN_PRODUCT_KEY ".iot-as-mqtt." ALIYUN_REGION_ID ".aliyuncs.com"
#define ALIYUN_MQTT_PORT      1883                         // MQTT端口(非SSL)
#define ALIYUN_MQTT_SSL_PORT  8883                         // MQTT SSL端口

// ==================== MQTT主题配置 ====================
// 数据上报主题
#define ALIYUN_TOPIC_POST     "/sys/" ALIYUN_PRODUCT_KEY "/" ALIYUN_DEVICE_NAME "/thing/event/property/post"
// 数据下发主题
#define ALIYUN_TOPIC_SET      "/sys/" ALIYUN_PRODUCT_KEY "/" ALIYUN_DEVICE_NAME "/thing/service/property/set"
// 设备上线主题
#define ALIYUN_TOPIC_ONLINE   "/ext/session/" ALIYUN_PRODUCT_KEY "/" ALIYUN_DEVICE_NAME "/combine/login"
// 设备下线主题
#define ALIYUN_TOPIC_OFFLINE  "/ext/session/" ALIYUN_PRODUCT_KEY "/" ALIYUN_DEVICE_NAME "/combine/logout"

// ==================== 连接参数配置 ====================
#define ALIYUN_KEEP_ALIVE     60                          // 心跳间隔(秒)
#define ALIYUN_CLEAN_SESSION  true                        // 清除会话
#define ALIYUN_QOS_LEVEL      1                           // QoS等级
#define ALIYUN_RETAIN_FLAG    false                       // 保留消息标志

// ==================== 数据上报配置 ====================
#define ALIYUN_REPORT_INTERVAL    10                      // 定时上报间隔(秒)
#define ALIYUN_HEARTBEAT_INTERVAL 300                     // 心跳间隔(秒)
#define ALIYUN_RETRY_COUNT        3                       // 重试次数
#define ALIYUN_TIMEOUT_MS         5000                    // 超时时间(毫秒)

// ==================== SSL/TLS配置 ====================
#define ALIYUN_USE_SSL        false                       // 是否使用SSL连接
#define ALIYUN_CA_CERT_PATH   "/etc/ssl/certs/ca-certificates.crt"  // CA证书路径

// ==================== 调试配置 ====================
#define ALIYUN_DEBUG_ENABLED  true                        // 是否启用调试输出
#define ALIYUN_LOG_LEVEL      2                           // 日志级别(0=ERROR, 1=WARN, 2=INFO, 3=DEBUG)

#endif // ALIYUN_CONFIG_H
