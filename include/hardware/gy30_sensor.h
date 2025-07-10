// GY30光照传感器代码 - 暂时注释保存，现在使用AHT20温湿度传感器
/*
#ifndef GY30_SENSOR_H
#define GY30_SENSOR_H

#include <QObject>
#include <QTimer>

class GY30Sensor : public QObject
{
    Q_OBJECT

public:
    explicit GY30Sensor(QObject *parent = nullptr);
    ~GY30Sensor();

    bool initialize(); // 初始化传感器
    void startReading(int intervalMs = 5000); // 开始读取，默认5秒间隔
    void stopReading(); // 停止读取
    float getCurrentLux() const; // 获取当前光照值

signals:
    void luxValueChanged(float lux); // 光照值变化信号

private slots:
    void readSensorData(); // 读取传感器数据

private:
    QTimer *m_timer; // 定时器
    QString m_devicePath; // 设备路径
    float m_currentLux; // 当前光照值
    bool m_initialized; // 初始化状态

    bool readRawData(unsigned short &data); // 读取原始数据
    float convertToLux(unsigned short rawData); // 转换为lux值
};

#endif // GY30_SENSOR_H
*/

// 现在使用AHT20温湿度传感器替代GY30
#ifndef AHT20_SENSOR_H
#define AHT20_SENSOR_H

#include <QObject>
#include <QTimer>

class AHT20Sensor : public QObject
{
    Q_OBJECT

public:
    explicit AHT20Sensor(QObject *parent = nullptr);
    ~AHT20Sensor();

    bool initialize(); // 初始化传感器
    void startReading(int intervalMs = 3000); // 开始读取，默认3秒间隔
    void stopReading(); // 停止读取
    float getCurrentTemperature() const; // 获取当前温度
    float getCurrentHumidity() const; // 获取当前湿度

signals:
    void dataChanged(float temperature, float humidity); // 温湿度变化信号

private slots:
    void readSensorData(); // 读取传感器数据

private:
    QTimer *m_timer; // 定时器
    QString m_devicePath; // I2C设备路径
    float m_currentTemperature; // 当前温度
    float m_currentHumidity; // 当前湿度
    bool m_initialized; // 初始化状态

    bool readAHT20Data(float &temperature, float &humidity); // 读取AHT20数据
    bool sendCommand(unsigned char cmd); // 发送命令
    bool readRawData(unsigned char *data, int length); // 读取原始数据
};

#endif // AHT20_SENSOR_H
