#include "device/curtain_controller.h"
#include "hardware/gpio_controller.h"
#include "config/gpio_config.h"

#include <QDebug>
#include <QTimer>

CurtainController::CurtainController(QObject *parent)
    : QObject(parent)
    , m_topCurtainState(Stopped)
    , m_sideCurtainState(Stopped)
    , m_initialized(false)
    , m_gpioController(nullptr)
{
    qDebug() << "保温帘控制器创建完成";
}

CurtainController::~CurtainController()
{
    qDebug() << "保温帘控制器已销毁";
}

bool CurtainController::openCurtain(CurtainType type)
{
    qDebug() << QString("打开%1").arg(curtainTypeToString(type));

    // 设置状态为打开中
    if (type == TopCurtain) {
        m_topCurtainState = Opening;
    } else {
        m_sideCurtainState = Opening;
    }

    emit curtainStateChanged(type, Opening);

    // 调用实际的GPIO控制方法
    bool success = false;
    if (type == TopCurtain) {
        success = openTopCurtain();
    } else {
        success = openSideCurtain();
    }

    if (success) {
        // 模拟操作完成后设置为打开状态
        QTimer::singleShot(2000, this, [this, type]() {
            if (type == TopCurtain) {
                m_topCurtainState = Open;
            } else {
                m_sideCurtainState = Open;
            }
            emit curtainStateChanged(type, Open);
            updateStatus();
        });
    } else {
        // 操作失败，设置为错误状态
        if (type == TopCurtain) {
            m_topCurtainState = Error;
        } else {
            m_sideCurtainState = Error;
        }
        emit curtainStateChanged(type, Error);
        emit errorOccurred(QString("%1打开失败").arg(curtainTypeToString(type)));
    }

    return success;
}

bool CurtainController::closeCurtain(CurtainType type)
{
    qDebug() << QString("关闭%1").arg(curtainTypeToString(type));

    // 设置状态为关闭中
    if (type == TopCurtain) {
        m_topCurtainState = Closing;
    } else {
        m_sideCurtainState = Closing;
    }

    emit curtainStateChanged(type, Closing);

    // 调用实际的GPIO控制方法
    bool success = false;
    if (type == TopCurtain) {
        success = closeTopCurtain();
    } else {
        success = closeSideCurtain();
    }

    if (success) {
        // 模拟操作完成后设置为关闭状态
        QTimer::singleShot(2000, this, [this, type]() {
            if (type == TopCurtain) {
                m_topCurtainState = Closed;
            } else {
                m_sideCurtainState = Closed;
            }
            emit curtainStateChanged(type, Closed);
            updateStatus();
        });
    } else {
        // 操作失败，设置为错误状态
        if (type == TopCurtain) {
            m_topCurtainState = Error;
        } else {
            m_sideCurtainState = Error;
        }
        emit curtainStateChanged(type, Error);
        emit errorOccurred(QString("%1关闭失败").arg(curtainTypeToString(type)));
    }

    return success;
}

bool CurtainController::pauseCurtain(CurtainType type)
{
    qDebug() << QString("暂停%1运动").arg(curtainTypeToString(type));

    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    bool success = false;
    if (type == TopCurtain) {
        success = pauseTopCurtain();
    } else {
        success = pauseSideCurtain();
    }

    if (!success) {
        emit errorOccurred(QString("%1暂停失败").arg(curtainTypeToString(type)));
    }

    return success;
}

bool CurtainController::resumeCurtain(CurtainType type)
{
    qDebug() << QString("恢复%1运动").arg(curtainTypeToString(type));

    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    // 根据当前状态决定恢复方向
    CurtainState currentState = getCurtainState(type);
    bool success = false;

    if (currentState == Paused) {
        // 暂停状态下恢复到之前的运动状态
        // 这里简化处理，默认恢复为打开操作
        success = openCurtain(type);
    } else {
        qWarning() << QString("%1不在暂停状态，无法恢复").arg(curtainTypeToString(type));
        return false;
    }

    if (!success) {
        emit errorOccurred(QString("%1恢复失败").arg(curtainTypeToString(type)));
    }

    return success;
}

void CurtainController::stopCurtain(CurtainType type)
{
    qDebug() << QString("停止%1运动").arg(curtainTypeToString(type));

    // TODO: 实现停止控制逻辑

    // 保持当前状态不变
    emit curtainStateChanged(type, getCurtainState(type));
}

CurtainController::CurtainState CurtainController::getCurtainState(CurtainType type) const
{
    return (type == TopCurtain) ? m_topCurtainState : m_sideCurtainState;
}

QString CurtainController::getStatusString() const
{
    QString topStatus = curtainStateToString(m_topCurtainState);
    QString sideStatus = curtainStateToString(m_sideCurtainState);

    return QString("🌡️ 当前状态: 顶部%1 | 侧部%2 | 温度: 适宜")
           .arg(topStatus)
           .arg(sideStatus);
}

void CurtainController::updateStatus()
{
    QString status = getStatusString();
    emit statusUpdated(status);
}

void CurtainController::onOperationTimeout()
{
    emit errorOccurred("保温帘操作超时");
}

bool CurtainController::controlStepperMotor(CurtainType type, bool open)
{
    // TODO: 实现实际的步进电机控制
    // 这里可以添加以下控制方式：
    // 1. 串口通信控制
    // 2. GPIO控制
    // 3. 网络通信控制
    // 4. I2C/SPI通信控制

    Q_UNUSED(type)
    Q_UNUSED(open)

    // 模拟控制成功
    return true;
}

bool CurtainController::readSensorStatus(CurtainType type)
{
    // TODO: 实现传感器状态读取
    // 可以读取限位开关、位置传感器等状态

    Q_UNUSED(type)

    // 模拟传感器读取
    return true;
}

QString CurtainController::curtainTypeToString(CurtainType type) const
{
    switch (type) {
        case TopCurtain:
            return "顶部保温帘";
        case SideCurtain:
            return "侧部保温帘";
        default:
            return "未知保温帘";
    }
}

QString CurtainController::curtainStateToString(CurtainState state) const
{
    switch (state) {
        case Stopped:
            return "停止";
        case Opening:
            return "打开中";
        case Closing:
            return "关闭中";
        case Paused:
            return "暂停";
        case Open:
            return "已打开";
        case Closed:
            return "已关闭";
        case Error:
            return "错误";
        default:
            return "未知状态";
    }
}

// GPIO控制方法实现

bool CurtainController::initialize()
{
    if (m_initialized) {
        return true;
    }

    if (!m_gpioController) {
        emit errorOccurred("GPIO控制器未设置");
        return false;
    }

    if (!initializeGPIOPins()) {
        emit errorOccurred("GPIO引脚初始化失败");
        return false;
    }

    m_initialized = true;
    return true;
}

void CurtainController::setGPIOController(GPIOController *controller)
{
    m_gpioController = controller;
}

bool CurtainController::openTopCurtain()
{
    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    // 上侧帘打开: 引脚1高电平，引脚2低电平，使能低电平（双使能引脚）
    return setTopCurtainGPIOWithDual(GPIO_HIGH, GPIO_LOW, CURTAIN_ENABLE);
}

bool CurtainController::closeTopCurtain()
{
    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    // 上侧帘关闭: 引脚1低电平，引脚2高电平，使能低电平（双使能引脚）
    return setTopCurtainGPIOWithDual(GPIO_LOW, GPIO_HIGH, CURTAIN_ENABLE);
}

bool CurtainController::pauseTopCurtain()
{
    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    // 上侧帘暂停: 使能高电平（双使能引脚）
    return (m_gpioController->setPin(TOP_CURTAIN_ENABLE_PIN, CURTAIN_DISABLE) &&
            m_gpioController->setPin(TOP_CURTAIN_ENABLE2_PIN, CURTAIN_DISABLE));
}

bool CurtainController::openSideCurtain()
{
    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    // 侧面帘打开: 引脚3高电平，引脚4低电平，使能低电平（双使能引脚）
    return setSideCurtainGPIOWithDual(GPIO_HIGH, GPIO_LOW, CURTAIN_ENABLE);
}

bool CurtainController::closeSideCurtain()
{
    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    // 侧面帘关闭: 引脚3低电平，引脚4高电平，使能低电平（双使能引脚）
    return setSideCurtainGPIOWithDual(GPIO_LOW, GPIO_HIGH, CURTAIN_ENABLE);
}

bool CurtainController::pauseSideCurtain()
{
    if (!m_initialized) {
        emit errorOccurred("保温帘控制器未初始化");
        return false;
    }

    // 侧面帘暂停: 使能高电平（双使能引脚）
    return (m_gpioController->setPin(SIDE_CURTAIN_ENABLE_PIN, CURTAIN_DISABLE) &&
            m_gpioController->setPin(SIDE_CURTAIN_ENABLE2_PIN, CURTAIN_DISABLE));
}

bool CurtainController::setTopCurtainGPIO(bool dir1, bool dir2, bool enable)
{
    if (!m_gpioController) {
        return false;
    }

    bool success = true;
    success &= m_gpioController->setPin(TOP_CURTAIN_DIR1_PIN, dir1);
    success &= m_gpioController->setPin(TOP_CURTAIN_DIR2_PIN, dir2);
    success &= m_gpioController->setPin(TOP_CURTAIN_ENABLE_PIN, enable);

    return success;
}

bool CurtainController::setSideCurtainGPIO(bool dir1, bool dir2, bool enable)
{
    if (!m_gpioController) {
        return false;
    }

    bool success = true;
    success &= m_gpioController->setPin(SIDE_CURTAIN_DIR1_PIN, dir1);
    success &= m_gpioController->setPin(SIDE_CURTAIN_DIR2_PIN, dir2);
    success &= m_gpioController->setPin(SIDE_CURTAIN_ENABLE_PIN, enable);

    return success;
}

bool CurtainController::setTopCurtainGPIOWithDual(bool dir1, bool dir2, bool enable)
{
    if (!m_gpioController) {
        return false;
    }

    bool success = true;
    success &= m_gpioController->setPin(TOP_CURTAIN_DIR1_PIN, dir1);
    success &= m_gpioController->setPin(TOP_CURTAIN_DIR2_PIN, dir2);
    success &= m_gpioController->setPin(TOP_CURTAIN_ENABLE_PIN, enable);  // GPIO3_A3
    success &= m_gpioController->setPin(TOP_CURTAIN_ENABLE2_PIN, enable); // GPIO3_A0 - 逻辑完全同GPIO3_A3

    return success;
}

bool CurtainController::setSideCurtainGPIOWithDual(bool dir1, bool dir2, bool enable)
{
    if (!m_gpioController) {
        return false;
    }

    bool success = true;
    success &= m_gpioController->setPin(SIDE_CURTAIN_DIR1_PIN, dir1);
    success &= m_gpioController->setPin(SIDE_CURTAIN_DIR2_PIN, dir2);
    success &= m_gpioController->setPin(SIDE_CURTAIN_ENABLE_PIN, enable);  // GPIO3_B1
    success &= m_gpioController->setPin(SIDE_CURTAIN_ENABLE2_PIN, enable); // GPIO3_A5 - 逻辑同GPIO3_B1

    return success;
}

bool CurtainController::initializeGPIOPins()
{
    if (!m_gpioController) {
        return false;
    }

    bool success = true;

    // 导出并设置上侧帘控制引脚
    success &= m_gpioController->exportPin(TOP_CURTAIN_DIR1_PIN);
    success &= m_gpioController->exportPin(TOP_CURTAIN_DIR2_PIN);
    success &= m_gpioController->exportPin(TOP_CURTAIN_ENABLE_PIN);
    success &= m_gpioController->exportPin(TOP_CURTAIN_ENABLE2_PIN); // 新增双使能引脚

    // 导出并设置侧面帘控制引脚
    success &= m_gpioController->exportPin(SIDE_CURTAIN_DIR1_PIN);
    success &= m_gpioController->exportPin(SIDE_CURTAIN_DIR2_PIN);
    success &= m_gpioController->exportPin(SIDE_CURTAIN_ENABLE_PIN);
    success &= m_gpioController->exportPin(SIDE_CURTAIN_ENABLE2_PIN); // 新增双使能引脚

    if (success) {
        // 设置所有引脚为输出方向
        success &= m_gpioController->setDirection(TOP_CURTAIN_DIR1_PIN, "out");
        success &= m_gpioController->setDirection(TOP_CURTAIN_DIR2_PIN, "out");
        success &= m_gpioController->setDirection(TOP_CURTAIN_ENABLE_PIN, "out");
        success &= m_gpioController->setDirection(TOP_CURTAIN_ENABLE2_PIN, "out"); // 新增双使能引脚
        success &= m_gpioController->setDirection(SIDE_CURTAIN_DIR1_PIN, "out");
        success &= m_gpioController->setDirection(SIDE_CURTAIN_DIR2_PIN, "out");
        success &= m_gpioController->setDirection(SIDE_CURTAIN_ENABLE_PIN, "out");
        success &= m_gpioController->setDirection(SIDE_CURTAIN_ENABLE2_PIN, "out"); // 新增双使能引脚

        // 初始化为停止状态（使能高电平，方向引脚低电平）- 使用双使能引脚
        if (success) {
            success &= setTopCurtainGPIOWithDual(GPIO_LOW, GPIO_LOW, CURTAIN_DISABLE);
            success &= setSideCurtainGPIOWithDual(GPIO_LOW, GPIO_LOW, CURTAIN_DISABLE);
        }
    }

    return success;
}