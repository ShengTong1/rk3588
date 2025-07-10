#ifndef CURTAIN_CONTROLLER_H
#define CURTAIN_CONTROLLER_H

#include <QObject>
#include <QString>

// 前向声明
class GPIOController;

/**
 * @brief 保温帘控制器
 *
 * 负责管理顶部和侧部保温帘的控制
 * 支持步进电机控制和状态监控
 */
class CurtainController : public QObject
{
    Q_OBJECT

public:
    // 保温帘类型枚举
    enum CurtainType {
        TopCurtain,    // 顶部保温帘
        SideCurtain    // 侧部保温帘
    };

    // 保温帘状态枚举
    enum CurtainState {
        Stopped,       // 停止状态
        Opening,       // 打开中
        Closing,       // 关闭中
        Paused,        // 暂停状态
        Open,          // 完全打开
        Closed,        // 完全关闭
        Error          // 错误状态
    };

    explicit CurtainController(QObject *parent = nullptr);
    ~CurtainController();

    // 主要功能接口
    bool openCurtain(CurtainType type);   // 打开保温帘
    bool closeCurtain(CurtainType type);  // 关闭保温帘
    bool pauseCurtain(CurtainType type);  // 暂停保温帘运动
    bool resumeCurtain(CurtainType type); // 恢复保温帘运动
    void stopCurtain(CurtainType type);   // 停止保温帘运动

    // GPIO控制接口
    bool openTopCurtain();                // 上侧帘打开
    bool closeTopCurtain();               // 上侧帘关闭
    bool pauseTopCurtain();               // 上侧帘暂停
    bool openSideCurtain();               // 侧面帘打开
    bool closeSideCurtain();              // 侧面帘关闭
    bool pauseSideCurtain();              // 侧面帘暂停

    // 初始化和设置
    bool initialize();                    // 初始化GPIO控制器
    void setGPIOController(GPIOController *controller); // 设置GPIO控制器

    // 状态查询
    CurtainState getCurtainState(CurtainType type) const;
    QString getStatusString() const;      // 获取状态字符串
    void updateStatus();                  // 更新状态

signals:
    void curtainStateChanged(CurtainType type, CurtainState state); // 状态改变信号
    void statusUpdated(const QString &status); // 状态更新信号
    void errorOccurred(const QString &error);  // 错误信号

private slots:
    void onOperationTimeout();           // 操作超时处理

private:
    // GPIO控制方法
    bool setTopCurtainGPIO(bool dir1, bool dir2, bool enable);   // 设置上侧帘GPIO
    bool setSideCurtainGPIO(bool dir1, bool dir2, bool enable);  // 设置侧面帘GPIO
    bool setTopCurtainGPIOWithDual(bool dir1, bool dir2, bool enable); // 设置上侧帘GPIO（双使能引脚）
    bool setSideCurtainGPIOWithDual(bool dir1, bool dir2, bool enable); // 设置侧面帘GPIO（双使能引脚）
    bool initializeGPIOPins();                                   // 初始化GPIO引脚

    // 状态变量
    CurtainState m_topCurtainState;
    CurtainState m_sideCurtainState;
    bool m_initialized;

    // GPIO控制器
    GPIOController *m_gpioController;

    // 硬件控制接口（待实现）
    bool controlStepperMotor(CurtainType type, bool open); // 步进电机控制
    bool readSensorStatus(CurtainType type);               // 读取传感器状态

    // 工具函数
    QString curtainTypeToString(CurtainType type) const;
    QString curtainStateToString(CurtainState state) const;
};

#endif // CURTAIN_CONTROLLER_H
