#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QKeyEvent;
QT_END_NAMESPACE

/**
 * @brief 窗口管理器
 * 
 * 负责管理窗口操作、快捷键处理和窗口状态
 * 提供全屏、最大化、最小化等功能
 */
class WindowManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowManager(QMainWindow *window, QObject *parent = nullptr);
    ~WindowManager();

    // 窗口操作接口
    void toggleFullscreen();             // 切换全屏模式
    void toggleMaximize();               // 切换最大化
    void minimizeWindow();               // 最小化窗口
    void restoreWindow();                // 恢复窗口

    // 快捷键处理
    bool handleKeyPress(QKeyEvent *event); // 处理按键事件

    // 窗口状态查询
    bool isFullScreen() const;
    bool isMaximized() const;
    bool isMinimized() const;

    // 配置接口
    void setWindowFlags();               // 设置窗口标志
    void setWindowStyle();               // 设置窗口样式

signals:
    void windowStateChanged();           // 窗口状态改变信号
    void fullscreenToggled(bool fullscreen); // 全屏状态切换信号

private:
    QMainWindow *m_window;               // 主窗口引用

    // 内部功能函数
    void logWindowOperation(const QString &operation); // 记录窗口操作
};

#endif // WINDOW_MANAGER_H
