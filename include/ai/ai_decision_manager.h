#ifndef AI_DECISION_MANAGER_H
#define AI_DECISION_MANAGER_H

#include <QObject>
#include <QTimer>

// 前向声明
class CurtainController;
class GY30LightSensor;

/**
 * @brief AI智能决策管理器
 * 
 * 负责基于光照强度自动控制遮光帘的智能决策系统
 * 功能特性：
 * - 光照>500自动开启上帘
 * - 光照<300自动关闭上帘
 * - 每次操作18秒，期间禁用手动控制
 * - 重启默认关闭状态
 */
class AIDecisionManager : public QObject
{
    Q_OBJECT

public:
    // AI决策状态枚举
    enum DecisionState {
        Disabled,       // 关闭状态（默认）
        Enabled,        // 开启状态
        Operating       // 执行操作中（18秒锁定期）
    };

    // 操作类型枚举
    enum OperationType {
        NoOperation,    // 无操作
        OpenCurtain,    // 开启上帘
        CloseCurtain    // 关闭上帘
    };

    explicit AIDecisionManager(QObject *parent = nullptr);
    ~AIDecisionManager();

    // 初始化和设置
    bool initialize();
    void setCurtainController(CurtainController *controller);
    void setLightSensor(GY30LightSensor *sensor);

    // 状态控制
    void enableAIDecision();    // 开启AI决策
    void disableAIDecision();   // 关闭AI决策
    bool isEnabled() const;     // 获取开启状态
    bool isOperating() const;   // 是否正在执行操作

    // 配置参数
    void setLightThresholds(float openThreshold, float closeThreshold);
    void setOperationDuration(int seconds);

signals:
    void stateChanged(DecisionState state);           // 状态改变信号
    void operationStarted(OperationType operation);   // 操作开始信号
    void operationCompleted(OperationType operation); // 操作完成信号
    void manualControlLocked(bool locked);            // 手动控制锁定信号
    void errorOccurred(const QString &error);         // 错误信号

private slots:
    void onLightValueChanged(float lux);              // 光照值变化处理
    void onOperationTimeout();                        // 操作超时处理

private:
    // 核心决策逻辑
    void processLightDecision(float lux);             // 处理光照决策
    void executeOperation(OperationType operation);   // 执行操作
    void lockManualControl();                         // 锁定手动控制
    void unlockManualControl();                       // 解锁手动控制

    // 状态变量
    DecisionState m_state;                            // 当前状态
    OperationType m_currentOperation;                 // 当前操作类型
    bool m_initialized;                               // 初始化标志

    // 控制器引用
    CurtainController *m_curtainController;           // 遮光帘控制器
    GY30LightSensor *m_lightSensor;                   // 光照传感器

    // 定时器
    QTimer *m_operationTimer;                         // 操作定时器（18秒）

    // 配置参数
    float m_openThreshold;                            // 开帘光照阈值（默认500）
    float m_closeThreshold;                           // 关帘光照阈值（默认300）
    int m_operationDuration;                          // 操作持续时间（默认18秒）

    // 防抖动
    float m_lastLightValue;                           // 上次光照值
    QTimer *m_debounceTimer;                          // 防抖动定时器
};

#endif // AI_DECISION_MANAGER_H
