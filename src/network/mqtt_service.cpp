#include "network/mqtt_service.h"
#include "config/aliyun_config.h"

#include <QTcpSocket>
#include <QSslSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QDebug>
#include <QRandomGenerator>

MqttService::MqttService(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_sslSocket(nullptr)
    , m_reportTimer(new QTimer(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_connectionState(Disconnected)
    , m_autoReconnect(true)
    , m_reconnectCount(0)
    , m_maxReconnectCount(ALIYUN_RETRY_COUNT)
    , m_reportInterval(ALIYUN_REPORT_INTERVAL)
    , m_heartbeatInterval(ALIYUN_HEARTBEAT_INTERVAL)
    , m_packetId(0)
{
    // 初始化定时器
    m_reportTimer->setSingleShot(false);
    m_heartbeatTimer->setSingleShot(false);
    m_reconnectTimer->setSingleShot(true);

    // 连接定时器信号
    connect(m_reportTimer, &QTimer::timeout, this, &MqttService::onReportTimer);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &MqttService::onHeartbeatTimer);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_autoReconnect && m_connectionState == Disconnected) {
            connectToAliyun();
        }
    });

    // 生成MQTT认证信息
    generateMqttCredentials();

    qDebug() << "MQTT服务初始化完成";
}

MqttService::~MqttService()
{
    disconnectFromAliyun();
    qDebug() << "MQTT服务已销毁";
}

bool MqttService::connectToAliyun()
{
    if (m_connectionState == Connected || m_connectionState == Connecting) {
        qDebug() << "MQTT已连接或正在连接中";
        return true;
    }

    setState(Connecting);

    // 创建Socket连接
    if (ALIYUN_USE_SSL) {
        if (!m_sslSocket) {
            m_sslSocket = new QSslSocket(this);
            connect(m_sslSocket, &QSslSocket::connected, this, &MqttService::onSocketConnected);
            connect(m_sslSocket, &QSslSocket::disconnected, this, &MqttService::onSocketDisconnected);
            connect(m_sslSocket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
                    [this](const QList<QSslError> &errors) {
                        for (const auto &error : errors) {
                            qWarning() << "SSL错误:" << error.errorString();
                        }
                        setError("SSL连接错误");
                    });
            connect(m_sslSocket, &QSslSocket::readyRead, this, &MqttService::onSocketReadyRead);
        }

        qDebug() << QString("连接阿里云MQTT服务器(SSL): %1:%2").arg(ALIYUN_MQTT_HOST).arg(ALIYUN_MQTT_SSL_PORT);
        m_sslSocket->connectToHostEncrypted(ALIYUN_MQTT_HOST, ALIYUN_MQTT_SSL_PORT);
    } else {
        if (!m_socket) {
            m_socket = new QTcpSocket(this);
            connect(m_socket, &QTcpSocket::connected, this, &MqttService::onSocketConnected);
            connect(m_socket, &QTcpSocket::disconnected, this, &MqttService::onSocketDisconnected);
            connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
                    this, &MqttService::onSocketError);
            connect(m_socket, &QTcpSocket::readyRead, this, &MqttService::onSocketReadyRead);
        }

        qDebug() << QString("连接阿里云MQTT服务器: %1:%2").arg(ALIYUN_MQTT_HOST).arg(ALIYUN_MQTT_PORT);
        m_socket->connectToHost(ALIYUN_MQTT_HOST, ALIYUN_MQTT_PORT);
    }

    return true;
}

void MqttService::disconnectFromAliyun()
{
    if (m_connectionState == Disconnected) {
        return;
    }

    // 停止定时器
    m_reportTimer->stop();
    m_heartbeatTimer->stop();
    m_reconnectTimer->stop();

    // 发送断开包
    if (m_connectionState == Connected) {
        QByteArray disconnectPacket = buildDisconnectPacket();
        if (ALIYUN_USE_SSL && m_sslSocket) {
            m_sslSocket->write(disconnectPacket);
            m_sslSocket->flush();
        } else if (m_socket) {
            m_socket->write(disconnectPacket);
            m_socket->flush();
        }
    }

    // 关闭Socket连接
    if (ALIYUN_USE_SSL && m_sslSocket) {
        m_sslSocket->disconnectFromHost();
    } else if (m_socket) {
        m_socket->disconnectFromHost();
    }

    setState(Disconnected);
    qDebug() << "MQTT连接已断开";
}

bool MqttService::publishDeviceData(const DeviceData &data)
{
    if (m_connectionState != Connected) {
        setError("MQTT未连接，无法发布数据");
        return false;
    }

    if (!data.isValid) {
        setError("设备数据无效");
        return false;
    }

    // 构建阿里云标准数据格式
    QJsonObject payload = deviceDataToJson(data);
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 发布到阿里云数据上报主题
    QByteArray publishPacket = buildPublishPacket(ALIYUN_TOPIC_POST, jsonData, ALIYUN_QOS_LEVEL);

    bool success = false;
    if (ALIYUN_USE_SSL && m_sslSocket) {
        success = (m_sslSocket->write(publishPacket) == publishPacket.size());
    } else if (m_socket) {
        success = (m_socket->write(publishPacket) == publishPacket.size());
    }

    if (success) {
        emit deviceDataPublished(true);
    } else {
        setError("数据发布失败");
        emit deviceDataPublished(false);
    }

    return success;
}

bool MqttService::publishHeartbeat()
{
    if (m_connectionState != Connected) {
        return false;
    }

    QByteArray pingPacket = buildPingReqPacket();

    bool success = false;
    if (ALIYUN_USE_SSL && m_sslSocket) {
        success = (m_sslSocket->write(pingPacket) == pingPacket.size());
    } else if (m_socket) {
        success = (m_socket->write(pingPacket) == pingPacket.size());
    }

    if (success) {
        emit heartbeatSent();
    }

    return success;
}

void MqttService::setReportInterval(int seconds)
{
    m_reportInterval = seconds;
    if (m_reportTimer->isActive()) {
        m_reportTimer->start(seconds * 1000);
    }
    qDebug() << "数据上报间隔设置为:" << seconds << "秒";
}

void MqttService::setHeartbeatInterval(int seconds)
{
    m_heartbeatInterval = seconds;
    if (m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start(seconds * 1000);
    }
    qDebug() << "心跳间隔设置为:" << seconds << "秒";
}

void MqttService::onSocketConnected()
{
    qDebug() << "Socket连接成功，发送MQTT连接包";

    // 发送MQTT连接包
    QByteArray connectPacket = buildConnectPacket();

    if (ALIYUN_USE_SSL && m_sslSocket) {
        m_sslSocket->write(connectPacket);
    } else if (m_socket) {
        m_socket->write(connectPacket);
    }
}

void MqttService::onSocketDisconnected()
{
    // 停止定时器
    m_reportTimer->stop();
    m_heartbeatTimer->stop();

    setState(Disconnected);

    // 启动重连定时器
    if (m_autoReconnect && m_reconnectCount < m_maxReconnectCount) {
        m_reconnectCount++;
        int delay = qMin(30000, 1000 * m_reconnectCount); // 最大30秒延时
        m_reconnectTimer->start(delay);
        setState(Reconnecting);
    }
}

void MqttService::onSocketError()
{
    QString error;
    if (ALIYUN_USE_SSL && m_sslSocket) {
        error = m_sslSocket->errorString();
    } else if (m_socket) {
        error = m_socket->errorString();
    }

    qWarning() << "Socket错误:" << error;
    setError(error);
    setState(Disconnected);
}

void MqttService::onSocketReadyRead()
{
    QByteArray data;
    if (ALIYUN_USE_SSL && m_sslSocket) {
        data = m_sslSocket->readAll();
    } else if (m_socket) {
        data = m_socket->readAll();
    }

    m_receiveBuffer.append(data);
    processReceivedData();
}

void MqttService::onReportTimer()
{
    // 发出数据收集请求信号，由主窗口响应并收集实际设备数据
    emit dataCollectionRequested();
}

void MqttService::onHeartbeatTimer()
{
    publishHeartbeat();
}

void MqttService::onReconnectTimer()
{
    if (m_autoReconnect && m_connectionState == Disconnected) {
        qDebug() << QString("执行重连(第%1次)").arg(m_reconnectCount);
        connectToAliyun();
    }
}

void MqttService::generateMqttCredentials()
{
    // 生成客户端ID: clientId + "|securemode=3,signmethod=hmacsha1|"
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    m_clientId = QString("%1|securemode=3,signmethod=hmacsha1,timestamp=%2|")
                 .arg(ALIYUN_DEVICE_NAME)
                 .arg(timestamp);

    // 生成用户名: deviceName&productKey
    m_username = QString("%1&%2").arg(ALIYUN_DEVICE_NAME).arg(ALIYUN_PRODUCT_KEY);

    // 生成密码: HMAC-SHA1签名
    QString signContent = QString("clientId%1deviceName%2productKey%3timestamp%4")
                         .arg(ALIYUN_DEVICE_NAME)
                         .arg(ALIYUN_DEVICE_NAME)
                         .arg(ALIYUN_PRODUCT_KEY)
                         .arg(timestamp);

    m_password = calculateHmacSha1(ALIYUN_DEVICE_SECRET, signContent);

    qDebug() << "MQTT认证信息生成完成";
    qDebug() << "ClientID:" << m_clientId;
    qDebug() << "Username:" << m_username;
}

QString MqttService::calculateHmacSha1(const QString &key, const QString &data)
{
    QByteArray keyBytes = key.toUtf8();
    QByteArray dataBytes = data.toUtf8();

    QByteArray hash = QMessageAuthenticationCode::hash(dataBytes, keyBytes, QCryptographicHash::Sha1);
    return hash.toHex();
}

QByteArray MqttService::buildConnectPacket()
{
    QByteArray packet;

    // 固定头部
    packet.append(static_cast<char>(0x10)); // CONNECT消息类型

    // 可变头部
    QByteArray variableHeader;

    // 协议名称
    variableHeader.append(encodeString("MQTT"));

    // 协议级别
    variableHeader.append(static_cast<char>(0x04));

    // 连接标志
    quint8 connectFlags = 0x02; // Clean Session
    if (!m_username.isEmpty()) {
        connectFlags |= 0x80; // User Name Flag
    }
    if (!m_password.isEmpty()) {
        connectFlags |= 0x40; // Password Flag
    }
    variableHeader.append(connectFlags);

    // 保持连接时间
    variableHeader.append(static_cast<char>((ALIYUN_KEEP_ALIVE >> 8) & 0xFF));
    variableHeader.append(static_cast<char>(ALIYUN_KEEP_ALIVE & 0xFF));

    // 载荷
    QByteArray payload;
    payload.append(encodeString(m_clientId));
    if (!m_username.isEmpty()) {
        payload.append(encodeString(m_username));
    }
    if (!m_password.isEmpty()) {
        payload.append(encodeString(m_password));
    }

    // 计算剩余长度
    quint32 remainingLength = variableHeader.size() + payload.size();
    packet.append(encodeLength(remainingLength));

    // 添加可变头部和载荷
    packet.append(variableHeader);
    packet.append(payload);

    return packet;
}

QByteArray MqttService::buildPublishPacket(const QString &topic, const QByteArray &payload, quint8 qos)
{
    QByteArray packet;

    // 固定头部
    quint8 fixedHeader = 0x30; // PUBLISH消息类型
    if (qos == 1) {
        fixedHeader |= 0x02; // QoS = 1
    } else if (qos == 2) {
        fixedHeader |= 0x04; // QoS = 2
    }
    if (ALIYUN_RETAIN_FLAG) {
        fixedHeader |= 0x01; // Retain
    }
    packet.append(fixedHeader);

    // 可变头部
    QByteArray variableHeader;
    variableHeader.append(encodeString(topic));

    // 如果QoS > 0，添加包标识符
    if (qos > 0) {
        quint16 packetId = getNextPacketId();
        variableHeader.append(static_cast<char>((packetId >> 8) & 0xFF));
        variableHeader.append(static_cast<char>(packetId & 0xFF));
    }

    // 计算剩余长度
    quint32 remainingLength = variableHeader.size() + payload.size();
    packet.append(encodeLength(remainingLength));

    // 添加可变头部和载荷
    packet.append(variableHeader);
    packet.append(payload);

    return packet;
}

QByteArray MqttService::buildSubscribePacket(const QString &topic, quint8 qos)
{
    QByteArray packet;

    // 固定头部
    packet.append(static_cast<char>(0x82)); // SUBSCRIBE消息类型

    // 可变头部
    QByteArray variableHeader;
    quint16 packetId = getNextPacketId();
    variableHeader.append(static_cast<char>((packetId >> 8) & 0xFF));
    variableHeader.append(static_cast<char>(packetId & 0xFF));

    // 载荷
    QByteArray payload;
    payload.append(encodeString(topic));
    payload.append(static_cast<char>(qos));

    // 计算剩余长度
    quint32 remainingLength = variableHeader.size() + payload.size();
    packet.append(encodeLength(remainingLength));

    // 添加可变头部和载荷
    packet.append(variableHeader);
    packet.append(payload);

    return packet;
}

QByteArray MqttService::buildPingReqPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(0xC0)); // PINGREQ消息类型
    packet.append(static_cast<char>(0x00)); // 剩余长度为0
    return packet;
}

QByteArray MqttService::buildDisconnectPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(0xE0)); // DISCONNECT消息类型
    packet.append(static_cast<char>(0x00)); // 剩余长度为0
    return packet;
}

void MqttService::processReceivedData()
{
    while (m_receiveBuffer.size() >= 2) {
        // 读取固定头部
        quint8 messageType = m_receiveBuffer.at(0);

        // 解码剩余长度
        int offset = 1;
        quint32 remainingLength = decodeLength(m_receiveBuffer, offset);

        // 检查是否有完整的消息
        if (static_cast<quint32>(m_receiveBuffer.size()) < offset + remainingLength) {
            break; // 等待更多数据
        }

        // 提取消息数据
        QByteArray messageData = m_receiveBuffer.mid(offset, remainingLength);
        m_receiveBuffer.remove(0, offset + remainingLength);

        // 处理不同类型的消息
        switch (messageType & 0xF0) {
        case 0x20: // CONNACK
            handleConnAck(messageData);
            break;
        case 0x30: // PUBLISH
            handlePublish(messageData);
            break;
        case 0x40: // PUBACK
            handlePubAck(messageData);
            break;
        case 0x90: // SUBACK
            handleSubAck(messageData);
            break;
        case 0xD0: // PINGRESP
            handlePingResp(messageData);
            break;
        default:
            qDebug() << "收到未知MQTT消息类型:" << QString::number(messageType, 16);
            break;
        }
    }
}

void MqttService::handleConnAck(const QByteArray &data)
{
    if (data.size() < 2) {
        setError("CONNACK消息格式错误");
        return;
    }

    quint8 returnCode = data.at(1);
    if (returnCode == 0) {
        setState(Connected);
        m_reconnectCount = 0; // 重置重连计数

        // 订阅下行数据主题
        QByteArray subscribePacket = buildSubscribePacket(ALIYUN_TOPIC_SET, ALIYUN_QOS_LEVEL);
        if (ALIYUN_USE_SSL && m_sslSocket) {
            m_sslSocket->write(subscribePacket);
        } else if (m_socket) {
            m_socket->write(subscribePacket);
        }

        // 启动定时器
        m_reportTimer->start(m_reportInterval * 1000);
        m_heartbeatTimer->start(m_heartbeatInterval * 1000);

    } else {
        QString error = QString("MQTT连接失败，返回码: %1").arg(returnCode);
        setError(error);
        setState(Disconnected);
    }
}

void MqttService::handlePublish(const QByteArray &data)
{
    int offset = 0;

    // 解析主题
    QString topic = decodeString(data, offset);

    // 解析载荷
    QByteArray payload = data.mid(offset);

    // 清理载荷数据，查找JSON开始位置
    QByteArray cleanPayload = payload;
    int jsonStart = cleanPayload.indexOf('{');
    if (jsonStart > 0) {
        cleanPayload = cleanPayload.mid(jsonStart);
    } else if (jsonStart < 0) {
        qWarning() << "载荷中未找到JSON数据";
        return;
    }

    // 解析JSON数据
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(cleanPayload, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析失败:" << error.errorString();
        return;
    }

    QJsonObject json = doc.object();

    // MQTT消息接收成功

    // 检查阿里云云产品流转格式的数据
    if (json.contains("items")) {
        QJsonObject items = json["items"].toObject();

        // 检查是否包含土壤湿度数据
        if (items.contains("soilHumidity")) {
            QJsonObject soilHumidityObj = items["soilHumidity"].toObject();
            if (soilHumidityObj.contains("value")) {
                double soilHumidity = soilHumidityObj["value"].toDouble();
                // 土壤湿度数据接收成功
                emit soilHumidityReceived(soilHumidity);
                return; // 找到数据后直接返回
            }
        }
    }

    // 检查阿里云物模型格式的数据
    if (json.contains("params")) {
        QJsonObject params = json["params"].toObject();

        // 检查是否包含土壤湿度数据
        if (params.contains("soilHumidity")) {
            double soilHumidity = params["soilHumidity"].toDouble();
            // 土壤湿度数据接收成功(params格式)
            emit soilHumidityReceived(soilHumidity);
            return; // 找到数据后直接返回
        }
    }

    // 也检查直接格式的土壤湿度数据（兼容性）
    if (json.contains("soilHumidity")) {
        double soilHumidity = json["soilHumidity"].toDouble();
        // 土壤湿度数据接收成功(直接格式)
        emit soilHumidityReceived(soilHumidity);
        return; // 找到数据后直接返回
    }

    // 处理控制指令
    ControlCommand cmd = parseControlCommand(json);
    if (cmd.isValid) {
        emit controlCommandReceived(cmd);
    }
}

void MqttService::handlePubAck(const QByteArray &data)
{
    Q_UNUSED(data)
    qDebug() << "收到PUBACK确认";
}

void MqttService::handleSubAck(const QByteArray &data)
{
    Q_UNUSED(data)
    qDebug() << "主题订阅成功";
}

void MqttService::handlePingResp(const QByteArray &data)
{
    Q_UNUSED(data)
    qDebug() << "收到心跳响应";
}

QJsonObject MqttService::deviceDataToJson(const DeviceData &data)
{
    QJsonObject root;
    root["id"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    root["version"] = "1.0";
    root["method"] = "thing.event.property.post";

    QJsonObject params;
    params["temperature"] = data.temperature;              // 温度
    params["Humidity"] = data.humidity;                    // 湿度
    params["LightLux"] = static_cast<int>(data.lightIntensity);  // 光照值(整数)
    params["pwm"] = data.pwmDutyCycle;                     // PWM占空比(整数)
    // 移除阿里云物模型中未定义的属性
    // params["curtainTopOpen"] = data.curtainTopOpen;
    // params["curtainSideOpen"] = data.curtainSideOpen;
    // params["timestamp"] = data.timestamp;

    root["params"] = params;

    return root;
}

MqttService::ControlCommand MqttService::parseControlCommand(const QJsonObject &json)
{
    ControlCommand cmd;

    if (json.contains("method") && json.contains("params")) {
        cmd.commandType = json["method"].toString();
        cmd.parameters = json["params"].toObject();
        cmd.messageId = json["id"].toString();
        cmd.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        cmd.isValid = true;
    }

    return cmd;
}

void MqttService::setState(ConnectionState state)
{
    if (m_connectionState != state) {
        m_connectionState = state;
        emit connectionStateChanged(state);
    }
}

void MqttService::setError(const QString &error)
{
    m_lastError = error;
    emit errorOccurred(error);
    qWarning() << "MQTT错误:" << error;
}

QByteArray MqttService::encodeString(const QString &str)
{
    QByteArray data;
    QByteArray utf8 = str.toUtf8();
    data.append(static_cast<char>((utf8.size() >> 8) & 0xFF));
    data.append(static_cast<char>(utf8.size() & 0xFF));
    data.append(utf8);
    return data;
}

QByteArray MqttService::encodeLength(quint32 length)
{
    QByteArray data;
    do {
        quint8 byte = length % 128;
        length /= 128;
        if (length > 0) {
            byte |= 0x80;
        }
        data.append(static_cast<char>(byte));
    } while (length > 0);
    return data;
}

quint32 MqttService::decodeLength(const QByteArray &data, int &offset)
{
    quint32 length = 0;
    quint32 multiplier = 1;

    while (offset < data.size()) {
        quint8 byte = data.at(offset++);
        length += (byte & 0x7F) * multiplier;
        if ((byte & 0x80) == 0) {
            break;
        }
        multiplier *= 128;
    }

    return length;
}

QString MqttService::decodeString(const QByteArray &data, int &offset)
{
    if (offset + 2 > data.size()) {
        return QString();
    }

    quint16 length = (data.at(offset) << 8) | data.at(offset + 1);
    offset += 2;

    if (offset + length > data.size()) {
        return QString();
    }

    QString str = QString::fromUtf8(data.mid(offset, length));
    offset += length;

    return str;
}
