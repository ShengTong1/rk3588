#include "hardware/gy30_light_sensor.h"
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cmath>

GY30LightSensor::GY30LightSensor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_devicePath("/dev/i2c-7") // I2C7设备路径
    , m_currentLux(0.0f)
    , m_initialized(false)
{
    connect(m_timer, &QTimer::timeout, this, &GY30LightSensor::readSensorData);
}

GY30LightSensor::~GY30LightSensor()
{
    stopReading();
}

bool GY30LightSensor::initialize()
{
    // 检查设备文件是否存在
    QFile deviceFile(m_devicePath);
    if (!deviceFile.exists()) {
        qWarning() << "GY30传感器设备文件不存在:" << m_devicePath;
        // 即使设备不存在也标记为初始化成功，使用模拟数据
        m_initialized = true;
        return true;
    }

    m_initialized = true;
    // GY30光照传感器初始化成功
    return true;
}

void GY30LightSensor::startReading(int intervalMs)
{
    if (!m_initialized) {
        qWarning() << "GY30传感器未初始化，无法开始读取";
        return;
    }

    m_timer->start(intervalMs);
    readSensorData(); // 立即读取一次
    // GY30传感器开始读取
}

void GY30LightSensor::stopReading()
{
    m_timer->stop();
    // GY30传感器停止读取
}

float GY30LightSensor::getCurrentLux() const
{
    return m_currentLux;
}

void GY30LightSensor::readSensorData()
{
    unsigned short rawData = 0;
    if (readRawData(rawData)) {
        float lux = convertToLux(rawData);
        if (m_currentLux != lux) {
            m_currentLux = lux;
            emit luxValueChanged(lux);
        }
    } else {
        // 硬件不可用时生成模拟数据
        static int counter = 0;
        counter++;

        // 生成300-800lx范围的模拟光照数据
        float simulatedLux = 500.0f + 150.0f * sin(counter * 0.1f);

        if (m_currentLux != simulatedLux) {
            m_currentLux = simulatedLux;
            emit luxValueChanged(simulatedLux);
            qDebug() << "GY30传感器使用模拟数据:" << simulatedLux << "lx";
        }
    }
}

bool GY30LightSensor::readRawData(unsigned short &data)
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

float GY30LightSensor::convertToLux(unsigned short rawData)
{
    return static_cast<float>(rawData) / 1.2f; // BH1750转换公式
}
