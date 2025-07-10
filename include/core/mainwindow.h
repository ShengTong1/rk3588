#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QJsonObject>

// 前向声明
class QTimer;
class UIManager;
class PWMController;
class GPIOController;
class CurtainController;
class YOLOv8Integration;
class WeatherService;
class MqttService;
class WindowManager;
class AHT20Sensor;
class GY30LightSensor;
class AIDecisionManager;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief 主窗口类 - 智能温室大棚控制系统核心
 *
 * 负责整个系统的初始化、协调各功能模块
 * 基于QMainWindow架构，支持完整的窗口操作
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override; // 键盘事件处理

private slots:
    void updateTime(); // 时间更新槽函数
    void handleCloudCommand(const QJsonObject &cmd); // 处理云端控制指令
    void collectDeviceData(); // 收集设备数据并上报
    void updateSoilHumidityDisplay(double humidity); // 更新土壤湿度显示
    void onNewActionButtonClicked(); // 新功能按键点击槽函数

private:
    // 内部方法
    void initializeModules(); // 初始化所有模块
    void setupConnections();  // 设置信号连接
    void setupLogging();      // 初始化日志系统
    void syncPWMSliderValue(); // 同步PWM滑块值
    void updateWeatherWidgets(QWidget *container, const QJsonObject &data); // 更新天气控件

    // UI组件
    Ui::MainWindow *ui;
    QTimer *m_timer;

    // 功能模块管理器
    UIManager *m_uiManager;           // UI界面管理
    PWMController *m_pwmController;   // PWM补光灯控制
    GPIOController *m_gpioController; // GPIO控制器
    CurtainController *m_curtainController; // 保温帘控制
    YOLOv8Integration *m_yoloIntegration;   // YOLOv8集成
    WeatherService *m_weatherService;       // 天气服务
    MqttService *m_mqttService;             // MQTT阿里云服务
    WindowManager *m_windowManager;         // 窗口管理
    AHT20Sensor *m_aht20Sensor;            // AHT20温湿度传感器
    GY30LightSensor *m_gy30Sensor;         // GY30光照传感器（I2C7）
    AIDecisionManager *m_aiDecisionManager; // AI智能决策管理器
};

#endif // MAINWINDOW_H
