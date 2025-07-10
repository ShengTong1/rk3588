#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTcpSocket;
class QSslSocket;
QT_END_NAMESPACE

/**
 * @brief 阿里云MQTT服务类
 *
 * 实现与阿里云物联网平台的MQTT通信功能
 * 支持设备数据上报和云端指令下发
 */
class MqttService : public QObject
{
    Q_OBJECT

public:
    // 连接状态枚举
    enum ConnectionState {
        Disconnected = 0,   // 已断开
        Connecting,         // 连接中
        Connected,          // 已连接
        Reconnecting        // 重连中
    };

    // 设备数据结构
    struct DeviceData {
        double temperature;     // 温度(°C)
        double humidity;        // 湿度(%)
        double lightIntensity;  // 光照强度(lux)
        int pwmDutyCycle;      // PWM占空比(%)
        bool curtainTopOpen;   // 顶部保温帘状态
        bool curtainSideOpen;  // 侧部保温帘状态
        QString timestamp;     // 时间戳
        bool isValid;          // 数据有效性

        DeviceData() : temperature(0), humidity(0), lightIntensity(0),
                      pwmDutyCycle(0), curtainTopOpen(false), curtainSideOpen(false),
                      isValid(false) {}
    };

    // 控制指令结构
    struct ControlCommand {
        QString commandType;    // 指令类型
        QJsonObject parameters; // 指令参数
        QString messageId;      // 消息ID
        QString timestamp;      // 时间戳
        bool isValid;          // 指令有效性

        ControlCommand() : isValid(false) {}
    };

    explicit MqttService(QObject *parent = nullptr);
    ~MqttService();

    // 主要功能接口
    bool connectToAliyun();                              // 连接阿里云
    void disconnectFromAliyun();                         // 断开连接
    bool publishDeviceData(const DeviceData &data);     // 发布设备数据
    bool publishHeartbeat();                             // 发布心跳

    // 状态查询
    ConnectionState getConnectionState() const { return m_connectionState; }
    bool isConnected() const { return m_connectionState == Connected; }
    QString getLastError() const { return m_lastError; }

    // 配置接口
    void setAutoReconnect(bool enabled) { m_autoReconnect = enabled; }
    void setReportInterval(int seconds);                 // 设置上报间隔
    void setHeartbeatInterval(int seconds);              // 设置心跳间隔

signals:
    void connectionStateChanged(ConnectionState state);  // 连接状态变化
    void deviceDataPublished(bool success);             // 数据发布结果
    void controlCommandReceived(const ControlCommand &cmd); // 收到控制指令
    void soilHumidityReceived(double humidity);          // 收到土壤湿度数据
    void errorOccurred(const QString &error);           // 错误发生
    void heartbeatSent();                               // 心跳发送
    void dataCollectionRequested();                     // 请求收集设备数据

private slots:
    void onSocketConnected();                           // Socket连接成功
    void onSocketDisconnected();                        // Socket断开连接
    void onSocketError();                               // Socket错误
    void onSocketReadyRead();                           // Socket数据就绪
    void onReportTimer();                               // 定时上报
    void onHeartbeatTimer();                            // 心跳定时器
    void onReconnectTimer();                            // 重连定时器

private:
    // 网络组件
    QTcpSocket *m_socket;                               // TCP Socket
    QSslSocket *m_sslSocket;                            // SSL Socket
    QTimer *m_reportTimer;                              // 上报定时器
    QTimer *m_heartbeatTimer;                           // 心跳定时器
    QTimer *m_reconnectTimer;                           // 重连定时器

    // 连接状态
    ConnectionState m_connectionState;                   // 连接状态
    QString m_lastError;                                // 最后错误信息
    bool m_autoReconnect;                               // 自动重连
    int m_reconnectCount;                               // 重连次数
    int m_maxReconnectCount;                            // 最大重连次数

    // 配置参数
    int m_reportInterval;                               // 上报间隔(秒)
    int m_heartbeatInterval;                            // 心跳间隔(秒)

    // MQTT协议相关
    QString m_clientId;                                 // 客户端ID
    QString m_username;                                 // 用户名
    QString m_password;                                 // 密码
    quint16 m_packetId;                                 // 包ID计数器
    QByteArray m_receiveBuffer;                         // 接收缓冲区

    // 内部功能函数
    void initializeConnection();                        // 初始化连接
    void generateMqttCredentials();                     // 生成MQTT认证信息
    QString calculateHmacSha1(const QString &key, const QString &data); // 计算HMAC-SHA1

    // MQTT协议实现
    QByteArray buildConnectPacket();                    // 构建连接包
    QByteArray buildPublishPacket(const QString &topic, const QByteArray &payload, quint8 qos = 1); // 构建发布包
    QByteArray buildSubscribePacket(const QString &topic, quint8 qos = 1); // 构建订阅包
    QByteArray buildPingReqPacket();                    // 构建心跳包
    QByteArray buildDisconnectPacket();                 // 构建断开包

    // 数据处理
    void processReceivedData();                         // 处理接收数据
    void handleConnAck(const QByteArray &data);         // 处理连接确认
    void handlePublish(const QByteArray &data);         // 处理发布消息
    void handlePubAck(const QByteArray &data);          // 处理发布确认
    void handleSubAck(const QByteArray &data);          // 处理订阅确认
    void handlePingResp(const QByteArray &data);        // 处理心跳响应

    // JSON数据处理
    QJsonObject deviceDataToJson(const DeviceData &data); // 设备数据转JSON
    ControlCommand parseControlCommand(const QJsonObject &json); // 解析控制指令

    // 工具函数
    void setState(ConnectionState state);               // 设置连接状态
    void setError(const QString &error);                // 设置错误信息
    void startReconnectTimer();                         // 启动重连定时器
    quint16 getNextPacketId() { return ++m_packetId; }  // 获取下一个包ID

    // 编码工具
    QByteArray encodeString(const QString &str);        // 编码字符串
    QByteArray encodeLength(quint32 length);            // 编码长度
    quint32 decodeLength(const QByteArray &data, int &offset); // 解码长度
    QString decodeString(const QByteArray &data, int &offset); // 解码字符串
};

#endif // MQTT_SERVICE_H
