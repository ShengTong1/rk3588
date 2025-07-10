#ifndef YOLOV8_INTEGRATION_H
#define YOLOV8_INTEGRATION_H

#include <QObject>
#include <QString>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QTimer;
class QWidget;
QT_END_NAMESPACE

/**
 * @brief YOLOv8检测系统集成管理器
 *
 * 负责管理外部YOLOv8 PyQt应用的启动、窗口切换和进程管理
 * 实现智能窗口管理，避免界面遮挡和桌面闪现
 */
class YOLOv8Integration : public QObject
{
    Q_OBJECT

public:
    explicit YOLOv8Integration(QWidget *parentWindow, QObject *parent = nullptr);
    ~YOLOv8Integration();

    // 主要功能接口
    void launchDetection();              // 启动YOLOv8检测系统
    void terminateDetection();           // 终止检测系统
    bool isRunning() const;              // 检查是否正在运行

    // 配置接口
    void setYOLOv8Path(const QString &path); // 设置YOLOv8路径
    void setWindowSwitchDelay(int milliseconds); // 设置窗口切换延时

signals:
    void detectionStarted();             // 检测启动信号
    void detectionFinished();            // 检测结束信号
    void errorOccurred(const QString &error); // 错误信号

private slots:
    void onProcessStarted();             // 进程启动处理
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus); // 进程结束处理
    void onProcessError(QProcess::ProcessError error); // 进程错误处理
    void onWindowSwitchTimer();          // 窗口切换定时器

private:
    // 组件
    QProcess *m_yoloProcess;             // YOLOv8进程
    QTimer *m_windowSwitchTimer;         // 窗口切换定时器
    QWidget *m_parentWindow;             // 父窗口

    // 配置参数
    QString m_yolov8Path;                // YOLOv8路径
    int m_windowSwitchDelay;             // 窗口切换延时(ms)

    // 内部功能函数
    bool validateYOLOv8Path() const;     // 验证YOLOv8路径
    void minimizeParentWindow();         // 最小化父窗口
    void restoreParentWindow();          // 恢复父窗口
    void setupProcessConnections();      // 设置进程信号连接
};

#endif // YOLOV8_INTEGRATION_H
