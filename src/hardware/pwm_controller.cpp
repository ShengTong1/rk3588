#include "hardware/pwm_controller.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QDebug>

// PWM配置常量定义
const QString PWMController::PWM_CHIP_PATH = "/sys/class/pwm/pwmchip0";
const QString PWMController::PWM_EXPORT_PATH = PWM_CHIP_PATH + "/export";
const QString PWMController::PWM_PATH = PWM_CHIP_PATH + "/pwm0";
const int PWMController::PWM_PERIOD_NS = 1000000; // 1000Hz = 1000000ns

PWMController::PWMController(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_currentDutyCycle(60) // 默认60%占空比
{
    qDebug() << "PWM控制器创建完成";
}

PWMController::~PWMController()
{
    cleanup();
    qDebug() << "PWM控制器已销毁";
}

bool PWMController::initialize()
{
    qDebug() << "开始初始化PWM控制器...";

    // 检查PWM设备是否存在
    if (!QDir(PWM_CHIP_PATH).exists()) {
        QString error = QString("PWM设备不存在: %1").arg(PWM_CHIP_PATH);
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    // 检查PWM设备是否已经存在
    QFile pwmDir(PWM_PATH);
    bool pwmExists = pwmDir.exists();

    if (pwmExists) {
        // 读取当前硬件占空比并同步到缓存
        int actualDuty = readActualDutyCycleInternal();
        if (actualDuty >= 0) {
            m_currentDutyCycle = actualDuty;
        }
    } else {
        // 导出PWM设备
        if (!exportPWM()) {
            emit errorOccurred("PWM设备导出失败");
            return false;
        }
    }

    // 设置PWM极性为正常
    if (!setPolarity("normal")) {
        qWarning() << "PWM极性设置失败，继续执行...";
        // 极性设置失败不是致命错误
    }

    // 设置PWM周期(确保1000Hz)
    if (!setPeriod(PWM_PERIOD_NS)) {
        emit errorOccurred("PWM周期设置失败");
        return false;
    }

    // 标记为已初始化
    m_initialized = true;

    // 如果是新导出的设备，设置初始占空比
    if (!pwmExists) {
        if (!setDutyCycle(m_currentDutyCycle)) {
            qWarning() << "PWM初始占空比设置失败";
            m_initialized = false;
            return false;
        }
    }

    // 启用PWM
    if (!enable(true)) {
        qWarning() << "PWM启用失败";
        m_initialized = false;
        return false;
    }

    return true;
}

bool PWMController::setDutyCycle(int percentage)
{
    if (!m_initialized) {
        QString error = "PWM未初始化，无法设置占空比";
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    // 限制范围 0-100%
    percentage = qBound(0, percentage, 100);

    // 计算占空比对应的纳秒值
    int dutyCycleNs = (PWM_PERIOD_NS * percentage) / 100;

    // 设置占空比
    QString dutyCycleFile = PWM_PATH + "/duty_cycle";
    if (writeToFile(dutyCycleFile, QString::number(dutyCycleNs))) {
        m_currentDutyCycle = percentage;
        emit dutyCycleChanged(percentage);
        return true;
    } else {
        QString error = QString("PWM占空比设置失败: %1%").arg(percentage);
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }
}

bool PWMController::enable(bool enabled)
{
    if (!m_initialized && enabled) {
        QString error = "PWM未初始化，无法启用";
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    QString enableFile = PWM_PATH + "/enable";
    QString value = enabled ? "1" : "0";

    if (writeToFile(enableFile, value)) {
        emit statusChanged(enabled);
        qDebug() << QString("PWM %1成功").arg(enabled ? "启用" : "禁用");
        return true;
    } else {
        QString error = QString("PWM %1失败").arg(enabled ? "启用" : "禁用");
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }
}

void PWMController::cleanup()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "开始清理PWM资源...";

    // 禁用PWM
    enable(false);

    // 注销PWM0 (可选，系统重启时会自动清理)
    QString unexportFile = PWM_CHIP_PATH + "/unexport";
    if (writeToFile(unexportFile, "0")) {
        qDebug() << "PWM0注销成功";
    } else {
        qWarning() << "PWM0注销失败";
    }

    m_initialized = false;
    qDebug() << "PWM资源清理完成";
}

bool PWMController::exportPWM()
{
    // 检查PWM0目录是否已存在
    if (QDir(PWM_PATH).exists()) {
        qDebug() << "PWM0设备已存在，跳过导出";
        return true;
    }

    // 导出PWM0
    if (writeToFile(PWM_EXPORT_PATH, "0")) {
        qDebug() << "PWM0导出成功";

        // 等待PWM设备就绪
        QThread::msleep(100);

        // 再次检查PWM0目录是否存在
        if (QDir(PWM_PATH).exists()) {
            return true;
        } else {
            qWarning() << "PWM0设备目录仍不存在:" << PWM_PATH;
            return false;
        }
    } else {
        qWarning() << "PWM0导出失败";
        return false;
    }
}

bool PWMController::setPeriod(int periodNs)
{
    QString periodFile = PWM_PATH + "/period";
    if (writeToFile(periodFile, QString::number(periodNs))) {
        qDebug() << QString("PWM周期设置成功: %1ns (1000Hz)").arg(periodNs);
        return true;
    } else {
        qWarning() << "PWM周期设置失败";
        return false;
    }
}

bool PWMController::setPolarity(const QString &polarity)
{
    QString polarityFile = PWM_PATH + "/polarity";
    if (writeToFile(polarityFile, polarity)) {
        qDebug() << QString("PWM极性设置成功: %1").arg(polarity);
        return true;
    } else {
        qWarning() << "PWM极性设置失败";
        return false;
    }
}

bool PWMController::writeToFile(const QString &filePath, const QString &value)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << value;
        file.close();
        return true;
    } else {
        qWarning() << QString("无法写入文件 %1: %2").arg(filePath).arg(file.errorString());
        return false;
    }
}

QString PWMController::readFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString content = stream.readAll().trimmed();
        file.close();
        return content;
    } else {
        qWarning() << QString("无法读取文件 %1: %2").arg(filePath).arg(file.errorString());
        return QString();
    }
}

int PWMController::readActualDutyCycle()
{
    if (!m_initialized) {
        qWarning() << "PWM未初始化，无法读取占空比";
        return m_currentDutyCycle; // 返回缓存值
    }

    return readActualDutyCycleInternal();
}

int PWMController::readActualDutyCycleInternal()
{
    // 读取实际的占空比值
    QString dutyCycleFile = PWM_PATH + "/duty_cycle";
    QString dutyCycleStr = readFromFile(dutyCycleFile);

    if (dutyCycleStr.isEmpty()) {
        qWarning() << "无法读取PWM占空比文件";
        return -1; // 返回错误值
    }

    // 将纳秒值转换为百分比
    bool ok;
    int dutyCycleNs = dutyCycleStr.toInt(&ok);
    if (!ok) {
        qWarning() << "PWM占空比值格式错误:" << dutyCycleStr;
        return -1;
    }

    // 计算百分比 (dutyCycleNs / PWM_PERIOD_NS * 100)
    int percentage = (dutyCycleNs * 100) / PWM_PERIOD_NS;

    return percentage;
}
