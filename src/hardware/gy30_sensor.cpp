// GY30光照传感器代码 - 暂时注释保存
/*
#include "hardware/gy30_sensor.h"
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
*/

// 现在使用AHT20温湿度传感器
#include "hardware/gy30_sensor.h" // 文件名保持不变，但内容是AHT20
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

/*
GY30Sensor::GY30Sensor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_devicePath("/dev/i2c-4") // i2c-4设备路径
    , m_currentLux(0.0f)
    , m_initialized(false)
{
    connect(m_timer, &QTimer::timeout, this, &GY30Sensor::readSensorData);
}
*/

// AHT20温湿度传感器实现
AHT20Sensor::AHT20Sensor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_devicePath("/dev/i2c-4") // i2c-4设备路径
    , m_currentTemperature(0.0f)
    , m_currentHumidity(0.0f)
    , m_initialized(false)
{
    connect(m_timer, &QTimer::timeout, this, &AHT20Sensor::readSensorData);
}

/*
GY30Sensor::~GY30Sensor()
{
    stopReading();
}

bool GY30Sensor::initialize()
{
    // 检查设备文件是否存在
    QFile deviceFile(m_devicePath);
    if (!deviceFile.exists()) {
        qWarning() << "GY30传感器设备文件不存在:" << m_devicePath;
        return false;
    }

    m_initialized = true;
    qDebug() << "GY30传感器初始化成功";
    return true;
}

void GY30Sensor::startReading(int intervalMs)
{
    if (!m_initialized) {
        qWarning() << "GY30传感器未初始化，无法开始读取";
        return;
    }

    m_timer->start(intervalMs);
    readSensorData(); // 立即读取一次
    qDebug() << "GY30传感器开始读取，间隔:" << intervalMs << "ms";
}

void GY30Sensor::stopReading()
{
    m_timer->stop();
    qDebug() << "GY30传感器停止读取";
}

float GY30Sensor::getCurrentLux() const
{
    return m_currentLux;
}

void GY30Sensor::readSensorData()
{
    unsigned short rawData = 0;
    if (readRawData(rawData)) {
        float lux = convertToLux(rawData);
        if (m_currentLux != lux) {
            m_currentLux = lux;
            emit luxValueChanged(lux);
        }
    } else {
        qWarning() << "GY30传感器数据读取失败";
    }
}

bool GY30Sensor::readRawData(unsigned short &data)
{
    int fd = open(m_devicePath.toLocal8Bit().data(), O_RDWR);
    if (fd < 0) {
        qWarning() << "无法打开i2c设备:" << m_devicePath;
        return false;
    }

    // 设置BH1750的i2c地址 (0x23)
    if (ioctl(fd, I2C_SLAVE, 0x23) < 0) {
        qWarning() << "设置BH1750 i2c地址失败";
        close(fd);
        return false;
    }

    // 发送开机命令
    unsigned char powerOn = 0x01;
    if (write(fd, &powerOn, 1) != 1) {
        qWarning() << "发送BH1750开机命令失败";
        close(fd);
        return false;
    }

    // 发送连续高分辨率模式命令
    unsigned char mode = 0x10;
    if (write(fd, &mode, 1) != 1) {
        qWarning() << "设置BH1750测量模式失败";
        close(fd);
        return false;
    }

    // 等待测量完成 (180ms)
    usleep(180000);

    // 读取2字节数据
    unsigned char buffer[2];
    if (read(fd, buffer, 2) != 2) {
        qWarning() << "读取BH1750数据失败";
        close(fd);
        return false;
    }

    data = (buffer[0] << 8) | buffer[1];
    close(fd);
    return true;
}

float GY30Sensor::convertToLux(unsigned short rawData)
{
    return static_cast<float>(rawData) / 1.2f; // 转换公式
}
*/

// AHT20温湿度传感器实现
AHT20Sensor::~AHT20Sensor()
{
    stopReading();
}

bool AHT20Sensor::initialize()
{
    // 检查设备文件是否存在
    QFile deviceFile(m_devicePath);
    if (!deviceFile.exists()) {
        return false;
    }

    // 初始化AHT20传感器
    int fd = open(m_devicePath.toLocal8Bit().data(), O_RDWR);
    if (fd < 0) {
        return false;
    }

    // 设置AHT20的i2c地址 (0x38)
    if (ioctl(fd, I2C_SLAVE, 0x38) < 0) {
        close(fd);
        return false;
    }

    // 发送初始化命令
    unsigned char initCmd[3] = {0xBE, 0x08, 0x00};
    if (write(fd, initCmd, 3) != 3) {
        close(fd);
        return false;
    }

    close(fd);
    usleep(10000); // 等待10ms

    m_initialized = true;
    return true;
}

void AHT20Sensor::startReading(int intervalMs)
{
    if (!m_initialized) {
        return;
    }

    m_timer->start(intervalMs);
    readSensorData(); // 立即读取一次
}

void AHT20Sensor::stopReading()
{
    m_timer->stop();
}

float AHT20Sensor::getCurrentTemperature() const
{
    return m_currentTemperature;
}

float AHT20Sensor::getCurrentHumidity() const
{
    return m_currentHumidity;
}

void AHT20Sensor::readSensorData()
{
    float temperature, humidity;
    if (readAHT20Data(temperature, humidity)) {
        bool changed = false;
        if (m_currentTemperature != temperature) {
            m_currentTemperature = temperature;
            changed = true;
        }
        if (m_currentHumidity != humidity) {
            m_currentHumidity = humidity;
            changed = true;
        }
        if (changed) {
            emit dataChanged(temperature, humidity);
        }
    }
}

bool AHT20Sensor::readAHT20Data(float &temperature, float &humidity)
{
    int fd = open(m_devicePath.toLocal8Bit().data(), O_RDWR);
    if (fd < 0) {
        return false;
    }

    // 设置AHT20的i2c地址 (0x38)
    if (ioctl(fd, I2C_SLAVE, 0x38) < 0) {
        close(fd);
        return false;
    }

    // 发送测量命令
    unsigned char measureCmd[3] = {0xAC, 0x33, 0x00};
    if (write(fd, measureCmd, 3) != 3) {
        close(fd);
        return false;
    }

    // 等待测量完成 (80ms)
    usleep(80000);

    // 读取7字节数据
    unsigned char buffer[7];
    if (read(fd, buffer, 7) != 7) {
        close(fd);
        return false;
    }

    close(fd);

    // 检查状态位
    if ((buffer[0] & 0x80) != 0) {
        return false; // 传感器忙碌
    }

    // 解析湿度数据 (20位)
    unsigned int humidityRaw = ((unsigned int)buffer[1] << 12) |
                               ((unsigned int)buffer[2] << 4) |
                               ((unsigned int)buffer[3] >> 4);
    humidity = (float)humidityRaw / 1048576.0f * 100.0f;

    // 解析温度数据 (20位)
    unsigned int temperatureRaw = (((unsigned int)buffer[3] & 0x0F) << 16) |
                                  ((unsigned int)buffer[4] << 8) |
                                  (unsigned int)buffer[5];
    temperature = (float)temperatureRaw / 1048576.0f * 200.0f - 50.0f;

    return true;
}

bool AHT20Sensor::sendCommand(unsigned char cmd)
{
    int fd = open(m_devicePath.toLocal8Bit().data(), O_RDWR);
    if (fd < 0) {
        return false;
    }

    if (ioctl(fd, I2C_SLAVE, 0x38) < 0) {
        close(fd);
        return false;
    }

    bool result = (write(fd, &cmd, 1) == 1);
    close(fd);
    return result;
}

bool AHT20Sensor::readRawData(unsigned char *data, int length)
{
    int fd = open(m_devicePath.toLocal8Bit().data(), O_RDWR);
    if (fd < 0) {
        return false;
    }

    if (ioctl(fd, I2C_SLAVE, 0x38) < 0) {
        close(fd);
        return false;
    }

    bool result = (read(fd, data, length) == length);
    close(fd);
    return result;
}
