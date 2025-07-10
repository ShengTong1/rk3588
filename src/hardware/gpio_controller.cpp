#include "hardware/gpio_controller.h"
#include "config/gpio_config.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>

GPIOController::GPIOController(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

GPIOController::~GPIOController()
{
    cleanup();
}

bool GPIOController::initialize()
{
    if (m_initialized) {
        return true;
    }

    // 检查GPIO系统是否可用
    QDir gpioDir(GPIO_BASE_PATH);
    if (!gpioDir.exists()) {
        emit errorOccurred("GPIO系统不可用");
        return false;
    }

    m_initialized = true;

    // 初始化GPIO3_B6为常高电平（3.3V电源输出）
    if (!initializePowerSupplyPin()) {
        qWarning() << "GPIO3_B6电源引脚初始化失败";
    }

    // 初始化GPIO3_A7水泵控制引脚
    if (!initializePumpControlPin()) {
        qWarning() << "GPIO3_A7水泵控制引脚初始化失败";
    }

    // 初始化GPIO3_A1施药泵控制引脚
    if (!initializeFertilizerPumpPin()) {
        qWarning() << "GPIO3_A1施药泵控制引脚初始化失败";
    }

    return true;
}

void GPIOController::cleanup()
{
    if (!m_initialized) {
        return;
    }

    // 注销所有已导出的引脚
    for (auto it = m_exportedPins.begin(); it != m_exportedPins.end(); ++it) {
        if (it.value()) {
            unexportPin(it.key());
        }
    }

    m_exportedPins.clear();
    m_initialized = false;
}

bool GPIOController::exportPin(int pin)
{
    if (!m_initialized) {
        emit errorOccurred("GPIO控制器未初始化");
        return false;
    }

    // 检查引脚是否已经导出
    QString pinPath = QString("%1/gpio%2").arg(GPIO_BASE_PATH).arg(pin);
    QDir pinDir(pinPath);
    if (pinDir.exists()) {
        m_exportedPins[pin] = true;
        return true;
    }

    // 导出引脚
    if (writeToFile(GPIO_EXPORT_PATH, QString::number(pin))) {
        m_exportedPins[pin] = true;
        return true;
    } else {
        emit errorOccurred(QString("GPIO引脚%1导出失败").arg(pin));
        return false;
    }
}

bool GPIOController::unexportPin(int pin)
{
    if (!m_initialized || !m_exportedPins.value(pin, false)) {
        return true;
    }

    if (writeToFile(GPIO_UNEXPORT_PATH, QString::number(pin))) {
        m_exportedPins[pin] = false;
        return true;
    } else {
        qWarning() << QString("GPIO引脚%1注销失败").arg(pin);
        return false;
    }
}

bool GPIOController::setDirection(int pin, const QString &direction)
{
    if (!isPinExported(pin)) {
        emit errorOccurred(QString("GPIO引脚%1未导出").arg(pin));
        return false;
    }

    QString directionPath = getPinPath(pin, "direction");
    if (writeToFile(directionPath, direction)) {
        return true;
    } else {
        emit errorOccurred(QString("GPIO引脚%1方向设置失败").arg(pin));
        return false;
    }
}

bool GPIOController::setPin(int pin, bool value)
{
    if (!isPinExported(pin)) {
        emit errorOccurred(QString("GPIO引脚%1未导出").arg(pin));
        return false;
    }

    QString valuePath = getPinPath(pin, "value");
    QString valueStr = value ? "1" : "0";

    if (writeToFile(valuePath, valueStr)) {
        return true;
    } else {
        emit errorOccurred(QString("GPIO引脚%1电平设置失败").arg(pin));
        return false;
    }
}

bool GPIOController::getPin(int pin)
{
    if (!isPinExported(pin)) {
        return false;
    }

    QString valuePath = getPinPath(pin, "value");
    QString value = readFromFile(valuePath);
    return value.trimmed() == "1";
}

bool GPIOController::isPinExported(int pin) const
{
    return m_exportedPins.value(pin, false);
}

bool GPIOController::writeToFile(const QString &filePath, const QString &value)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << value;
        file.close();
        return true;
    } else {
        QString errorMsg = QString("无法写入文件 %1: %2").arg(filePath).arg(file.errorString());
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }
}

QString GPIOController::readFromFile(const QString &filePath)
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

QString GPIOController::getPinPath(int pin, const QString &attribute)
{
    return QString("%1/gpio%2/%3").arg(GPIO_BASE_PATH).arg(pin).arg(attribute);
}

bool GPIOController::initializePowerSupplyPin()
{
    // 导出GPIO3_B6引脚
    if (!exportPin(POWER_SUPPLY_PIN)) {
        return false;
    }

    // 设置为输出模式
    if (!setDirection(POWER_SUPPLY_PIN, "out")) {
        return false;
    }

    // 设置为高电平（3.3V输出）
    if (!setPin(POWER_SUPPLY_PIN, GPIO_HIGH)) {
        return false;
    }

    qDebug() << "GPIO3_B6电源引脚初始化成功，设置为常高电平";
    return true;
}

bool GPIOController::initializePumpControlPin()
{
    // 导出GPIO3_A7引脚
    if (!exportPin(PUMP_CONTROL_PIN)) {
        return false;
    }

    // 设置为输出模式
    if (!setDirection(PUMP_CONTROL_PIN, "out")) {
        return false;
    }

    // 初始状态设置为低电平（水泵关闭）
    if (!setPin(PUMP_CONTROL_PIN, GPIO_LOW)) {
        return false;
    }

    qDebug() << "GPIO3_A7水泵控制引脚初始化成功，初始状态为关闭";
    return true;
}

bool GPIOController::startPump()
{
    if (!m_initialized) {
        emit errorOccurred("GPIO控制器未初始化");
        return false;
    }

    // 设置GPIO3_A7为高电平（开启水泵）
    if (setPin(PUMP_CONTROL_PIN, GPIO_HIGH)) {
        qDebug() << "水泵已开启 - GPIO3_A7置1";
        return true;
    } else {
        emit errorOccurred("水泵开启失败");
        return false;
    }
}

bool GPIOController::stopPump()
{
    if (!m_initialized) {
        emit errorOccurred("GPIO控制器未初始化");
        return false;
    }

    // 设置GPIO3_A7为低电平（关闭水泵）
    if (setPin(PUMP_CONTROL_PIN, GPIO_LOW)) {
        qDebug() << "水泵已关闭 - GPIO3_A7置0";
        return true;
    } else {
        emit errorOccurred("水泵关闭失败");
        return false;
    }
}

bool GPIOController::getPumpStatus()
{
    if (!m_initialized) {
        return false;
    }

    // 读取GPIO3_A7的状态
    return getPin(PUMP_CONTROL_PIN);
}

bool GPIOController::initializeFertilizerPumpPin()
{
    // 导出GPIO3_A1引脚
    if (!exportPin(FERTILIZER_PUMP_PIN)) {
        return false;
    }

    // 设置为输出模式
    if (!setDirection(FERTILIZER_PUMP_PIN, "out")) {
        return false;
    }

    // 初始状态设置为低电平（关闭）
    if (!setPin(FERTILIZER_PUMP_PIN, GPIO_LOW)) {
        return false;
    }

    qDebug() << "GPIO3_A1施药泵控制引脚初始化成功";
    return true;
}

bool GPIOController::startFertilizerPump()
{
    if (!m_initialized) {
        emit errorOccurred("GPIO控制器未初始化");
        return false;
    }

    // 设置GPIO3_A1为高电平（开启施药泵）
    if (setPin(FERTILIZER_PUMP_PIN, GPIO_HIGH)) {
        qDebug() << "施药泵已开启 - GPIO3_A1置1";
        return true;
    } else {
        emit errorOccurred("施药泵开启失败");
        return false;
    }
}

bool GPIOController::stopFertilizerPump()
{
    if (!m_initialized) {
        emit errorOccurred("GPIO控制器未初始化");
        return false;
    }

    // 设置GPIO3_A1为低电平（关闭施药泵）
    if (setPin(FERTILIZER_PUMP_PIN, GPIO_LOW)) {
        qDebug() << "施药泵已关闭 - GPIO3_A1置0";
        return true;
    } else {
        emit errorOccurred("施药泵关闭失败");
        return false;
    }
}

bool GPIOController::getFertilizerPumpStatus()
{
    if (!m_initialized) {
        return false;
    }

    // 读取GPIO3_A1的状态
    return getPin(FERTILIZER_PUMP_PIN);
}
