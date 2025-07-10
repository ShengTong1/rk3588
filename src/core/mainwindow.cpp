#include "core/mainwindow.h"
#include "ui_mainwindow.h"
#include "config/aliyun_config.h"
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QDateTime>

// Qt模块
#include <QMessageBox>
#include <QProcess>

// 功能模块
#include "ui/ui_manager.h"
#include "hardware/pwm_controller.h"
#include "hardware/gpio_controller.h"
#include "hardware/gy30_sensor.h" // AHT20传感器
#include "hardware/gy30_light_sensor.h" // GY30光照传感器
#include "device/curtain_controller.h"
#include "ai/ai_decision_manager.h"
#include "integration/yolov8_integration.h"
#include "network/weather_service.h"
#include "network/mqtt_service.h"
#include "system/window_manager.h"

// Qt核心
#include <QDateTime>
#include <QTimeZone>
#include <QTimer>

#include <QFont>
#include <QApplication>
#include <QSlider>
#include <QLabel>
#include <QJsonObject>

// 全局日志文件指针
static QFile *logFile = nullptr;
static QTextStream *logStream = nullptr;

// 自定义消息处理函数
void messageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString typeStr;

    switch (type) {
    case QtDebugMsg:    typeStr = "DEBUG"; break;
    case QtWarningMsg:  typeStr = "WARN "; break;
    case QtCriticalMsg: typeStr = "ERROR"; break;
    case QtFatalMsg:    typeStr = "FATAL"; break;
    case QtInfoMsg:     typeStr = "INFO "; break;
    }

    QString formattedMsg = QString("[%1] %2: %3").arg(timestamp, typeStr, msg);

    // 输出到控制台
    fprintf(stderr, "%s\n", formattedMsg.toLocal8Bit().constData());

    // 输出到文件
    if (logStream) {
        *logStream << formattedMsg << Qt::endl;
        logStream->flush();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_timer(new QTimer(this))
    , m_uiManager(nullptr)
    , m_pwmController(nullptr)
    , m_curtainController(nullptr)
    , m_yoloIntegration(nullptr)
    , m_weatherService(nullptr)
    , m_mqttService(nullptr)
    , m_windowManager(nullptr)
    , m_aht20Sensor(nullptr)
    , m_gy30Sensor(nullptr)
    , m_aiDecisionManager(nullptr)
{
    ui->setupUi(this);

    // 初始化日志系统
    setupLogging();

    // 初始化所有功能模块
    initializeModules();

    // 设置信号连接
    setupConnections();

    // 启动定时器 - 设置为精确的1秒间隔
    m_timer->setTimerType(Qt::PreciseTimer); // 使用精确定时器
    m_timer->start(1000); // 1秒更新一次时间

    // 立即更新一次时间显示
    updateTime();

    // 系统初始化完成
}

MainWindow::~MainWindow()
{
    // 清理资源
    if (m_pwmController) {
        m_pwmController->cleanup();
    }

    // 清理日志系统
    if (logStream) {
        delete logStream;
        logStream = nullptr;
    }
    if (logFile) {
        logFile->close();
        delete logFile;
        logFile = nullptr;
    }

    delete ui;
}

void MainWindow::initializeModules()
{
    // 1. 初始化窗口管理器
    m_windowManager = new WindowManager(this, this);
    m_windowManager->setWindowFlags();
    m_windowManager->setWindowStyle();

    // 2. 初始化PWM控制器
    m_pwmController = new PWMController(this);
    if (!m_pwmController->initialize()) {
        qWarning() << "PWM控制器初始化失败";
    }

    // 3. 初始化MQTT阿里云服务
    m_mqttService = new MqttService(this);
    m_mqttService->setAutoReconnect(true);
    m_mqttService->setReportInterval(10); // 10秒上报一次数据
    m_mqttService->setHeartbeatInterval(300); // 5分钟心跳间隔

    // 4. 初始化GPIO控制器
    m_gpioController = new GPIOController(this);
    if (!m_gpioController->initialize()) {
        qWarning() << "GPIO控制器初始化失败";
    }

    // 5. 初始化保温帘控制器
    m_curtainController = new CurtainController(this);
    m_curtainController->setGPIOController(m_gpioController);
    if (!m_curtainController->initialize()) {
        qWarning() << "保温帘控制器初始化失败";
    }

    // 6. 初始化UI管理器并设置控制器
    m_uiManager = new UIManager(this);
    m_uiManager->setPWMController(m_pwmController);      // 在UI初始化前设置
    m_uiManager->setMqttService(m_mqttService);          // 在UI初始化前设置
    m_uiManager->setCurtainController(m_curtainController); // 在UI初始化前设置
    m_uiManager->setGPIOController(m_gpioController);    // 在UI初始化前设置
    m_uiManager->setupMainPage(ui->mainPage);
    m_uiManager->initializeSubPages(ui->stackedWidget);

    // 7. 初始化YOLOv8集成
    m_yoloIntegration = new YOLOv8Integration(this, this);
    m_yoloIntegration->setYOLOv8Path("../yolov8_pyqt_modified");
    // 不再设置窗口切换延时，PyQt程序直接在上层显示

    // 8. 初始化天气服务
    m_weatherService = new WeatherService(this);
    m_weatherService->setCity("沈阳");
    m_weatherService->setApiKey("S21KbWeZdTz-wOoqI");
    m_weatherService->setAutoUpdate(true, 30); // 30分钟自动更新
    m_weatherService->fetchWeatherData(); // 立即获取天气数据

    // 9. 同步PWM滑块初始值
    syncPWMSliderValue();

    // 延时启动MQTT连接，确保其他模块初始化完成
    QTimer::singleShot(2000, this, [this]() {
        if (m_mqttService) {
            qDebug() << "=== 启动MQTT服务 ===";
            bool mqttStarted = m_mqttService->connectToAliyun();
            qDebug() << "MQTT服务启动结果:" << (mqttStarted ? "成功" : "失败");
            qDebug() << "===================";
        } else {
            qWarning() << "MQTT服务未初始化！";
        }
    });

    // 10. 初始化AHT20温湿度传感器
    m_aht20Sensor = new AHT20Sensor(this);
    if (m_aht20Sensor->initialize()) {
        qDebug() << "AHT20温湿度传感器初始化成功";
    } else {
        qWarning() << "AHT20温湿度传感器初始化失败";
    }

    // 11. 初始化GY30光照传感器（I2C7）
    m_gy30Sensor = new GY30LightSensor(this);
    if (m_gy30Sensor->initialize()) {
        qDebug() << "GY30光照传感器初始化成功";
    } else {
        qWarning() << "GY30光照传感器初始化失败";
    }

    // 12. 初始化AI智能决策管理器
    m_aiDecisionManager = new AIDecisionManager(this);
    m_aiDecisionManager->setCurtainController(m_curtainController);
    m_aiDecisionManager->setLightSensor(m_gy30Sensor);
    if (m_aiDecisionManager->initialize()) {
        qDebug() << "AI智能决策管理器初始化成功";
    } else {
        qWarning() << "AI智能决策管理器初始化失败";
    }

    // 将AI管理器设置到UI管理器
    m_uiManager->setAIDecisionManager(m_aiDecisionManager);
}

void MainWindow::setupConnections()
{
    // 时间更新连接
    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateTime);

    // UI管理器连接
    if (m_uiManager) {
        // 主页面按钮连接
        connect(ui->btn1, &QPushButton::clicked, [this]() {
            m_uiManager->switchToPage(ui->stackedWidget, 1);
        });
        connect(ui->btn2, &QPushButton::clicked, [this]() {
            m_uiManager->switchToPage(ui->stackedWidget, 2);
        });
        connect(ui->btn3, &QPushButton::clicked, [this]() {
            if (m_yoloIntegration) {
                m_yoloIntegration->launchDetection();
            }
        });
        connect(ui->btn4, &QPushButton::clicked, [this]() {
            m_uiManager->switchToPage(ui->stackedWidget, 4);
        });
        connect(ui->btn5, &QPushButton::clicked, [this]() {
            m_uiManager->switchToPage(ui->stackedWidget, 5);
        });
        connect(ui->btn6, &QPushButton::clicked, [this]() {
            // 智能灌溉系统 - 跳转到灌溉控制页面
            m_uiManager->switchToPage(ui->stackedWidget, 6);
        });

        // 新功能按键连接
        connect(ui->newActionButton, &QPushButton::clicked, this, &MainWindow::onNewActionButtonClicked);



        // 连接返回按钮信号 - 修复返回按键失效问题
        connect(m_uiManager, &UIManager::pageChanged, [this](int index) {
            if (index >= 0 && index < ui->stackedWidget->count()) {
                ui->stackedWidget->setCurrentIndex(index);
                qDebug() << "页面切换到:" << index;
            }
        });

        // 连接动态创建的按钮信号 - 在UI初始化完成后
        QTimer::singleShot(100, [this]() {
            // 查找动态创建的按钮并连接信号
            QPushButton *btn4 = ui->mainPage->findChild<QPushButton*>("btn4");
            QPushButton *btn5 = ui->mainPage->findChild<QPushButton*>("btn5");
            // QPushButton *btn6 = ui->mainPage->findChild<QPushButton*>("btn6"); // 已在上方连接

            if (btn4) {
                connect(btn4, &QPushButton::clicked, [this]() {
                    m_uiManager->switchToPage(ui->stackedWidget, 4);
                });
            }
            if (btn5) {
                connect(btn5, &QPushButton::clicked, [this]() {
                    m_uiManager->switchToPage(ui->stackedWidget, 5);
                });
            }
            // btn6灌溉系统按钮已在上方连接，避免重复连接
        });

    }

    // PWM控制器连接
    if (m_pwmController) {
        connect(m_pwmController, &PWMController::errorOccurred,
                [](const QString &error) {
                    Q_UNUSED(error)
                });
    }

    // 保温帘控制器连接
    if (m_curtainController) {
        connect(m_curtainController, &CurtainController::statusUpdated,
                [](const QString &status) {
                    Q_UNUSED(status)
                });
    }

    // YOLOv8集成连接
    if (m_yoloIntegration) {
        connect(m_yoloIntegration, &YOLOv8Integration::detectionStarted,
                []() { });
        connect(m_yoloIntegration, &YOLOv8Integration::detectionFinished,
                []() { });
        connect(m_yoloIntegration, &YOLOv8Integration::errorOccurred,
                [](const QString &error) {
                    Q_UNUSED(error)
                });
    }

    // 天气服务连接
    if (m_weatherService) {
        // 实时天气数据更新
        connect(m_weatherService, &WeatherService::weatherDataUpdated,
                [this](const WeatherService::WeatherData &data) {
                    if (data.isValid) {
                        // 将WeatherData转换为QJsonObject
                        QJsonObject dataObj;
                        dataObj["temperature"] = data.temperature;
                        dataObj["description"] = data.description;
                        dataObj["humidity"] = data.humidity;
                        dataObj["windSpeed"] = data.windSpeed;
                        dataObj["pressure"] = data.pressure;
                        dataObj["feelLike"] = data.feelLike;
                        dataObj["precipitation"] = data.precipitation;
                        dataObj["isValid"] = data.isValid;

                        // 更新主页面的天气控件
                        updateWeatherWidgets(ui->mainPage, dataObj);

                        // 更新天气信息页面的天气控件（如果存在）
                        if (ui->stackedWidget->count() > 4) {
                            QWidget *weatherPage = ui->stackedWidget->widget(4);
                            if (weatherPage) {
                                updateWeatherWidgets(weatherPage, dataObj);
                            }
                        }

                        // 更新原始weatherLabel（如果可见）
                        if (ui->weatherLabel && !ui->weatherLabel->isHidden()) {
                            QString weatherText = m_weatherService->formatWeatherDisplay(data);
                            ui->weatherLabel->setText(weatherText);
                        }

                        qDebug() << "天气信息已更新到所有页面 - 温度:" << data.temperature << "°C, 天气:" << data.description;
                    }
                });



        // 天气预警数据更新
        connect(m_weatherService, &WeatherService::warningUpdated,
                [this](const WeatherService::WeatherWarning &warning) {
                    if (warning.isValid) {
                        QString warningText;
                        if (warning.level == "无" || warning.title == "暂无预警信息") {
                            warningText = "暂无预警信息";
                        } else {
                            // 显示完整预警信息，允许换行
                            warningText = QString("%1\n%2级").arg(warning.title).arg(warning.level);
                        }

                        // 更新主页面的预警信息
                        QLabel *warningLabel = ui->mainPage->findChild<QLabel*>("warningValue");
                        if (warningLabel) {
                            warningLabel->setText(warningText);
                        }

                        // 更新天气信息页面的预警信息
                        if (ui->stackedWidget->count() > 4) {
                            QWidget *weatherPage = ui->stackedWidget->widget(4);
                            if (weatherPage) {
                                QLabel *weatherPageWarning = weatherPage->findChild<QLabel*>("warningValue");
                                if (weatherPageWarning) {
                                    weatherPageWarning->setText(warningText);
                                }
                            }
                        }

                        qDebug() << "预警信息已更新到所有页面";
                    }
                });

        // 降水预报数据更新
        connect(m_weatherService, &WeatherService::precipitationUpdated,
                [this](const WeatherService::PrecipitationForecast &forecast) {
                    if (forecast.isValid) {
                        QString precipitationText = QString("未来2小时: %1\n概率: %2")
                                                   .arg(forecast.summary)
                                                   .arg(forecast.probability);

                        // 更新主页面的降水预报信息
                        QLabel *precipitationLabel = ui->mainPage->findChild<QLabel*>("precipitationInfo");
                        if (precipitationLabel) {
                            precipitationLabel->setText(precipitationText);
                        }

                        // 更新主页面的降水量显示
                        QLabel *precipValue = ui->mainPage->findChild<QLabel*>("precipValue");
                        if (precipValue) {
                            precipValue->setText(forecast.probability);
                        }

                        // 更新天气信息页面的降水量显示
                        if (ui->stackedWidget->count() > 4) {
                            QWidget *weatherPage = ui->stackedWidget->widget(4);
                            if (weatherPage) {
                                QLabel *weatherPagePrecip = weatherPage->findChild<QLabel*>("precipValue");
                                if (weatherPagePrecip) {
                                    weatherPagePrecip->setText(forecast.probability);
                                }
                            }
                        }

                        qDebug() << "降水预报信息已更新到所有页面";
                    }
                });

        // 更新失败处理
        connect(m_weatherService, &WeatherService::updateFailed,
                [this](const QString &error) {
                    ui->weatherLabel->setText("天气获取失败: " + error);
                    qWarning() << "天气更新失败:" << error;
                });
    }

    // MQTT服务连接
    if (m_mqttService) {
        // 连接状态变化
        connect(m_mqttService, &MqttService::connectionStateChanged,
                [this](MqttService::ConnectionState state) {
                    QString stateText;
                    switch (state) {
                    case MqttService::Disconnected:
                        stateText = "已断开";
                        break;
                    case MqttService::Connecting:
                        stateText = "连接中";
                        break;
                    case MqttService::Connected:
                        stateText = "已连接";
                        break;
                    case MqttService::Reconnecting:
                        stateText = "重连中";
                        break;
                    }
                    qDebug() << "MQTT连接状态:" << stateText;
                });

        // 数据发布结果
        connect(m_mqttService, &MqttService::deviceDataPublished,
                [](bool success) {
                    if (success) {
                        qDebug() << "设备数据上报成功";
                    } else {
                        qWarning() << "设备数据上报失败";
                    }
                });

        // 收到控制指令
        connect(m_mqttService, &MqttService::controlCommandReceived,
                [this](const MqttService::ControlCommand &cmd) {
                    handleCloudCommand(cmd.parameters);
                });

        // 收到土壤湿度数据
        connect(m_mqttService, &MqttService::soilHumidityReceived,
                [this](double humidity) {
                    updateSoilHumidityDisplay(humidity);
                });

        // 配置验证和测试代码已移除

        // MQTT错误处理
        connect(m_mqttService, &MqttService::errorOccurred,
                [](const QString &error) {
                    qWarning() << "MQTT错误:" << error;
                });

        // 心跳发送
        connect(m_mqttService, &MqttService::heartbeatSent,
                []() {
                    // 心跳发送成功
                });

        // 数据收集请求
        connect(m_mqttService, &MqttService::dataCollectionRequested,
                this, &MainWindow::collectDeviceData);
    }

    // AHT20温湿度传感器连接
    if (m_aht20Sensor) {
        connect(m_aht20Sensor, &AHT20Sensor::dataChanged,
                [this](float temperature, float humidity) {
                    // 只更新大棚实时信息页面的传感器数据
                    if (ui->stackedWidget->count() > 5) {
                        QWidget *greenhousePage = ui->stackedWidget->widget(5);
                        if (greenhousePage) {
                            // 更新温度显示
                            QLabel *tempLabel = greenhousePage->findChild<QLabel*>("tempHumLabel");
                            if (tempLabel) {
                                tempLabel->setText(QString("%1°C").arg(temperature, 0, 'f', 1));
                            }

                            // 更新湿度显示
                            QLabel *humidityLabel = greenhousePage->findChild<QLabel*>("humidityLabel");
                            if (humidityLabel) {
                                humidityLabel->setText(QString("%1%").arg(humidity, 0, 'f', 1));
                            }
                        }
                    }
                });

        // 信号连接完成后启动传感器读取
        m_aht20Sensor->startReading(3000); // 3秒间隔读取，立即执行一次
    }

    // GY30光照传感器连接
    if (m_gy30Sensor) {
        connect(m_gy30Sensor, &GY30LightSensor::luxValueChanged,
                [this](float lux) {
                    // 只更新大棚实时信息页面的光照数据
                    if (ui->stackedWidget->count() > 5) {
                        QWidget *greenhousePage = ui->stackedWidget->widget(5);
                        if (greenhousePage) {
                            QLabel *greenhouseLuxLabel = greenhousePage->findChild<QLabel*>("luxLabel");
                            if (greenhouseLuxLabel) {
                                greenhouseLuxLabel->setText(QString("%1 lx").arg(lux, 0, 'f', 1));
                                qDebug() << "GY30光照数据已更新到大棚实时信息页面:" << lux << "lx";
                            }
                        }
                    }
                });

        // 信号连接完成后启动传感器读取
        m_gy30Sensor->startReading(2000); // 2秒间隔读取，立即执行一次
        qDebug() << "GY30传感器开始读取数据";
    }

    // AI智能决策管理器连接
    if (m_aiDecisionManager && m_uiManager) {
        // 连接手动控制锁定信号
        connect(m_aiDecisionManager, &AIDecisionManager::manualControlLocked,
                m_uiManager, &UIManager::lockManualCurtainControls);

        // 连接错误信号
        connect(m_aiDecisionManager, &AIDecisionManager::errorOccurred,
                [this](const QString &error) {
                    qWarning() << "AI决策错误:" << error;
                });

        qDebug() << "AI智能决策管理器信号连接完成";
    }
}

void MainWindow::updateTime()
{
    // 使用静态时区对象，避免重复创建
    static const QTimeZone beijingTimeZone("Asia/Shanghai");

    // 获取当前时间并转换为北京时间
    QDateTime beijingDateTime = QDateTime::currentDateTime().toTimeZone(beijingTimeZone);

    // 格式化时间显示 - 使用中文格式
    QString currentTime = beijingDateTime.toString("yyyy年MM月dd日 hh:mm:ss");

    // 使用静态数组优化星期显示
    static const QString weekDays[] = {
        "", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日"
    };

    int dayOfWeek = beijingDateTime.date().dayOfWeek();
    QString weekDay = (dayOfWeek >= 1 && dayOfWeek <= 7) ? weekDays[dayOfWeek] : "";

    // 组合完整的时间显示
    QString fullTimeText = QString("%1 %2").arg(currentTime, weekDay);
    ui->timeLabel->setText("当前时间: " + fullTimeText);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 委托给窗口管理器处理
    if (m_windowManager && m_windowManager->handleKeyPress(event)) {
        return; // 已处理
    }

    // 调用父类处理其他按键
    QMainWindow::keyPressEvent(event);
}

void MainWindow::handleCloudCommand(const QJsonObject &cmd)
{
    // 解析控制指令并执行相应操作
    if (cmd.contains("pwmDutyCycle") && m_pwmController) {
        int dutyCycle = cmd["pwmDutyCycle"].toInt();
        if (dutyCycle >= 0 && dutyCycle <= 100) {
            m_pwmController->setDutyCycle(dutyCycle);
        }
    }

    if (cmd.contains("curtainTopOpen") && m_curtainController) {
        bool open = cmd["curtainTopOpen"].toBool();
        if (open) {
            m_curtainController->openCurtain(CurtainController::TopCurtain);
        } else {
            m_curtainController->closeCurtain(CurtainController::TopCurtain);
        }
    }

    if (cmd.contains("curtainSideOpen") && m_curtainController) {
        bool open = cmd["curtainSideOpen"].toBool();
        if (open) {
            m_curtainController->openCurtain(CurtainController::SideCurtain);
        } else {
            m_curtainController->closeCurtain(CurtainController::SideCurtain);
        }

    }

    // 可以添加更多控制指令处理
    // 例如：温度设定、湿度控制、自动模式切换等
}

void MainWindow::collectDeviceData()
{
    if (!m_mqttService || !m_mqttService->isConnected()) {
        return;
    }

    // 收集设备数据
    MqttService::DeviceData data;

    // 初始化默认值，避免传感器故障时上报0值
    data.temperature = 25.0;    // 默认温度25°C
    data.humidity = 50.0;       // 默认湿度50%
    data.lightIntensity = 500;  // 默认光照500lux
    data.pwmDutyCycle = 50;     // 默认PWM 50%

    // 从AHT20传感器获取温湿度数据
    if (m_aht20Sensor) {
        double temp = m_aht20Sensor->getCurrentTemperature();
        double hum = m_aht20Sensor->getCurrentHumidity();
        if (temp > -50 && temp < 100) data.temperature = temp; // 合理温度范围
        if (hum > 0 && hum <= 100) data.humidity = hum; // 合理湿度范围
    }

    // 光照数据从GY30传感器获取
    if (m_gy30Sensor) {
        data.lightIntensity = m_gy30Sensor->getCurrentLux();
    } else {
        data.lightIntensity = 500.0; // 传感器不可用时使用固定值
    }

    // 从PWM控制器获取占空比
    if (m_pwmController) {
        int pwm = m_pwmController->getCurrentDutyCycle();
        if (pwm >= 0 && pwm <= 100) data.pwmDutyCycle = pwm;
    }

    // 从保温帘控制器获取状态
    if (m_curtainController) {
        data.curtainTopOpen = (m_curtainController->getCurtainState(CurtainController::TopCurtain) == CurtainController::Open);
        data.curtainSideOpen = (m_curtainController->getCurtainState(CurtainController::SideCurtain) == CurtainController::Open);
    }

    // 设置时间戳和有效性
    data.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    data.isValid = true;

    // 发布数据到阿里云
    m_mqttService->publishDeviceData(data);


}

void MainWindow::syncPWMSliderValue()
{
    if (!m_pwmController) {
        return;
    }

    // 获取PWM当前占空比
    int currentDutyCycle = m_pwmController->getCurrentDutyCycle();

    // 查找PWM滑块控件
    QSlider *lightSlider = ui->stackedWidget->findChild<QSlider*>("lightSlider");
    if (lightSlider) {
        // 暂时断开信号连接，避免触发硬件设置
        lightSlider->blockSignals(true);
        lightSlider->setValue(currentDutyCycle);
        lightSlider->blockSignals(false);

        // 同步状态显示
        QLabel *lightStatusValue = ui->stackedWidget->findChild<QLabel*>("lightStatusValue");
        if (lightStatusValue) {
            lightStatusValue->setText(QString("%1%").arg(currentDutyCycle));
        }
    }
}

void MainWindow::updateWeatherWidgets(QWidget *container, const QJsonObject &data)
{
    if (!container || !data["isValid"].toBool()) {
        return;
    }

    // 更新温度显示
    QLabel *tempDisplay = container->findChild<QLabel*>("tempDisplay");
    if (tempDisplay) {
        tempDisplay->setText(data["temperature"].toString() + "°C");
    }

    // 更新天气描述
    QLabel *weatherDesc = container->findChild<QLabel*>("weatherDesc");
    if (weatherDesc) {
        weatherDesc->setText(data["description"].toString());
    }

    // 更新详细天气信息
    QLabel *humidityValue = container->findChild<QLabel*>("humidityValue");
    if (humidityValue) {
        humidityValue->setText(data["humidity"].toString() + "%");
    }

    QLabel *windSpeedValue = container->findChild<QLabel*>("windSpeedValue");
    if (windSpeedValue) {
        windSpeedValue->setText(data["windSpeed"].toString() + "km/h");
    }

    QLabel *pressureValue = container->findChild<QLabel*>("pressureValue");
    if (pressureValue) {
        pressureValue->setText(data["pressure"].toString() + "hPa");
    }

    QLabel *feelsLikeValue = container->findChild<QLabel*>("feelsLikeValue");
    if (feelsLikeValue) {
        feelsLikeValue->setText(data["feelLike"].toString() + "°C");
    }

    QLabel *precipValue = container->findChild<QLabel*>("precipValue");
    if (precipValue) {
        precipValue->setText(data["precipitation"].toString() + "mm");
    }

    // 更新最后更新时间
    QLabel *updateTimeLabel = container->findChild<QLabel*>("updateTimeLabel");
    if (updateTimeLabel) {
        QDateTime currentTime = QDateTime::currentDateTime();
        QString timeString = currentTime.toString("MM-dd hh:mm");
        updateTimeLabel->setText("最后更新时间：" + timeString);
    }

    // 更新预警信息（这个需要单独处理，因为它来自不同的信号）
    // 这里只处理基本天气数据，预警信息在warningUpdated信号中处理
}

void MainWindow::updateSoilHumidityDisplay(double humidity)
{
    // 更新环境数据监测页面的土壤湿度显示
    if (ui->stackedWidget->count() > 5) {
        QWidget *greenhousePage = ui->stackedWidget->widget(5);
        if (greenhousePage) {
            QLabel *soilHumidityLabel = greenhousePage->findChild<QLabel*>("soilMoistureLabel");
            if (soilHumidityLabel) {
                soilHumidityLabel->setText(QString::number(humidity, 'f', 1) + "%");
                // 土壤湿度显示更新成功
            } else {
                qWarning() << "未找到土壤湿度显示标签";
            }
        }
    }
}

void MainWindow::setupLogging()
{
    // 创建日志目录
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);

    // 创建日志文件
    QString logFileName = logDir + "/qt_mainwindow_debug.log";
    logFile = new QFile(logFileName);

    if (logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        logStream = new QTextStream(logFile);

        // 安装自定义消息处理器
        qInstallMessageHandler(messageOutput);

        qDebug() << "=== 日志系统初始化 ===";
        qDebug() << "日志文件路径:" << logFileName;
        qDebug() << "程序启动时间:" << QDateTime::currentDateTime().toString();
        qDebug() << "==================";
    } else {
        qWarning() << "无法创建日志文件:" << logFileName;
    }
}

void MainWindow::onNewActionButtonClicked()
{
    // 新功能按键点击处理
    qDebug() << "新功能按键被点击";

    // TODO: 在这里实现具体的功能
    // 用户后续会说明具体要实现什么功能

    // 临时显示一个消息提示
    // 可以根据后续需求修改为具体的功能实现
}