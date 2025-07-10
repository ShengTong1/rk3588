#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <QObject>
#include <QString>

/**
 * @brief PWM补光灯控制器
 *
 * 负责管理PWM硬件控制，实现补光灯强度调节
 * 基于Linux sysfs PWM接口，支持1000Hz频率控制
 */
class PWMController : public QObject
{
    Q_OBJECT

public:
    explicit PWMController(QObject *parent = nullptr);
    ~PWMController();

    // 主要功能接口
    bool initialize();                    // 初始化PWM设备
    bool setDutyCycle(int percentage);    // 设置占空比(0-100%)
    bool enable(bool enabled);            // 启用/禁用PWM
    void cleanup();                       // 清理PWM资源

    // 状态查询
    bool isInitialized() const { return m_initialized; }
    int getCurrentDutyCycle() const { return m_currentDutyCycle; }
    int readActualDutyCycle();            // 从硬件读取实际占空比

signals:
    void dutyCycleChanged(int percentage); // 占空比改变信号
    void statusChanged(bool enabled);      // 状态改变信号
    void errorOccurred(const QString &error); // 错误信号

private:
    // PWM配置常量
    static const QString PWM_CHIP_PATH;
    static const QString PWM_EXPORT_PATH;
    static const QString PWM_PATH;
    static const int PWM_PERIOD_NS;       // 1000Hz = 1000000ns

    // 状态变量
    bool m_initialized;
    int m_currentDutyCycle;

    // 内部功能函数
    bool exportPWM();                     // 导出PWM设备
    bool setPeriod(int periodNs);         // 设置PWM周期
    bool setPolarity(const QString &polarity); // 设置PWM极性
    bool writeToFile(const QString &filePath, const QString &value); // 写入文件
    QString readFromFile(const QString &filePath); // 从文件读取
    int readActualDutyCycleInternal();    // 内部读取占空比(不检查初始化状态)
};

#endif // PWM_CONTROLLER_H
