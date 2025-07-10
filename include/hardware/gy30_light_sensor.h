#ifndef GY30_LIGHT_SENSOR_H
#define GY30_LIGHT_SENSOR_H

#include <QObject>
#include <QTimer>

/**
 * @brief GY30光照传感器类 - 基于BH1750芯片
 * 
 * 使用I2C7接口连接，提供光照强度检测功能
 */
class GY30LightSensor : public QObject
{
    Q_OBJECT

public:
    explicit GY30LightSensor(QObject *parent = nullptr);
    ~GY30LightSensor();
    
    bool initialize(); // 初始化传感器
    void startReading(int intervalMs = 2000); // 开始读取，默认2秒间隔
    void stopReading(); // 停止读取
    float getCurrentLux() const; // 获取当前光照值

signals:
    void luxValueChanged(float lux); // 光照值变化信号

private slots:
    void readSensorData(); // 读取传感器数据

private:
    QTimer *m_timer; // 定时器
    QString m_devicePath; // I2C设备路径
    float m_currentLux; // 当前光照值
    bool m_initialized; // 初始化状态

    bool readRawData(unsigned short &data); // 读取原始数据
    float convertToLux(unsigned short rawData); // 转换为lux值
};

#endif // GY30_LIGHT_SENSOR_H
