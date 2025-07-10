#include "integration/yolov8_integration.h"

#include <QProcess>
#include <QTimer>
#include <QWidget>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>

YOLOv8Integration::YOLOv8Integration(QWidget *parentWindow, QObject *parent)
    : QObject(parent)
    , m_yoloProcess(new QProcess(this))
    , m_windowSwitchTimer(new QTimer(this))
    , m_parentWindow(parentWindow)
    , m_yolov8Path("./pyqt")
    , m_windowSwitchDelay(3000) // 默认3秒延时
{
    setupProcessConnections();

    // 设置窗口切换定时器为单次触发
    m_windowSwitchTimer->setSingleShot(true);
    connect(m_windowSwitchTimer, &QTimer::timeout, this, &YOLOv8Integration::onWindowSwitchTimer);

    qDebug() << "YOLOv8集成管理器创建完成";
}

YOLOv8Integration::~YOLOv8Integration()
{
    terminateDetection();
}

void YOLOv8Integration::launchDetection()
{
    // 验证YOLOv8路径
    if (!validateYOLOv8Path()) {
        return;
    }

    // 如果进程已经在运行，先终止
    if (isRunning()) {
        terminateDetection();
    }

    // 断开所有之前的连接，避免重复连接
    m_yoloProcess->disconnect();
    setupProcessConnections();

    // 设置工作目录
    QDir yolov8Dir(m_yolov8Path);
    m_yoloProcess->setWorkingDirectory(yolov8Dir.absolutePath());

    // 启动YOLOv8检测系统
    QString program = "python3";
    QStringList arguments;
    arguments << "main.py";

    m_yoloProcess->start(program, arguments);
}

void YOLOv8Integration::terminateDetection()
{
    if (!isRunning()) {
        return;
    }

    qDebug() << "终止YOLOv8检测系统...";

    m_yoloProcess->kill();
    if (!m_yoloProcess->waitForFinished(3000)) {
        qWarning() << "YOLOv8进程强制终止超时";
    }
}

bool YOLOv8Integration::isRunning() const
{
    return m_yoloProcess->state() != QProcess::NotRunning;
}

void YOLOv8Integration::setYOLOv8Path(const QString &path)
{
    m_yolov8Path = path;
    qDebug() << "YOLOv8路径设置为:" << path;
}

void YOLOv8Integration::setWindowSwitchDelay(int milliseconds)
{
    m_windowSwitchDelay = milliseconds;
    qDebug() << "窗口切换延时设置为:" << milliseconds << "ms";
}

void YOLOv8Integration::onProcessStarted()
{
    qDebug() << "YOLOv8进程启动成功";
    emit detectionStarted();

    // 不再启动窗口切换定时器，让PyQt程序直接在上层显示
    // m_windowSwitchTimer->start(m_windowSwitchDelay);
}

void YOLOv8Integration::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    qDebug() << QString("YOLOv8进程结束 (退出码: %1)").arg(exitCode);
    emit detectionFinished();

    // 停止窗口切换定时器
    m_windowSwitchTimer->stop();

    // PyQt程序关闭后，原Qt界面自然显示，无需特殊处理
    qDebug() << "YOLOv8程序已关闭，原界面自动显示";
}

void YOLOv8Integration::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "启动失败：无法启动Python进程\n请确保系统已安装Python3";
            break;
        case QProcess::Crashed:
            errorMsg = "YOLOv8检测系统意外退出";
            break;
        case QProcess::Timedout:
            errorMsg = "YOLOv8检测系统启动超时";
            break;
        case QProcess::WriteError:
            errorMsg = "YOLOv8检测系统写入错误";
            break;
        case QProcess::ReadError:
            errorMsg = "YOLOv8检测系统读取错误";
            break;
        default:
            errorMsg = "启动YOLOv8检测系统时发生未知错误";
            break;
    }

    qWarning() << "YOLOv8进程错误:" << errorMsg;
    emit errorOccurred(errorMsg);

    // 显示错误消息
    if (m_parentWindow) {
        QMessageBox::critical(m_parentWindow, "错误", errorMsg);
    }

    // 错误时无需特殊处理窗口，原界面保持显示
    qDebug() << "YOLOv8启动失败，原界面保持显示";
}

void YOLOv8Integration::onWindowSwitchTimer()
{
    // 不再需要窗口切换逻辑，PyQt程序直接在上层显示
    qDebug() << "窗口切换定时器触发，但已禁用窗口切换功能";
}

bool YOLOv8Integration::validateYOLOv8Path() const
{
    QDir yolov8Dir(m_yolov8Path);

    if (!yolov8Dir.exists()) {
        QString error = QString("未找到YOLOv8检测系统目录\n请确保%1文件夹存在于项目根目录").arg(m_yolov8Path);
        qWarning() << error;

        if (m_parentWindow) {
            QMessageBox::warning(m_parentWindow, "启动失败", error);
        }

        const_cast<YOLOv8Integration*>(this)->errorOccurred(error);
        return false;
    }

    // 检查main.py文件
    QString mainPyPath = yolov8Dir.absoluteFilePath("main.py");
    if (!QFileInfo::exists(mainPyPath)) {
        QString error = "未找到YOLOv8主程序文件\n请确保main.py文件存在于pyqt目录中";
        qWarning() << error;

        if (m_parentWindow) {
            QMessageBox::warning(m_parentWindow, "启动失败", error);
        }

        const_cast<YOLOv8Integration*>(this)->errorOccurred(error);
        return false;
    }

    return true;
}

void YOLOv8Integration::minimizeParentWindow()
{
    if (m_parentWindow) {
        qDebug() << "最小化父窗口";
        m_parentWindow->showMinimized();
    }
}

void YOLOv8Integration::restoreParentWindow()
{
    if (m_parentWindow) {
        qDebug() << "恢复父窗口到最大化状态";
        m_parentWindow->showMaximized(); // 修改为最大化显示
        m_parentWindow->raise();
        m_parentWindow->activateWindow();

        // 短暂延时确保窗口完全激活并保持最大化
        QTimer::singleShot(50, [this]() {
            if (m_parentWindow) {
                m_parentWindow->showMaximized(); // 再次确保最大化
                m_parentWindow->raise();
                m_parentWindow->activateWindow();
            }
        });
    }
}

void YOLOv8Integration::setupProcessConnections()
{
    // 连接进程信号
    connect(m_yoloProcess, &QProcess::started,
            this, &YOLOv8Integration::onProcessStarted);

    connect(m_yoloProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &YOLOv8Integration::onProcessFinished);

    connect(m_yoloProcess, &QProcess::errorOccurred,
            this, &YOLOv8Integration::onProcessError);
}
