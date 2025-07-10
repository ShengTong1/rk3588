#include "system/window_manager.h"

#include <QMainWindow>
#include <QKeyEvent>
#include <QDebug>
#include <QSizePolicy>
#include <QRect>

WindowManager::WindowManager(QMainWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
    if (!m_window) {
        qWarning() << "窗口管理器: 主窗口指针为空";
    }

    qDebug() << "窗口管理器创建完成";
}

WindowManager::~WindowManager()
{
    qDebug() << "窗口管理器已销毁";
}

void WindowManager::toggleFullscreen()
{
    if (!m_window) return;

    if (isFullScreen()) {
        m_window->showMaximized();
        logWindowOperation("退出全屏模式");
    } else {
        m_window->showFullScreen();
        logWindowOperation("进入全屏模式");
    }

    emit fullscreenToggled(isFullScreen());
    emit windowStateChanged();
}

void WindowManager::toggleMaximize()
{
    if (!m_window) return;

    if (isMaximized()) {
        m_window->showNormal();
        logWindowOperation("窗口还原");
    } else {
        m_window->showMaximized();
        logWindowOperation("窗口最大化");
    }

    emit windowStateChanged();
}

void WindowManager::minimizeWindow()
{
    if (!m_window) return;

    m_window->showMinimized();
    logWindowOperation("窗口最小化");
    emit windowStateChanged();
}

void WindowManager::restoreWindow()
{
    if (!m_window) return;

    m_window->showNormal();
    m_window->raise();
    m_window->activateWindow();
    logWindowOperation("窗口恢复");
    emit windowStateChanged();
}

bool WindowManager::handleKeyPress(QKeyEvent *event)
{
    if (!event) return false;

    // F11: 切换全屏
    if (event->key() == Qt::Key_F11) {
        toggleFullscreen();
        return true;
    }

    // Ctrl+M: 切换最大化
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_M) {
        toggleMaximize();
        return true;
    }

    // Ctrl+H: 最小化窗口
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_H) {
        minimizeWindow();
        return true;
    }

    // ESC: 退出全屏（如果在全屏模式）
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        m_window->showMaximized();
        logWindowOperation("ESC退出全屏模式");
        emit fullscreenToggled(false);
        emit windowStateChanged();
        return true;
    }

    return false; // 未处理的按键
}

bool WindowManager::isFullScreen() const
{
    return m_window ? m_window->isFullScreen() : false;
}

bool WindowManager::isMaximized() const
{
    return m_window ? m_window->isMaximized() : false;
}

bool WindowManager::isMinimized() const
{
    return m_window ? m_window->isMinimized() : false;
}

void WindowManager::setWindowFlags()
{
    if (!m_window) return;

    // 设置完整的窗口标志，确保包含最大化和最小化按钮
    Qt::WindowFlags flags = Qt::Window |
                           Qt::WindowTitleHint |
                           Qt::WindowSystemMenuHint |
                           Qt::WindowMinimizeButtonHint |
                           Qt::WindowMaximizeButtonHint |
                           Qt::WindowCloseButtonHint;

    // 保存当前窗口状态
    bool wasVisible = m_window->isVisible();
    QRect geometry = m_window->geometry();

    // 设置窗口标志
    m_window->setWindowFlags(flags);

    // 确保窗口可以调整大小（这对最大化按钮很重要）
    m_window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 恢复窗口几何和可见性
    if (wasVisible) {
        m_window->setGeometry(geometry);
        m_window->show();
    }

    qDebug() << "窗口标志设置完成，包含最大化和最小化按钮";
}

void WindowManager::setWindowStyle()
{
    if (!m_window) return;

    // 设置主窗口样式，去除圆角，确保完全覆盖屏幕
    m_window->setStyleSheet("QMainWindow { border-radius: 0px; }");

    qDebug() << "窗口样式设置完成";
}

void WindowManager::logWindowOperation(const QString &operation)
{
    qDebug() << "窗口操作:" << operation;
}
