#include "core/mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QTimeZone>
#include <QLocale>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 强制设置北京时间时区环境变量
    qputenv("TZ", "Asia/Shanghai");
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));

    // 设置北京时区
    QTimeZone beijingTimeZone("Asia/Shanghai");
    QDateTime beijingTime = QDateTime::currentDateTime().toTimeZone(beijingTimeZone);

    // 设置全局字体 - 使用Linux兼容字体
    QFont font("DejaVu Sans", 10); // Linux系统通用字体
    QApplication::setFont(font);

    MainWindow w;
    w.setWindowTitle("基于RK3588的智能温室大棚控制系统");
    w.resize(1024, 573);

    // 显示窗口（正常大小，不最大化）
    w.show();

    return a.exec();
}
