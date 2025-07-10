#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QMap>

class GPIOController : public QObject
{
    Q_OBJECT

public:
    explicit GPIOController(QObject *parent = nullptr);
    ~GPIOController();

    // 初始化和清理
    bool initialize();
    void cleanup();

    // GPIO基本操作
    bool exportPin(int pin);
    bool unexportPin(int pin);
    bool setDirection(int pin, const QString &direction);
    bool setPin(int pin, bool value);
    bool getPin(int pin);

    // 专用初始化方法
    bool initializePowerSupplyPin(); // 初始化GPIO3_B6为常高电平
    bool initializePumpControlPin(); // 初始化GPIO3_A7水泵控制引脚
    bool initializeFertilizerPumpPin(); // 初始化GPIO3_A1施药泵控制引脚

    // 水泵控制方法
    bool startPump();  // 开启水泵（GPIO3_A7置1）
    bool stopPump();   // 关闭水泵（GPIO3_A7置0）
    bool getPumpStatus(); // 获取水泵状态

    // 施药泵控制方法
    bool startFertilizerPump();  // 开启施药泵（GPIO3_A1置1）
    bool stopFertilizerPump();   // 关闭施药泵（GPIO3_A1置0）
    bool getFertilizerPumpStatus(); // 获取施药泵状态

    // 状态查询
    bool isInitialized() const { return m_initialized; }
    bool isPinExported(int pin) const;

signals:
    void errorOccurred(const QString &error);

private:
    // 内部辅助方法
    bool writeToFile(const QString &filePath, const QString &value);
    QString readFromFile(const QString &filePath);
    QString getPinPath(int pin, const QString &attribute);

    bool m_initialized;
    QMap<int, bool> m_exportedPins; // 记录已导出的引脚
};

#endif // GPIO_CONTROLLER_H
