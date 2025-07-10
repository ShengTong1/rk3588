#include "ai/ai_decision_manager.h"
#include "device/curtain_controller.h"
#include "hardware/gy30_light_sensor.h"
#include "config/ai_config.h"

#include <QDebug>
#include <QTimer>

AIDecisionManager::AIDecisionManager(QObject *parent)
    : QObject(parent)
    , m_state(Disabled)  // 重启默认关闭状态
    , m_currentOperation(NoOperation)
    , m_initialized(false)
    , m_curtainController(nullptr)
    , m_lightSensor(nullptr)
    , m_operationTimer(new QTimer(this))
    , m_openThreshold(AI_LIGHT_OPEN_THRESHOLD)      // 光照>500开帘
    , m_closeThreshold(AI_LIGHT_CLOSE_THRESHOLD)     // 光照<300关帘
    , m_operationDuration(AI_OPERATION_DURATION * 1000)   // 18秒操作时间
    , m_lastLightValue(0.0f)
    , m_debounceTimer(new QTimer(this))
{
    // 配置操作定时器
    m_operationTimer->setSingleShot(true);
    connect(m_operationTimer, &QTimer::timeout, this, &AIDecisionManager::onOperationTimeout);

    // 配置防抖动定时器
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(AI_DEBOUNCE_INTERVAL * 1000);

    qDebug() << "AI智能决策管理器创建完成 - 默认关闭状态";
}

AIDecisionManager::~AIDecisionManager()
{
    qDebug() << "AI智能决策管理器已销毁";
}

bool AIDecisionManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    if (!m_curtainController) {
        qWarning() << "遮光帘控制器未设置";
        return false;
    }

    if (!m_lightSensor) {
        qWarning() << "光照传感器未设置";
        return false;
    }

    // 连接光照传感器信号
    connect(m_lightSensor, &GY30LightSensor::luxValueChanged,
            this, &AIDecisionManager::onLightValueChanged);

    m_initialized = true;
    qDebug() << "AI智能决策管理器初始化完成";
    return true;
}

void AIDecisionManager::setCurtainController(CurtainController *controller)
{
    m_curtainController = controller;
}

void AIDecisionManager::setLightSensor(GY30LightSensor *sensor)
{
    m_lightSensor = sensor;
}

void AIDecisionManager::enableAIDecision()
{
    if (m_state == Operating) {
        qWarning() << "AI决策正在执行操作中，无法切换状态";
        return;
    }

    m_state = Enabled;
    emit stateChanged(m_state);
    qDebug() << "AI智能决策已开启";
}

void AIDecisionManager::disableAIDecision()
{
    if (m_state == Operating) {
        qWarning() << "AI决策正在执行操作中，无法切换状态";
        return;
    }

    m_state = Disabled;
    emit stateChanged(m_state);
    qDebug() << "AI智能决策已关闭";
}

bool AIDecisionManager::isEnabled() const
{
    return m_state == Enabled;
}

bool AIDecisionManager::isOperating() const
{
    return m_state == Operating;
}

void AIDecisionManager::setLightThresholds(float openThreshold, float closeThreshold)
{
    m_openThreshold = openThreshold;
    m_closeThreshold = closeThreshold;
    qDebug() << QString("光照阈值已更新: 开帘>%1, 关帘<%2").arg(openThreshold).arg(closeThreshold);
}

void AIDecisionManager::setOperationDuration(int seconds)
{
    m_operationDuration = seconds * 1000; // 转换为毫秒
    qDebug() << QString("操作持续时间已更新: %1秒").arg(seconds);
}

void AIDecisionManager::onLightValueChanged(float lux)
{
    if (m_state != Enabled) {
        return; // 只有开启状态才处理光照变化
    }

    // 防抖动处理
    m_lastLightValue = lux;
    m_debounceTimer->stop();

    // 断开之前的连接，避免重复连接
    disconnect(m_debounceTimer, &QTimer::timeout, this, nullptr);

    // 连接新的处理函数
    connect(m_debounceTimer, &QTimer::timeout, [this]() {
        processLightDecision(m_lastLightValue);
    });

    m_debounceTimer->start();
}

void AIDecisionManager::processLightDecision(float lux)
{
    if (m_state != Enabled) {
        return;
    }

    OperationType operation = NoOperation;

    // 决策逻辑
    if (lux > m_openThreshold) {
        operation = OpenCurtain;
        qDebug() << QString("光照强度%1 > %2，决策：开启上帘").arg(lux).arg(m_openThreshold);
    } else if (lux < m_closeThreshold) {
        operation = CloseCurtain;
        qDebug() << QString("光照强度%1 < %2，决策：关闭上帘").arg(lux).arg(m_closeThreshold);
    }

    if (operation != NoOperation) {
        executeOperation(operation);
    }
}

void AIDecisionManager::executeOperation(OperationType operation)
{
    if (m_state == Operating) {
        qDebug() << "AI决策正在执行操作中，忽略新的操作请求";
        return;
    }

    // 切换到操作状态
    m_state = Operating;
    m_currentOperation = operation;
    emit stateChanged(m_state);
    emit operationStarted(operation);

    // 锁定手动控制
    lockManualControl();

    // 执行具体操作
    bool success = false;
    if (operation == OpenCurtain) {
        success = m_curtainController->openTopCurtain();
        qDebug() << "AI决策执行：开启上帘";
    } else if (operation == CloseCurtain) {
        success = m_curtainController->closeTopCurtain();
        qDebug() << "AI决策执行：关闭上帘";
    }

    if (!success) {
        emit errorOccurred("AI决策操作执行失败");
        onOperationTimeout(); // 立即结束操作
        return;
    }

    // 启动18秒定时器
    m_operationTimer->start(m_operationDuration);
    qDebug() << QString("AI决策操作开始，%1秒后自动结束").arg(m_operationDuration / 1000);
}

void AIDecisionManager::onOperationTimeout()
{
    if (m_state != Operating) {
        return;
    }

    // 停止操作
    if (m_currentOperation == OpenCurtain || m_currentOperation == CloseCurtain) {
        m_curtainController->pauseTopCurtain(); // 暂停操作
        qDebug() << "AI决策操作时间到，暂停遮光帘";
    }

    // 解锁手动控制
    unlockManualControl();

    // 恢复到开启状态
    OperationType completedOperation = m_currentOperation;
    m_currentOperation = NoOperation;
    m_state = Enabled;

    emit operationCompleted(completedOperation);
    emit stateChanged(m_state);
    qDebug() << "AI决策操作完成，恢复到开启状态";
}

void AIDecisionManager::lockManualControl()
{
    emit manualControlLocked(true);
    qDebug() << "手动控制已锁定";
}

void AIDecisionManager::unlockManualControl()
{
    emit manualControlLocked(false);
    qDebug() << "手动控制已解锁";
}
