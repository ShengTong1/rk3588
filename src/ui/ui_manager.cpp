#include "ui/ui_manager.h"
#include "hardware/pwm_controller.h"
#include "hardware/gpio_controller.h"
#include "network/mqtt_service.h"
#include "device/curtain_controller.h"
#include "ai/ai_decision_manager.h"

// Qt界面组件
#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDebug>
#include <QApplication>

// 样式常量定义 - 农业主题
const QString UIManager::DEFAULT_FONT = "DejaVu Sans, Liberation Sans, sans-serif";
const QString UIManager::MAIN_BACKGROUND = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #e8f5e8, stop:1 #c8e6c8)"; // 绿色生态背景
const QString UIManager::BUTTON_BASE_STYLE =
    "QPushButton { "
    "   border: 2px solid #4CAF50; "
    "   border-radius: 16px; "
    "   padding: 24px 20px; "
    "   font-size: 16px; "
    "   font-weight: 600; "
    "   margin: 12px; "
    "   min-width: 160px; "
    "   min-height: 100px; "
    "   color: #2E7D32; "
    "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #f1f8e9); "
    "   font-family: '" + DEFAULT_FONT + "'; "
    "} "
    "QPushButton:hover { "
    "   border: 2px solid #2E7D32; "
    "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f1f8e9, stop:1 #dcedc8); "
    "} "
    "QPushButton:pressed { "
    "   border: 2px solid #1B5E20; "
    "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #dcedc8, stop:1 #c5e1a5); "
    "}";

UIManager::UIManager(QObject *parent)
    : QObject(parent)
    , m_pwmController(nullptr)
    , m_mqttService(nullptr)
    , m_curtainController(nullptr)
    , m_gpioController(nullptr)
    , m_aiDecisionManager(nullptr)
{
    qDebug() << "UI管理器初始化完成";
}

UIManager::~UIManager()
{
    qDebug() << "UI管理器已销毁";
}

void UIManager::setPWMController(PWMController *controller)
{
    m_pwmController = controller;
}

void UIManager::setMqttService(MqttService *service)
{
    m_mqttService = service;
}

void UIManager::setCurtainController(CurtainController *controller)
{
    m_curtainController = controller;
}

void UIManager::setGPIOController(GPIOController *controller)
{
    m_gpioController = controller;
}

void UIManager::setAIDecisionManager(AIDecisionManager *manager)
{
    m_aiDecisionManager = manager;
}

void UIManager::setupMainPage(QWidget *mainPage)
{
    if (!mainPage) {
        qWarning() << "主页面指针为空";
        return;
    }

    // 设置主页面样式
    QString mainStyle = QString(
        "QWidget { "
        "   background: %1; "
        "   font-family: '%2'; "
        "} "
        "QLabel { "
        "   color: #000000; "
        "   font-weight: 500; "
        "}"
    ).arg(MAIN_BACKGROUND, DEFAULT_FONT);

    mainPage->setStyleSheet(mainStyle);

    // 重构主页面布局为现代化风格
    setupModernMainPageLayout(mainPage);

    qDebug() << "主页面现代化风格设置完成";
}

void UIManager::setupModernMainPageLayout(QWidget *mainPage)
{
    qDebug() << "开始创建现代化仪表盘界面...";

    // 获取现有的布局和控件
    QVBoxLayout *mainLayout = mainPage->findChild<QVBoxLayout*>("mainLayout");
    QLabel *timeLabel = mainPage->findChild<QLabel*>("timeLabel");
    QPushButton *newActionButton = mainPage->findChild<QPushButton*>("newActionButton"); // 新添加的功能按键
    // QLabel *weatherLabel = mainPage->findChild<QLabel*>("weatherLabel"); // 已移除天气信息到独立页面
    QPushButton *btn1 = mainPage->findChild<QPushButton*>("btn1");
    QPushButton *btn2 = mainPage->findChild<QPushButton*>("btn2");
    QPushButton *btn3 = mainPage->findChild<QPushButton*>("btn3");
    QPushButton *btn4 = mainPage->findChild<QPushButton*>("btn4");
    QPushButton *btn5 = mainPage->findChild<QPushButton*>("btn5");
    QPushButton *btn6 = mainPage->findChild<QPushButton*>("btn6");

    if (!mainLayout) return;

    // 清空现有布局
    QLayoutItem *item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
        }
        delete item;
    }

    // 设置主页面背景 - 现代农业风格（优化渐变）
    mainPage->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #f1f8e9, stop:0.3 #e8f5e8, stop:0.7 #dcedc8, stop:1 #c8e6c8); "
        "   color: #2E7D32; "
        "}"
    );

    // 设置布局 - 适配小屏幕
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(15);

    // 创建现代化组件
    createModernTopBar(mainPage, mainLayout, timeLabel, newActionButton);

    // 创建控制功能区域 - 居中显示
    QWidget *controlArea = new QWidget(mainPage);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlArea);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(15);

    createSixButtonGrid(controlArea, controlLayout, btn1, btn2, btn3, btn4, btn5, btn6);

    // 添加到主布局 - 居中显示
    mainLayout->addWidget(controlArea);

    qDebug() << "现代化仪表盘界面创建完成";
}

// 创建现代化顶部状态栏
QWidget* UIManager::createModernTopBar(QWidget *parent, QVBoxLayout *mainLayout, QLabel *timeLabel, QPushButton *newActionButton)
{
    // 创建顶部状态栏容器
    QWidget *topBar = new QWidget(parent);
    topBar->setFixedHeight(100);
    topBar->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "       stop:0 #4CAF50, stop:1 #2E7D32); "
        "   border-radius: 12px; "
        "}"
    );

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(30, 20, 30, 20);
    topLayout->setSpacing(20);

    // 左侧：系统标题和状态
    QWidget *titleSection = new QWidget(topBar);
    QVBoxLayout *titleLayout = new QVBoxLayout(titleSection);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(5);

    QLabel *systemTitle = new QLabel("🌾 灵枢智慧农业边缘智控系统", titleSection);
    systemTitle->setStyleSheet(
        "font-size: 22px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 1px 1px 3px rgba(0,0,0,0.3);"
    );

    QLabel *systemStatus = new QLabel("🏆 嵌入式芯片与系统设计大赛-瑞芯微赛题", titleSection);
    systemStatus->setStyleSheet(
        "font-size: 14px; "
        "color: #90EE90; "
        "background: transparent; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.3);"
    );

    titleLayout->addWidget(systemTitle);
    titleLayout->addWidget(systemStatus);

    // 中间：独立的功能按键区域
    QWidget *buttonSection = new QWidget(topBar);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonSection);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setAlignment(Qt::AlignCenter);

    // 添加AI智能决策模式按键 - 独立区域，更明显的颜色
    if (newActionButton) {
        newActionButton->setParent(buttonSection);
        newActionButton->setText("🤖 AI智能决策");
        newActionButton->setFixedSize(150, 45); // 调整宽度以适应新文本

        // 设置AI按钮样式 - 默认关闭状态（灰色）
        updateAIButtonStyle(newActionButton, false);

        // 连接AI智能决策按钮事件
        connect(newActionButton, &QPushButton::clicked, [this, newActionButton]() {
            if (m_aiDecisionManager) {
                if (m_aiDecisionManager->isEnabled()) {
                    // 当前开启，点击关闭
                    m_aiDecisionManager->disableAIDecision();
                    updateAIButtonStyle(newActionButton, false);
                    qDebug() << "用户关闭AI智能决策";
                } else {
                    // 当前关闭，点击开启
                    m_aiDecisionManager->enableAIDecision();
                    updateAIButtonStyle(newActionButton, true);
                    qDebug() << "用户开启AI智能决策";
                }
            }
        });

        buttonLayout->addWidget(newActionButton);
    }

    // 右侧：时间显示
    QWidget *timeSection = new QWidget(topBar);
    QHBoxLayout *timeLayout = new QHBoxLayout(timeSection);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(15);
    timeLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    if (timeLabel) {
        timeLabel->setParent(timeSection);
        timeLabel->setStyleSheet(
            "font-size: 18px; "
            "font-weight: 600; "
            "color: white; "
            "background: transparent; "
            "text-shadow: 1px 1px 2px rgba(0,0,0,0.3);"
        );
        timeLabel->setAlignment(Qt::AlignRight);
        timeLayout->addWidget(timeLabel);
    }

    // 添加到顶部布局 - 三部分布局
    topLayout->addWidget(titleSection, 3);  // 标题部分
    topLayout->addWidget(buttonSection, 1); // 功能按键部分
    topLayout->addWidget(timeSection, 2);   // 时间部分

    // 添加到主布局
    mainLayout->addWidget(topBar);

    return topBar;
}

// 创建天气仪表盘
QWidget* UIManager::createWeatherDashboard(QWidget *parent, QVBoxLayout *parentLayout, QLabel *weatherLabel)
{
    // 创建天气仪表盘容器 - 农业主题美化，适配小屏幕
    QWidget *weatherDashboard = new QWidget(parent);
    weatherDashboard->setFixedHeight(280);
    weatherDashboard->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "       stop:0 #A8E6CF, stop:0.3 #7FCDCD, stop:0.7 #81C784, stop:1 #66BB6A); "
        "   border-radius: 18px; "
        "}"
    );

    // 改为垂直布局：上方横向显示主要信息，下方显示详细信息
    QVBoxLayout *dashboardLayout = new QVBoxLayout(weatherDashboard);
    dashboardLayout->setContentsMargins(15, 15, 15, 15); // 适配小屏幕
    dashboardLayout->setSpacing(15); // 适配小屏幕

    // 上方：横向显示温度、地点、天气预报
    QWidget *topSection = new QWidget(weatherDashboard);
    QHBoxLayout *topLayout = new QHBoxLayout(topSection);
    topLayout->setContentsMargins(0, 3, 0, 3); // 适配小屏幕
    topLayout->setSpacing(15); // 适配小屏幕

    // 温度显示区域
    QWidget *tempArea = new QWidget(topSection);
    QHBoxLayout *tempAreaLayout = new QHBoxLayout(tempArea);
    tempAreaLayout->setContentsMargins(0, 0, 0, 0); // 保持原始边距
    tempAreaLayout->setSpacing(10); // 适配小屏幕

    QLabel *weatherIcon = new QLabel("🌱", tempArea); // 农业主题图标
    weatherIcon->setStyleSheet(
        "font-size: 60px; "
        "color: #FFFFFF; "
        "background: transparent; "
        "text-shadow: 3px 3px 6px rgba(0,0,0,0.6);"
    );

    QLabel *tempDisplay = new QLabel("22°C", tempArea);
    tempDisplay->setObjectName("tempDisplay");
    tempDisplay->setStyleSheet(
        "font-size: 32px; " // 进一步减小温度字体确保完全显示
        "font-weight: 700; "
        "color: white; "
        "background: transparent;"
    );

    tempAreaLayout->addWidget(weatherIcon);
    tempAreaLayout->addWidget(tempDisplay);

    // 地点信息区域
    QWidget *locationArea = new QWidget(topSection);
    QVBoxLayout *locationLayout = new QVBoxLayout(locationArea);
    locationLayout->setContentsMargins(0, 0, 0, 0); // 恢复原始边距
    locationLayout->setSpacing(5); // 恢复原始间距
    locationLayout->setAlignment(Qt::AlignCenter);

    QLabel *cityLabel = new QLabel("沈阳", locationArea);
    cityLabel->setStyleSheet(
        "font-size: 24px; "
        "font-weight: 600; "
        "color: rgba(255,255,255,0.9); "
        "background: transparent;"
    );
    cityLabel->setAlignment(Qt::AlignCenter);

    QLabel *regionLabel = new QLabel("辽宁省", locationArea);
    regionLabel->setStyleSheet(
        "font-size: 16px; "
        "color: rgba(255,255,255,0.7); "
        "background: transparent;"
    );
    regionLabel->setAlignment(Qt::AlignCenter);

    locationLayout->addWidget(cityLabel);
    locationLayout->addWidget(regionLabel);

    // 天气预报区域
    QWidget *forecastArea = new QWidget(topSection);
    QVBoxLayout *forecastLayout = new QVBoxLayout(forecastArea);
    forecastLayout->setContentsMargins(0, 0, 0, 0); // 恢复原始边距
    forecastLayout->setSpacing(5); // 恢复原始间距
    forecastLayout->setAlignment(Qt::AlignCenter);

    QLabel *weatherDesc = new QLabel("多云转晴", forecastArea);
    weatherDesc->setObjectName("weatherDesc");
    weatherDesc->setStyleSheet(
        "font-size: 20px; "
        "font-weight: 600; "
        "color: rgba(255,255,255,0.9); "
        "background: transparent;"
    );
    weatherDesc->setAlignment(Qt::AlignCenter);

    QLabel *tempRange = new QLabel("15°C / 25°C", forecastArea);
    tempRange->setStyleSheet(
        "font-size: 16px; "
        "color: rgba(255,255,255,0.7); "
        "background: transparent;"
    );
    tempRange->setAlignment(Qt::AlignCenter);

    forecastLayout->addWidget(weatherDesc);
    forecastLayout->addWidget(tempRange);

    // 添加到顶部布局 - 恢复原始比例
    topLayout->addWidget(tempArea, 2); // 恢复原始比例
    topLayout->addWidget(locationArea, 1);
    topLayout->addWidget(forecastArea, 2);

    // 添加顶部区域到主布局
    dashboardLayout->addWidget(topSection);

    // 下方：详细信息横向布局
    QWidget *detailsSection = new QWidget(weatherDashboard);
    QHBoxLayout *detailsLayout = new QHBoxLayout(detailsSection);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(10); // 恢复原始间距

    // 创建详细信息项
    auto createDetailItem = [](QWidget *parent, const QString &icon, const QString &label, const QString &value, const QString &objectName = "") -> QWidget* {
        QWidget *item = new QWidget(parent);
        QVBoxLayout *layout = new QVBoxLayout(item);
        layout->setContentsMargins(8, 8, 8, 8); // 恢复原始内边距
        layout->setSpacing(5); // 恢复原始间距

        item->setStyleSheet(
            "QWidget { "
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "       stop:0 rgba(255,255,255,0.25), stop:1 rgba(255,255,255,0.15)); "
            "   border-radius: 12px; "
            "}"
        );

        QLabel *iconLabel = new QLabel(icon, item);
        iconLabel->setStyleSheet(
            "font-size: 24px; " // 恢复原始图标大小
            "color: #FFFFFF; "
            "background: transparent; "
            "text-shadow: 2px 2px 4px rgba(0,0,0,0.5);"
        );
        iconLabel->setAlignment(Qt::AlignCenter);

        QLabel *valueLabel = new QLabel(value, item);
        valueLabel->setStyleSheet(
            "font-size: 14px; " // 恢复原始数值字体大小
            "font-weight: 600; "
            "color: white; "
            "background: transparent;"
        );
        valueLabel->setAlignment(Qt::AlignCenter);

        // 对于预警信息，允许自动换行显示完整内容
        if (objectName == "warningValue") {
            valueLabel->setWordWrap(true); // 允许自动换行
            valueLabel->setStyleSheet(
                "font-size: 12px; " // 稍微减小字体以适应更多内容
                "font-weight: 600; "
                "color: white; "
                "background: transparent;"
            );
        }

        // 设置objectName以便后续更新
        if (!objectName.isEmpty()) {
            valueLabel->setObjectName(objectName);
        }

        QLabel *labelLabel = new QLabel(label, item);
        labelLabel->setStyleSheet(
            "font-size: 11px; " // 恢复原始标签字体大小
            "color: rgba(255,255,255,0.7); "
            "background: transparent;"
        );
        labelLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(iconLabel);
        layout->addWidget(valueLabel);
        layout->addWidget(labelLabel);

        return item;
    };

    // 添加详细信息项 - 农业主题图标，横向排列
    detailsLayout->addWidget(createDetailItem(detailsSection, "💧", "湿度", "--", "humidityValue"));
    detailsLayout->addWidget(createDetailItem(detailsSection, "🌾", "风速", "--", "windSpeedValue"));
    detailsLayout->addWidget(createDetailItem(detailsSection, "🌡️", "气压", "--", "pressureValue"));
    detailsLayout->addWidget(createDetailItem(detailsSection, "🌞", "体感温度", "--", "feelsLikeValue"));
    detailsLayout->addWidget(createDetailItem(detailsSection, "🌧️", "降水量", "--", "precipValue"));
    detailsLayout->addWidget(createDetailItem(detailsSection, "⚠️", "灾害预警", "暂无预警", "warningValue"));

    // 添加详细信息区域到主布局
    dashboardLayout->addWidget(detailsSection);

    // 隐藏原始天气标签，我们使用新的显示方式
    if (weatherLabel) {
        weatherLabel->hide();
    }

    // 添加到父布局
    parentLayout->addWidget(weatherDashboard);

    return weatherDashboard;
}

// 创建六个按钮的控制网格 (3x2网格布局)
QWidget* UIManager::createSixButtonGrid(QWidget *parent, QVBoxLayout *parentLayout, QPushButton *btn1, QPushButton *btn2, QPushButton *btn3, QPushButton *btn4, QPushButton *btn5, QPushButton *btn6)
{
    // 创建控制网格容器
    QWidget *controlGrid = new QWidget(parent);
    controlGrid->setStyleSheet(
        "QWidget { "
        "   background: transparent; "
        "}"
    );

    QGridLayout *gridLayout = new QGridLayout(controlGrid);
    gridLayout->setContentsMargins(20, 20, 20, 20);
    gridLayout->setSpacing(20);

    // 创建六个现代化控制按钮 - 农业主题，统一样式
    QPushButton *lightButton = createModernButton(controlGrid, "智能补光系统", "🌻", "#FF8F00");
    QPushButton *curtainButton = createModernButton(controlGrid, "智能遮光系统", "🌿", "#00796B");
    QPushButton *yoloButton = createModernButton(controlGrid, "作物监测系统", "🔍", "#1565C0");
    QPushButton *weatherButton = createModernButton(controlGrid, "天气信息", "🌤️", "#4CAF50");
    QPushButton *greenhouseButton = createModernButton(controlGrid, "环境数据监测", "🏠", "#E91E63");
    QPushButton *irrigationButton = createModernButton(controlGrid, "智能灌溉系统", "💧", "#2196F3");

    // 适应1024×573分辨率的按钮尺寸 - 2行3列布局优化
    lightButton->setFixedSize(240, 100);
    curtainButton->setFixedSize(240, 100);
    yoloButton->setFixedSize(240, 100);
    weatherButton->setFixedSize(240, 100);
    greenhouseButton->setFixedSize(240, 100);
    irrigationButton->setFixedSize(240, 100);

    // 连接原始按钮的信号到新按钮
    if (btn1) {
        QObject::connect(lightButton, &QPushButton::clicked, btn1, &QPushButton::click);
        btn1->hide();
    }
    if (btn2) {
        QObject::connect(curtainButton, &QPushButton::clicked, btn2, &QPushButton::click);
        btn2->hide();
    }
    if (btn3) {
        QObject::connect(yoloButton, &QPushButton::clicked, btn3, &QPushButton::click);
        btn3->hide();
    }
    if (btn4) {
        QObject::connect(weatherButton, &QPushButton::clicked, btn4, &QPushButton::click);
        btn4->hide();
    }
    if (btn5) {
        QObject::connect(greenhouseButton, &QPushButton::clicked, btn5, &QPushButton::click);
        btn5->hide();
    }
    if (btn6) {
        QObject::connect(irrigationButton, &QPushButton::clicked, btn6, &QPushButton::click);
        btn6->hide();
    }

    // 添加到2x3网格布局 - 6个按钮排列
    gridLayout->addWidget(lightButton, 0, 0);      // 第一行第一列
    gridLayout->addWidget(curtainButton, 0, 1);    // 第一行第二列
    gridLayout->addWidget(yoloButton, 0, 2);       // 第一行第三列
    gridLayout->addWidget(weatherButton, 1, 0);    // 第二行第一列
    gridLayout->addWidget(greenhouseButton, 1, 1); // 第二行第二列
    gridLayout->addWidget(irrigationButton, 1, 2);   // 第二行第三列

    // 添加到父布局
    parentLayout->addWidget(controlGrid);

    return controlGrid;
}

// 创建控制网格
QWidget* UIManager::createControlGrid(QWidget *parent, QVBoxLayout *parentLayout, QPushButton *btn1, QPushButton *btn2, QPushButton *btn3)
{
    // 创建控制网格容器
    QWidget *controlGrid = new QWidget(parent);
    controlGrid->setStyleSheet(
        "QWidget { "
        "   background: transparent; "
        "}"
    );

    QVBoxLayout *gridLayout = new QVBoxLayout(controlGrid);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(15);

    // 创建现代化控制按钮 - 农业主题
    QPushButton *lightButton = createModernButton(controlGrid, "智能补光系统", "🌻", "#FF8F00");
    QPushButton *curtainButton = createModernButton(controlGrid, "智能遮光系统", "🌿", "#00796B");
    QPushButton *yoloButton = createModernButton(controlGrid, "作物监测系统", "🔍", "#1565C0");

    // 调整按钮尺寸适应更小的右侧布局
    lightButton->setFixedSize(240, 100);
    curtainButton->setFixedSize(240, 100);
    yoloButton->setFixedSize(240, 100);

    // 连接原始按钮的信号到新按钮
    if (btn1) {
        QObject::connect(lightButton, &QPushButton::clicked, btn1, &QPushButton::click);
        btn1->hide();
    }
    if (btn2) {
        QObject::connect(curtainButton, &QPushButton::clicked, btn2, &QPushButton::click);
        btn2->hide();
    }
    if (btn3) {
        QObject::connect(yoloButton, &QPushButton::clicked, btn3, &QPushButton::click);
        btn3->hide();
    }

    // 添加到垂直布局
    gridLayout->addWidget(lightButton);
    gridLayout->addWidget(curtainButton);
    gridLayout->addWidget(yoloButton);
    gridLayout->addStretch(); // 添加弹性空间

    // 添加到父布局
    parentLayout->addWidget(controlGrid);

    return controlGrid;
}

// 创建现代化按钮
QPushButton* UIManager::createModernButton(QWidget *parent, const QString &text, const QString &icon, const QString &color)
{
    QPushButton *button = new QPushButton(parent);
    button->setFixedSize(240, 100); // 2行3列布局优化尺寸

    // 创建按钮内容 - 确保完美居中
    QWidget *content = new QWidget(button);
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(12, 12, 12, 12); // 增大边距确保居中
    layout->setSpacing(8); // 适当间距
    layout->setAlignment(Qt::AlignCenter); // 整体布局居中

    // 图标 - 完美居中显示
    QLabel *iconLabel = new QLabel(icon, content);
    iconLabel->setStyleSheet(
        "font-size: 36px; " // 增大图标尺寸
        "color: #FFFFFF; "
        "background: transparent; "
        "text-shadow: 3px 3px 6px rgba(0,0,0,0.7);"
    );
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // 水平扩展，垂直固定

    // 文字 - 完美居中显示
    QLabel *textLabel = new QLabel(text, content);
    textLabel->setStyleSheet(
        "font-size: 14px; " // 增大文字尺寸
        "font-weight: 600; "
        "color: white; "
        "background: transparent;"
    );
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // 水平扩展，垂直固定

    // 添加弹性空间确保垂直居中
    layout->addStretch(1); // 上方弹性空间
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch(1); // 下方弹性空间

    // 设置按钮样式
    button->setStyleSheet(QString(
        "QPushButton { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 %1, stop:1 %2); "
        "   border-radius: 15px; "
        "   border: none; "
        "} "
        "QPushButton:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 %2, stop:1 %1); "
        "   transform: scale(1.05); "
        "} "
        "QPushButton:pressed { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 %3, stop:1 %2); "
        "}"
    ).arg(color)
     .arg(adjustColor(color, -20))
     .arg(adjustColor(color, -40)));

    // 确保内容容器在按钮中居中
    content->setGeometry(0, 0, 240, 100);

    return button;
}

// 辅助函数：调整颜色亮度 - 农业主题色彩
QString UIManager::adjustColor(const QString &color, int adjustment)
{
    // 农业主题色彩调整
    if (color == "#FF8F00") { // 补光系统橙色
        if (adjustment < 0) return "#E65100";
        return "#FFA726";
    } else if (color == "#00796B") { // 保温系统蓝绿色
        if (adjustment < 0) return "#004D40";
        return "#26A69A";
    } else if (color == "#1565C0") { // 监测系统深蓝色
        if (adjustment < 0) return "#0D47A1";
        return "#42A5F5";
    }
    return color;
}

void UIManager::initializeSubPages(QStackedWidget *stackedWidget)
{
    if (!stackedWidget) {
        qWarning() << "StackedWidget指针为空";
        return;
    }

    // 初始化各个子页面
    if (stackedWidget->count() > 1) {
        QWidget *page1 = stackedWidget->widget(1);
        if (page1) {
            QVBoxLayout *layout = new QVBoxLayout(page1);
            createLightControlPage(page1, layout);
        }
    }

    if (stackedWidget->count() > 2) {
        QWidget *page2 = stackedWidget->widget(2);
        if (page2) {
            QVBoxLayout *layout = new QVBoxLayout(page2);
            createCurtainControlPage(page2, layout);
        }
    }

    if (stackedWidget->count() > 3) {
        QWidget *page3 = stackedWidget->widget(3);
        if (page3) {
            QVBoxLayout *layout = new QVBoxLayout(page3);
            createYOLOv8Page(page3, layout);
        }
    }

    if (stackedWidget->count() > 4) {
        QWidget *page4 = stackedWidget->widget(4);
        if (page4) {
            QVBoxLayout *layout = new QVBoxLayout(page4);
            createWeatherInfoPage(page4, layout);
        }
    }

    if (stackedWidget->count() > 5) {
        QWidget *page5 = stackedWidget->widget(5);
        if (page5) {
            QVBoxLayout *layout = new QVBoxLayout(page5);
            createGreenhouseInfoPage(page5, layout);
        }
    }

    if (stackedWidget->count() > 6) {
        QWidget *page6 = stackedWidget->widget(6);
        if (page6) {
            QVBoxLayout *layout = new QVBoxLayout(page6);
            createIrrigationControlPage(page6, layout);
        }
    }

    qDebug() << "子页面初始化完成";
}

void UIManager::switchToPage(QStackedWidget *stackedWidget, int index)
{
    if (!stackedWidget) {
        qWarning() << "StackedWidget指针为空";
        return;
    }

    if (index >= 0 && index < stackedWidget->count()) {
        // 设置子页面尺寸与父容器一致
        QWidget *page = stackedWidget->widget(index);
        if (page) {
            page->setFixedSize(stackedWidget->size());
            stackedWidget->setCurrentIndex(index);
            emit pageChanged(index);
            qDebug() << "切换到页面" << index;
        }
    } else {
        qWarning() << "页面索引超出范围:" << index;
    }
}

QString UIManager::getMainBackgroundStyle() const
{
    return MAIN_BACKGROUND;
}

QString UIManager::getButtonStyle(const QString &bgColor) const
{
    return BUTTON_BASE_STYLE + QString("background-color: %1;").arg(bgColor);
}

QString UIManager::getCardStyle(const QString &borderColor) const
{
    return QString(
        "QWidget { "
        "   background: #ffffff; "
        "   border-radius: 10px; "
        "   border: 2px solid %1; "
        "   padding: 20px; "
        "}"
    ).arg(borderColor);
}

void UIManager::createLightControlPage(QWidget *page, QVBoxLayout *contentLayout)
{
    // 设置页面样式 - 与主页面完全一致
    page->setStyleSheet(QString(
        "QWidget { background: %1; } "
        "QLabel { color: #000000; font-weight: 500; }"
    ).arg(MAIN_BACKGROUND));

    // 设置布局 - 与主页面一致
    contentLayout->setContentsMargins(20, 15, 20, 15);
    contentLayout->setSpacing(20);

    // 创建简洁的标题栏
    QWidget *titleBar = new QWidget(page);
    titleBar->setStyleSheet("QWidget { background: transparent; }");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *mainTitle = new QLabel("🌻 智能补光灯控制系统", titleBar);
    mainTitle->setStyleSheet(
        "font-size: 24px; "
        "font-weight: 700; "
        "color: #FF8F00; "
        "background: transparent;"
    );

    // 返回按钮 - 简洁样式
    QPushButton *returnBtn = new QPushButton("← 返回", titleBar);
    returnBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #FF8F00; "
        "   color: white; "
        "   border: none; "
        "   border-radius: 8px; "
        "   padding: 8px 16px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "   background-color: #E65100; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #BF360C; "
        "}"
    );

    // 连接返回按钮信号
    connect(returnBtn, &QPushButton::clicked, [this]() {
        emit pageChanged(0); // 返回主页面
    });

    titleLayout->addWidget(mainTitle);
    titleLayout->addStretch();
    titleLayout->addWidget(returnBtn);

    contentLayout->addWidget(titleBar);

    // 创建补光灯控制仪表盘
    QWidget *lightDashboard = new QWidget(page);
    lightDashboard->setFixedHeight(400);
    lightDashboard->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "       stop:0 #E65100, stop:0.3 #F57C00, stop:0.7 #FF8F00, stop:1 #FFB74D); "
        "   border-radius: 18px; "
        "}"
    );

    QVBoxLayout *dashboardLayout = new QVBoxLayout(lightDashboard);
    dashboardLayout->setContentsMargins(30, 30, 30, 30);
    dashboardLayout->setSpacing(25);

    // 标题区域
    QLabel *dashboardTitle = new QLabel("🌻 智能补光灯控制", lightDashboard);
    dashboardTitle->setStyleSheet(
        "font-size: 26px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.7);"
    );
    dashboardTitle->setAlignment(Qt::AlignCenter);
    dashboardLayout->addWidget(dashboardTitle);

    // 状态显示区域
    QWidget *statusArea = new QWidget(lightDashboard);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusArea);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(20);

    // 创建状态信息卡片的函数
    auto createStatusCard = [](QWidget *parent, const QString &icon, const QString &title, const QString &value, const QString &objectName = "") -> QWidget* {
        QWidget *card = new QWidget(parent);
        card->setFixedHeight(120); // 水平布局后减小高度
        card->setStyleSheet(
            "QWidget { "
            "   background: rgba(255,255,255,0.2); "
            "   border-radius: 15px; "
            "}"
        );
        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(10, 15, 10, 15);
        layout->setSpacing(8);
        layout->setAlignment(Qt::AlignCenter);

        // 图标居中显示
        QLabel *iconLabel = new QLabel(icon, card);
        iconLabel->setStyleSheet(
            "font-size: 36px; "
            "color: white; "
            "background: transparent; "
            "text-shadow: 2px 2px 4px rgba(0,0,0,0.7);"
        );
        iconLabel->setAlignment(Qt::AlignCenter);

        // 标题和数值水平排列
        QWidget *infoWidget = new QWidget(card);
        QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);
        infoLayout->setContentsMargins(0, 0, 0, 0);
        infoLayout->setSpacing(8);
        infoLayout->setAlignment(Qt::AlignCenter);

        QLabel *titleLabel = new QLabel(title, infoWidget);
        titleLabel->setStyleSheet(
            "font-size: 13px; "
            "color: white; "
            "background: transparent; "
            "font-weight: 600; "
            "text-shadow: 1px 1px 3px rgba(0,0,0,0.5);"
        );
        titleLabel->setAlignment(Qt::AlignCenter);

        QLabel *valueLabel = new QLabel(value, infoWidget);
        valueLabel->setStyleSheet(
            "font-size: 18px; "
            "color: white; "
            "background: transparent; "
            "font-weight: 700; "
            "text-shadow: 1px 1px 3px rgba(0,0,0,0.5);"
        );
        valueLabel->setAlignment(Qt::AlignCenter);

        infoLayout->addWidget(titleLabel);
        infoLayout->addWidget(valueLabel);

        // 设置objectName以便后续更新
        if (!objectName.isEmpty()) {
            valueLabel->setObjectName(objectName);
        }

        layout->addWidget(iconLabel);
        layout->addWidget(infoWidget);

        return card;
    };

    // 添加两个状态卡片
    statusLayout->addWidget(createStatusCard(statusArea, "💡", "补光灯强度", "60%", "lightStatusValue"));
    statusLayout->addWidget(createStatusCard(statusArea, "⚙️", "工作模式", "手动模式", "modeStatusValue"));

    dashboardLayout->addWidget(statusArea);

    // 控制区域
    QWidget *controlArea = new QWidget(lightDashboard);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlArea);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(15);

    QLabel *controlTitle = new QLabel("🌞 补光强度调节", controlArea);
    controlTitle->setStyleSheet(
        "font-size: 22px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.7);"
    );
    controlTitle->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(controlTitle);

    // 滑块控制区域
    QWidget *sliderContainer = new QWidget(controlArea);
    sliderContainer->setStyleSheet(
        "QWidget { "
        "   background: rgba(255,255,255,0.2); "
        "   border-radius: 15px; "
        "}"
    );
    QVBoxLayout *sliderLayout = new QVBoxLayout(sliderContainer);
    sliderLayout->setContentsMargins(30, 20, 30, 20);
    sliderLayout->setSpacing(10);

    QSlider *lightSlider = new QSlider(Qt::Horizontal, sliderContainer);
    lightSlider->setRange(0, 100);
    lightSlider->setValue(60);
    lightSlider->setObjectName("lightSlider");
    lightSlider->setStyleSheet(
        "QSlider::groove:horizontal { "
        "   height: 8px; "
        "   background: rgba(255,255,255,0.3); "
        "   border-radius: 4px; "
        "} "
        "QSlider::handle:horizontal { "
        "   width: 20px; "
        "   height: 20px; "
        "   margin: -6px 0; "
        "   background: white; "
        "   border-radius: 10px; "
        "   box-shadow: 0 2px 6px rgba(0,0,0,0.3); "
        "} "
        "QSlider::sub-page:horizontal { "
        "   background: rgba(255,255,255,0.8); "
        "   border-radius: 4px; "
        "}"
    );

    // 连接滑块信号，实时更新状态显示、PWM硬件和MQTT上报
    QLabel *lightStatusValue = lightDashboard->findChild<QLabel*>("lightStatusValue");
    connect(lightSlider, &QSlider::valueChanged, [this, lightStatusValue](int value) {
        // 更新UI显示
        if (lightStatusValue) {
            lightStatusValue->setText(QString("%1%").arg(value));
        }

        // 更新PWM硬件占空比
        if (m_pwmController) {
            m_pwmController->setDutyCycle(value);

            // 立即触发MQTT数据上报
            if (m_mqttService) {
                emit m_mqttService->dataCollectionRequested();
            }
        }
    });

    sliderLayout->addWidget(lightSlider);
    controlLayout->addWidget(sliderContainer);

    dashboardLayout->addWidget(controlArea);

    // 添加状态信息
    QLabel *statusLabel = new QLabel("实时控制补光灯强度，确保作物光照充足", lightDashboard);
    statusLabel->setStyleSheet(
        "font-size: 16px; "
        "color: white; "
        "background: transparent; "
        "text-align: center; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
    );
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    dashboardLayout->addWidget(statusLabel);

    contentLayout->addWidget(lightDashboard);
    contentLayout->addStretch();

    qDebug() << "补光灯控制页面创建完成";
}

void UIManager::createCurtainControlPage(QWidget *page, QVBoxLayout *contentLayout)
{
    // 设置页面样式 - 与主页面完全一致
    page->setStyleSheet(QString(
        "QWidget { background: %1; } "
        "QLabel { color: #000000; font-weight: 500; }"
    ).arg(MAIN_BACKGROUND));

    // 设置布局 - 与主页面一致
    contentLayout->setContentsMargins(20, 15, 20, 15);
    contentLayout->setSpacing(20);

    // 创建简洁的标题栏
    QWidget *titleBar = new QWidget(page);
    titleBar->setStyleSheet("QWidget { background: transparent; }");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *mainTitle = new QLabel("🌿 智能遮光系统", titleBar);
    mainTitle->setStyleSheet(
        "font-size: 24px; "
        "font-weight: 700; "
        "color: #00796B; "
        "background: transparent;"
    );

    // 返回按钮 - 简洁样式
    QPushButton *returnBtn = new QPushButton("← 返回", titleBar);
    returnBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #00796B; "
        "   color: white; "
        "   border: none; "
        "   border-radius: 8px; "
        "   padding: 8px 16px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "   background-color: #00695C; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #004D40; "
        "}"
    );

    // 连接返回按钮信号
    connect(returnBtn, &QPushButton::clicked, [this]() {
        emit pageChanged(0); // 返回主页面
    });

    titleLayout->addWidget(mainTitle);
    titleLayout->addStretch();
    titleLayout->addWidget(returnBtn);

    contentLayout->addWidget(titleBar);

    // 创建遮光系统控制仪表盘
    QWidget *curtainDashboard = new QWidget(page);
    curtainDashboard->setFixedHeight(400);
    curtainDashboard->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "       stop:0 #004D40, stop:0.3 #00695C, stop:0.7 #00796B, stop:1 #26A69A); "
        "   border-radius: 18px; "
        "}"
    );

    QVBoxLayout *dashboardLayout = new QVBoxLayout(curtainDashboard);
    dashboardLayout->setContentsMargins(30, 30, 30, 30);
    dashboardLayout->setSpacing(25);

    // 标题区域
    QLabel *dashboardTitle = new QLabel("🌿 智能遮光控制", curtainDashboard);
    dashboardTitle->setStyleSheet(
        "font-size: 26px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.7);"
    );
    dashboardTitle->setAlignment(Qt::AlignCenter);
    dashboardLayout->addWidget(dashboardTitle);

    // 控制区域
    QWidget *controlsArea = new QWidget(curtainDashboard);
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsArea);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(30);

    // 创建控制卡片的函数 - 添加GPIO控制逻辑
    auto createControlCard = [this](QWidget *parent, const QString &icon, const QString &title, const QString &status, const QString &objectName = "", bool isTopCurtain = true) -> QWidget* {
        QWidget *card = new QWidget(parent);
        card->setStyleSheet(
            "QWidget { "
            "   background: rgba(255,255,255,0.15); "
            "   border-radius: 15px; "
            "   border: 1px solid rgba(255,255,255,0.3); "
            "}"
        );

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 20, 20, 20);
        cardLayout->setSpacing(15);

        // 图标
        QLabel *iconLabel = new QLabel(icon, card);
        iconLabel->setStyleSheet(
            "font-size: 56px; "
            "color: #FFFFFF; "
            "background: transparent; "
            "text-shadow: 3px 3px 6px rgba(0,0,0,0.8);"
        );
        iconLabel->setAlignment(Qt::AlignCenter);

        // 标题
        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet(
            "font-size: 20px; "
            "font-weight: 600; "
            "color: white; "
            "background: transparent; "
            "text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
        );
        titleLabel->setAlignment(Qt::AlignCenter);

        // 状态
        QLabel *statusLabel = new QLabel(status, card);
        statusLabel->setStyleSheet(
            "font-size: 18px; "
            "font-weight: 700; "
            "color: #FFF59D; "
            "background: transparent; "
            "text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
        );
        statusLabel->setAlignment(Qt::AlignCenter);

        // 设置objectName以便后续更新
        if (!objectName.isEmpty()) {
            statusLabel->setObjectName(objectName);
        }

        // 控制按钮
        QWidget *buttonArea = new QWidget(card);
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonArea);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(10);

        QPushButton *openBtn = new QPushButton("🔓 打开", buttonArea);
        QPushButton *closeBtn = new QPushButton("🔒 关闭", buttonArea);

        QString buttonStyle =
            "QPushButton { "
            "   background: rgba(255,255,255,0.25); "
            "   color: white; "
            "   border: 1px solid rgba(255,255,255,0.4); "
            "   border-radius: 10px; "
            "   padding: 10px 16px; "
            "   font-size: 16px; "
            "   font-weight: 600; "
            "   text-shadow: 1px 1px 2px rgba(0,0,0,0.5); "
            "} "
            "QPushButton:hover { "
            "   background: rgba(255,255,255,0.35); "
            "   border: 1px solid rgba(255,255,255,0.6); "
            "} "
            "QPushButton:pressed { "
            "   background: rgba(255,255,255,0.15); "
            "}";

        // 定义按下时的按钮样式
        QString pressedButtonStyle =
            "QPushButton { "
            "   background: rgba(255,255,255,0.45); "
            "   color: white; "
            "   border: 1px solid rgba(255,255,255,0.8); "
            "   border-radius: 10px; "
            "   padding: 10px 16px; "
            "   font-size: 16px; "
            "   font-weight: 600; "
            "   text-shadow: 1px 1px 2px rgba(0,0,0,0.5); "
            "}";

        openBtn->setStyleSheet(buttonStyle);
        closeBtn->setStyleSheet(buttonStyle);

        // 连接按钮信号 - 实现GPIO控制逻辑
        if (isTopCurtain) {
            // 上侧帘控制逻辑
            connect(openBtn, &QPushButton::pressed, [this, statusLabel, openBtn, pressedButtonStyle]() {
                statusLabel->setText("打开中");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #4CAF50; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                openBtn->setStyleSheet(pressedButtonStyle);

                // 调用GPIO控制方法
                if (m_curtainController) {
                    m_curtainController->openTopCurtain();
                }
            });

            connect(openBtn, &QPushButton::released, [this, statusLabel, openBtn, buttonStyle]() {
                statusLabel->setText("暂停");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #FFF59D; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                openBtn->setStyleSheet(buttonStyle);

                // 调用GPIO暂停方法
                if (m_curtainController) {
                    m_curtainController->pauseTopCurtain();
                }
            });

            connect(closeBtn, &QPushButton::pressed, [this, statusLabel, closeBtn, pressedButtonStyle]() {
                statusLabel->setText("关闭中");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #F44336; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                closeBtn->setStyleSheet(pressedButtonStyle);

                // 调用GPIO控制方法
                if (m_curtainController) {
                    m_curtainController->closeTopCurtain();
                }
            });

            connect(closeBtn, &QPushButton::released, [this, statusLabel, closeBtn, buttonStyle]() {
                statusLabel->setText("暂停");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #FFF59D; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                closeBtn->setStyleSheet(buttonStyle);

                // 调用GPIO暂停方法
                if (m_curtainController) {
                    m_curtainController->pauseTopCurtain();
                }
            });
        } else {
            // 侧面帘控制逻辑
            connect(openBtn, &QPushButton::pressed, [this, statusLabel, openBtn, pressedButtonStyle]() {
                statusLabel->setText("打开中");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #4CAF50; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                openBtn->setStyleSheet(pressedButtonStyle);

                // 调用GPIO控制方法
                if (m_curtainController) {
                    m_curtainController->openSideCurtain();
                }
            });

            connect(openBtn, &QPushButton::released, [this, statusLabel, openBtn, buttonStyle]() {
                statusLabel->setText("暂停");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #FFF59D; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                openBtn->setStyleSheet(buttonStyle);

                // 调用GPIO暂停方法
                if (m_curtainController) {
                    m_curtainController->pauseSideCurtain();
                }
            });

            connect(closeBtn, &QPushButton::pressed, [this, statusLabel, closeBtn, pressedButtonStyle]() {
                statusLabel->setText("关闭中");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #F44336; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                closeBtn->setStyleSheet(pressedButtonStyle);

                // 调用GPIO控制方法
                if (m_curtainController) {
                    m_curtainController->closeSideCurtain();
                }
            });

            connect(closeBtn, &QPushButton::released, [this, statusLabel, closeBtn, buttonStyle]() {
                statusLabel->setText("暂停");
                statusLabel->setStyleSheet(
                    "font-size: 18px; font-weight: 700; color: #FFF59D; "
                    "background: transparent; text-shadow: 1px 1px 3px rgba(0,0,0,0.7);"
                );
                closeBtn->setStyleSheet(buttonStyle);

                // 调用GPIO暂停方法
                if (m_curtainController) {
                    m_curtainController->pauseSideCurtain();
                }
            });
        }

        buttonLayout->addWidget(openBtn);
        buttonLayout->addWidget(closeBtn);

        cardLayout->addWidget(iconLabel);
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(statusLabel);
        cardLayout->addWidget(buttonArea);

        return card;
    };

    // 添加两个控制卡片 - 区分上帘和侧帘
    controlsLayout->addWidget(createControlCard(controlsArea, "🏠", "上侧遮光帘", "暂停", "topCurtainStatus", true));  // true表示上帘
    controlsLayout->addWidget(createControlCard(controlsArea, "🌿", "侧面遮光帘", "暂停", "sideCurtainStatus", false)); // false表示侧帘

    dashboardLayout->addWidget(controlsArea);

    // 添加状态信息
    QLabel *statusLabel = new QLabel("实时控制遮光帘开关，确保大棚光照适宜", curtainDashboard);
    statusLabel->setStyleSheet(
        "font-size: 16px; "
        "color: rgba(255,255,255,0.9); "
        "background: transparent; "
        "text-align: center; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
    );
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    dashboardLayout->addWidget(statusLabel);

    contentLayout->addWidget(curtainDashboard);
    contentLayout->addStretch();

    qDebug() << "智能遮光系统页面创建完成";
}



void UIManager::createYOLOv8Page(QWidget *page, QVBoxLayout *contentLayout)
{
    // 设置页面样式
    page->setStyleSheet(QString(
        "QWidget { background: %1; } "
        "QLabel { color: #000000; font-weight: 500; }"
    ).arg(MAIN_BACKGROUND));

    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(20);

    // 创建页面头部
    QWidget *header = createPageHeader(page, "YOLOv8实时监测");
    contentLayout->addWidget(header);

    // 创建内容容器
    QWidget *contentWidget = createContentContainer(page);
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);

    // 页面3现在为空，YOLOv8功能直接从主页面按钮启动
    QLabel *emptyLabel = new QLabel("此页面已移除，YOLOv8功能请从主页面直接启动", page);
    emptyLabel->setStyleSheet(
        "font-size: 16px; "
        "color: #666666; "
        "text-align: center; "
        "padding: 50px;"
    );
    layout->addWidget(emptyLabel);
    layout->addStretch();

    contentLayout->addWidget(contentWidget);

    qDebug() << "YOLOv8页面创建完成";
}

void UIManager::createWeatherInfoPage(QWidget *page, QVBoxLayout *contentLayout)
{
    // 设置页面样式 - 与主页面完全一致
    page->setStyleSheet(QString(
        "QWidget { background: %1; } "
        "QLabel { color: #000000; font-weight: 500; }"
    ).arg(MAIN_BACKGROUND));

    // 设置布局 - 适配小屏幕
    contentLayout->setContentsMargins(15, 10, 15, 10);
    contentLayout->setSpacing(15);

    // 创建简洁的标题栏
    QWidget *titleBar = new QWidget(page);
    titleBar->setStyleSheet("QWidget { background: transparent; }");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *mainTitle = new QLabel("🌤️ 天气信息", titleBar);
    mainTitle->setStyleSheet(
        "font-size: 20px; "
        "font-weight: 700; "
        "color: #4CAF50; "
        "background: transparent;"
    );

    // 返回按钮 - 简洁样式
    QPushButton *returnBtn = new QPushButton("← 返回", titleBar);
    returnBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #4CAF50; "
        "   color: white; "
        "   border: none; "
        "   border-radius: 8px; "
        "   padding: 8px 16px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "   background-color: #45a049; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #3d8b40; "
        "}"
    );

    // 连接返回按钮信号
    connect(returnBtn, &QPushButton::clicked, [this]() {
        emit pageChanged(0); // 返回主页面
    });

    titleLayout->addWidget(mainTitle);
    titleLayout->addStretch();
    titleLayout->addWidget(returnBtn);

    contentLayout->addWidget(titleBar);

    // 创建美化的天气信息仪表盘 - 适配小屏幕
    QWidget *weatherDashboard = new QWidget(page);
    weatherDashboard->setFixedHeight(400);
    weatherDashboard->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "       stop:0 #1B5E20, stop:0.2 #2E7D32, stop:0.5 #388E3C, stop:0.8 #43A047, stop:1 #4CAF50); "
        "   border-radius: 15px; "
        "}"
    );

    QVBoxLayout *dashboardLayout = new QVBoxLayout(weatherDashboard);
    dashboardLayout->setContentsMargins(20, 15, 20, 15);
    dashboardLayout->setSpacing(15);

    // 主要天气信息区域
    QWidget *mainWeatherArea = new QWidget(weatherDashboard);
    QHBoxLayout *mainWeatherLayout = new QHBoxLayout(mainWeatherArea);
    mainWeatherLayout->setContentsMargins(0, 0, 0, 0);
    mainWeatherLayout->setSpacing(30);

    // 左侧：温度和天气图标
    QWidget *tempSection = new QWidget(mainWeatherArea);
    QVBoxLayout *tempLayout = new QVBoxLayout(tempSection);
    tempLayout->setContentsMargins(0, 0, 0, 0);
    tempLayout->setSpacing(10);
    tempLayout->setAlignment(Qt::AlignCenter);

    QLabel *weatherIcon = new QLabel("🌤️", tempSection);
    weatherIcon->setStyleSheet(
        "font-size: 70px; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 3px 3px 8px rgba(0,0,0,0.6);"
    );
    weatherIcon->setAlignment(Qt::AlignCenter);

    QLabel *currentTemp = new QLabel("22°C", tempSection);
    currentTemp->setObjectName("tempDisplay");
    currentTemp->setStyleSheet(
        "font-size: 42px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 6px rgba(0,0,0,0.6);"
    );
    currentTemp->setAlignment(Qt::AlignCenter);

    tempLayout->addWidget(weatherIcon);
    tempLayout->addWidget(currentTemp);

    // 右侧：地点和天气描述
    QWidget *infoSection = new QWidget(mainWeatherArea);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoSection);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(15);
    infoLayout->setAlignment(Qt::AlignCenter);

    QLabel *cityName = new QLabel("沈阳市", infoSection);
    cityName->setStyleSheet(
        "font-size: 26px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.6);"
    );
    cityName->setAlignment(Qt::AlignCenter);

    QLabel *weatherDesc = new QLabel("多云转晴", infoSection);
    weatherDesc->setObjectName("weatherDesc");
    weatherDesc->setStyleSheet(
        "font-size: 18px; "
        "font-weight: 600; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 1px 1px 3px rgba(0,0,0,0.5);"
    );
    weatherDesc->setAlignment(Qt::AlignCenter);

    QLabel *tempRange = new QLabel("15°C ~ 25°C", infoSection);
    tempRange->setStyleSheet(
        "font-size: 16px; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 1px 1px 3px rgba(0,0,0,0.5);"
    );
    tempRange->setAlignment(Qt::AlignCenter);

    infoLayout->addWidget(cityName);
    infoLayout->addWidget(weatherDesc);
    infoLayout->addWidget(tempRange);

    mainWeatherLayout->addWidget(tempSection, 1);
    mainWeatherLayout->addWidget(infoSection, 1);
    dashboardLayout->addWidget(mainWeatherArea);

    // 详细天气信息区域
    QWidget *detailsArea = new QWidget(weatherDashboard);
    QGridLayout *detailsLayout = new QGridLayout(detailsArea);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(10);

    // 创建天气详情卡片的函数
    auto createWeatherDetailCard = [](QWidget *parent, const QString &icon, const QString &title, const QString &value, const QString &objectName = "") -> QWidget* {
        QWidget *card = new QWidget(parent);
        card->setStyleSheet(
            "QWidget { "
            "   background: rgba(255,255,255,0.25); "
            "   border-radius: 12px; "
            "}"
        );
        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(5);

        QLabel *iconLabel = new QLabel(icon, card);
        iconLabel->setStyleSheet(
            "font-size: 22px; "
            "color: white; "
            "background: transparent; "
            "text-shadow: 2px 2px 4px rgba(0,0,0,0.6);"
        );
        iconLabel->setAlignment(Qt::AlignCenter);

        QLabel *valueLabel = new QLabel(value, card);
        valueLabel->setStyleSheet(
            "font-size: 16px; "
            "font-weight: 700; "
            "color: white; "
            "background: transparent; "
            "text-shadow: 1px 1px 3px rgba(0,0,0,0.5);"
        );
        valueLabel->setAlignment(Qt::AlignCenter);

        // 对于预警信息，允许自动换行显示完整内容
        if (objectName == "warningValue") {
            valueLabel->setWordWrap(true);
            valueLabel->setStyleSheet(
                "font-size: 14px; "
                "font-weight: 600; "
                "color: white; "
                "background: transparent; "
                "text-shadow: 1px 1px 3px rgba(0,0,0,0.3);"
            );
        }

        // 设置objectName以便后续更新
        if (!objectName.isEmpty()) {
            valueLabel->setObjectName(objectName);
        }

        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet(
            "font-size: 14px; "
            "color: white; "
            "background: transparent; "
            "text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
        );
        titleLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(iconLabel);
        layout->addWidget(valueLabel);
        layout->addWidget(titleLabel);

        return card;
    };

    // 添加天气详情卡片 - 3x2网格布局
    detailsLayout->addWidget(createWeatherDetailCard(detailsArea, "💧", "湿度", "--", "humidityValue"), 0, 0);
    detailsLayout->addWidget(createWeatherDetailCard(detailsArea, "🌾", "风速", "--", "windSpeedValue"), 0, 1);
    detailsLayout->addWidget(createWeatherDetailCard(detailsArea, "🌡️", "气压", "--", "pressureValue"), 0, 2);
    detailsLayout->addWidget(createWeatherDetailCard(detailsArea, "🌞", "体感温度", "--", "feelsLikeValue"), 1, 0);
    detailsLayout->addWidget(createWeatherDetailCard(detailsArea, "🌧️", "降水量", "--", "precipValue"), 1, 1);
    detailsLayout->addWidget(createWeatherDetailCard(detailsArea, "⚠️", "灾害预警", "暂无预警", "warningValue"), 1, 2);

    dashboardLayout->addWidget(detailsArea);

    // 添加底部状态信息和更新时间 - 同行显示
    QWidget *bottomInfoArea = new QWidget(weatherDashboard);
    QVBoxLayout *bottomInfoLayout = new QVBoxLayout(bottomInfoArea);
    bottomInfoLayout->setContentsMargins(0, 0, 0, 0);
    bottomInfoLayout->setSpacing(5);

    QLabel *statusInfo = new QLabel("实时监测天气变化，为温室管理提供科学依据", bottomInfoArea);
    statusInfo->setStyleSheet(
        "font-size: 14px; "
        "color: white; "
        "background: transparent; "
        "text-align: center; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
    );
    statusInfo->setAlignment(Qt::AlignCenter);
    statusInfo->setWordWrap(true);

    QLabel *updateTimeLabel = new QLabel("最后更新时间：--", bottomInfoArea);
    updateTimeLabel->setObjectName("updateTimeLabel");
    updateTimeLabel->setStyleSheet(
        "font-size: 12px; "
        "color: white; "
        "background: transparent; "
        "text-align: center; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
    );
    updateTimeLabel->setAlignment(Qt::AlignCenter);

    bottomInfoLayout->addWidget(statusInfo);
    bottomInfoLayout->addWidget(updateTimeLabel);
    dashboardLayout->addWidget(bottomInfoArea);

    contentLayout->addWidget(weatherDashboard);
    contentLayout->addStretch();

    qDebug() << "天气信息页面创建完成";
}

void UIManager::createGreenhouseInfoPage(QWidget *page, QVBoxLayout *contentLayout)
{
    // 设置页面样式 - 与主页面完全一致
    page->setStyleSheet(QString(
        "QWidget { background: %1; } "
        "QLabel { color: #000000; font-weight: 500; }"
    ).arg(MAIN_BACKGROUND));

    // 设置布局 - 与主页面一致
    contentLayout->setContentsMargins(20, 15, 20, 15);
    contentLayout->setSpacing(20);

    // 创建简洁的标题栏
    QWidget *titleBar = new QWidget(page);
    titleBar->setStyleSheet("QWidget { background: transparent; }");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *mainTitle = new QLabel("🏠 环境数据监测", titleBar);
    mainTitle->setStyleSheet(
        "font-size: 24px; "
        "font-weight: 700; "
        "color: #E91E63; "
        "background: transparent;"
    );

    // 返回按钮 - 简洁样式
    QPushButton *returnBtn = new QPushButton("← 返回", titleBar);
    returnBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #E91E63; "
        "   color: white; "
        "   border: none; "
        "   border-radius: 8px; "
        "   padding: 8px 16px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "   background-color: #C2185B; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #AD1457; "
        "}"
    );

    // 连接返回按钮信号
    connect(returnBtn, &QPushButton::clicked, [this]() {
        emit pageChanged(0); // 返回主页面
    });

    titleLayout->addWidget(mainTitle);
    titleLayout->addStretch();
    titleLayout->addWidget(returnBtn);

    contentLayout->addWidget(titleBar);

    // 创建大棚信息仪表盘
    QWidget *greenhouseDashboard = new QWidget(page);
    greenhouseDashboard->setFixedHeight(400); // 适当减小高度，确保在屏幕范围内
    greenhouseDashboard->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "       stop:0 #FCE4EC, stop:0.3 #F8BBD9, stop:0.7 #F48FB1, stop:1 #E91E63); "
        "   border-radius: 18px; "
        "}"
    );

    QVBoxLayout *dashboardLayout = new QVBoxLayout(greenhouseDashboard);
    dashboardLayout->setContentsMargins(20, 15, 20, 15);
    dashboardLayout->setSpacing(15);

    // 标题区域
    QLabel *dashboardTitle = new QLabel("🌡️ 大棚环境监测", greenhouseDashboard);
    dashboardTitle->setStyleSheet(
        "font-size: 20px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.5);"
    );
    dashboardTitle->setAlignment(Qt::AlignCenter);
    dashboardLayout->addWidget(dashboardTitle);

    // 传感器数据显示区域 - 改为2x2网格布局
    QWidget *sensorsArea = new QWidget(greenhouseDashboard);
    QGridLayout *sensorsLayout = new QGridLayout(sensorsArea);
    sensorsLayout->setContentsMargins(0, 0, 0, 5); // 适当减小底部边距
    sensorsLayout->setSpacing(15); // 适当减小卡片间距

    // 创建传感器信息卡片的函数
    auto createSensorCard = [](QWidget *parent, const QString &icon, const QString &title, const QString &value, const QString &unit, const QString &objectName = "") -> QWidget* {
        QWidget *card = new QWidget(parent);
        card->setFixedHeight(160); // 适当减小高度，平衡显示效果和空间利用
        card->setStyleSheet(
            "QWidget { "
            "   background: rgba(255,255,255,0.2); "
            "   border-radius: 15px; "
            "}"
        );

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(15, 15, 15, 20); // 适当减小下方边距
        cardLayout->setSpacing(8);

        // 图标
        QLabel *iconLabel = new QLabel(icon, card);
        iconLabel->setStyleSheet(
            "font-size: 40px; "
            "color: #FFFFFF; "
            "background: transparent; "
            "text-shadow: 3px 3px 6px rgba(0,0,0,0.6);"
        );
        iconLabel->setAlignment(Qt::AlignCenter);

        // 标题
        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet(
            "font-size: 14px; "
            "font-weight: 600; "
            "color: rgba(255,255,255,0.9); "
            "background: transparent;"
        );
        titleLabel->setAlignment(Qt::AlignCenter);

        // 数值
        QLabel *valueLabel = new QLabel(value + unit, card);
        valueLabel->setStyleSheet(
            "font-size: 20px; "
            "font-weight: 700; "
            "color: white; "
            "background: transparent;"
        );
        valueLabel->setAlignment(Qt::AlignCenter);

        // 设置objectName以便后续更新
        if (!objectName.isEmpty()) {
            valueLabel->setObjectName(objectName);
        }

        cardLayout->addWidget(iconLabel);
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabel);

        return card;
    };

    // 添加四个传感器卡片 - 2x2网格布局
    sensorsLayout->addWidget(createSensorCard(sensorsArea, "🌡️", "温度", "--", "°C", "tempHumLabel"), 0, 0);
    sensorsLayout->addWidget(createSensorCard(sensorsArea, "💧", "湿度", "--", "%", "humidityLabel"), 0, 1);
    sensorsLayout->addWidget(createSensorCard(sensorsArea, "☀️", "光照强度", "--", " lx", "luxLabel"), 1, 0);
    sensorsLayout->addWidget(createSensorCard(sensorsArea, "🌱", "土壤湿度", "--", "%", "soilMoistureLabel"), 1, 1);

    dashboardLayout->addWidget(sensorsArea);

    // 添加状态信息
    QLabel *statusLabel = new QLabel("实时监测大棚内环境参数，确保作物生长环境最佳", greenhouseDashboard);
    statusLabel->setStyleSheet(
        "font-size: 12px; "
        "color: rgba(255,255,255,0.8); "
        "background: transparent; "
        "text-align: center;"
    );
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    dashboardLayout->addWidget(statusLabel);

    contentLayout->addWidget(greenhouseDashboard);
    contentLayout->addStretch();

    qDebug() << "大棚实时信息页面创建完成";
}

// 创建天气信息卡片 - 在主界面2x2布局中显示
QWidget* UIManager::createWeatherCard(QWidget *parent)
{
    QWidget *weatherCard = new QWidget(parent);
    weatherCard->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "       stop:0 #A8E6CF, stop:0.3 #7FCDCD, stop:0.7 #81C784, stop:1 #66BB6A); "
        "   border-radius: 15px; "
        "   border: 2px solid rgba(255,255,255,0.3); "
        "}"
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(weatherCard);
    cardLayout->setContentsMargins(15, 10, 15, 10);
    cardLayout->setSpacing(8);

    // 顶部：天气图标和温度
    QWidget *topSection = new QWidget(weatherCard);
    QHBoxLayout *topLayout = new QHBoxLayout(topSection);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    // 天气图标
    QLabel *weatherIcon = new QLabel("🌱", topSection);
    weatherIcon->setStyleSheet(
        "font-size: 32px; "
        "color: #FFFFFF; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.5);"
    );

    // 温度显示
    QLabel *tempDisplay = new QLabel("22°C", topSection);
    tempDisplay->setObjectName("tempDisplay");
    tempDisplay->setStyleSheet(
        "font-size: 24px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent;"
    );

    topLayout->addWidget(weatherIcon);
    topLayout->addWidget(tempDisplay);
    topLayout->addStretch();

    // 中部：地点信息
    QWidget *middleSection = new QWidget(weatherCard);
    QVBoxLayout *middleLayout = new QVBoxLayout(middleSection);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(2);

    QLabel *cityLabel = new QLabel("沈阳", middleSection);
    cityLabel->setStyleSheet(
        "font-size: 16px; "
        "font-weight: 600; "
        "color: rgba(255,255,255,0.9); "
        "background: transparent;"
    );

    QLabel *weatherDesc = new QLabel("多云转晴", middleSection);
    weatherDesc->setObjectName("weatherDesc");
    weatherDesc->setStyleSheet(
        "font-size: 12px; "
        "color: rgba(255,255,255,0.7); "
        "background: transparent;"
    );

    middleLayout->addWidget(cityLabel);
    middleLayout->addWidget(weatherDesc);

    // 底部：关键信息横向显示
    QWidget *bottomSection = new QWidget(weatherCard);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomSection);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(8);

    // 创建小信息项
    auto createMiniInfo = [](QWidget *parent, const QString &icon, const QString &value, const QString &objectName = "") -> QWidget* {
        QWidget *item = new QWidget(parent);
        QVBoxLayout *layout = new QVBoxLayout(item);
        layout->setContentsMargins(3, 3, 3, 3);
        layout->setSpacing(2);

        item->setStyleSheet(
            "QWidget { "
            "   background: rgba(255,255,255,0.2); "
            "   border-radius: 6px; "
            "}"
        );

        QLabel *iconLabel = new QLabel(icon, item);
        iconLabel->setStyleSheet(
            "font-size: 12px; "
            "color: #FFFFFF; "
            "background: transparent;"
        );
        iconLabel->setAlignment(Qt::AlignCenter);

        QLabel *valueLabel = new QLabel(value, item);
        valueLabel->setStyleSheet(
            "font-size: 10px; "
            "font-weight: 600; "
            "color: white; "
            "background: transparent;"
        );
        valueLabel->setAlignment(Qt::AlignCenter);

        // 设置objectName以便后续更新
        if (!objectName.isEmpty()) {
            valueLabel->setObjectName(objectName);
        }

        layout->addWidget(iconLabel);
        layout->addWidget(valueLabel);

        return item;
    };

    // 添加关键天气信息
    bottomLayout->addWidget(createMiniInfo(bottomSection, "💧", "--", "humidityValue"));
    bottomLayout->addWidget(createMiniInfo(bottomSection, "🌾", "--", "windSpeedValue"));
    bottomLayout->addWidget(createMiniInfo(bottomSection, "🌡️", "--", "feelsLikeValue"));

    // 组装卡片
    cardLayout->addWidget(topSection);
    cardLayout->addWidget(middleSection);
    cardLayout->addWidget(bottomSection);

    return weatherCard;
}



QWidget* UIManager::createPageHeader(QWidget *parent, const QString &title)
{
    QWidget *headerWidget = new QWidget(parent);
    headerWidget->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "   stop:0 #ffffff, stop:1 #f8f9fa); "
        "   border-radius: 12px; "
        "   border: 2px solid rgba(52, 73, 94, 0.2); "
        "}"
    );

    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 15, 20, 15);

    // 页面标题
    QLabel *titleLabel = new QLabel(title, headerWidget);
    titleLabel->setStyleSheet(
        "QLabel { "
        "   font-size: 24px; "
        "   font-weight: 600; "
        "   color: #000000; "
        "   background: transparent; "
        "}"
    );

    // 检查是否需要隐藏窗口控制按钮
    bool hideWindowControls = (title == "补光灯控制" || title == "保温帘控制");

    qDebug() << "页面标题:" << title << "隐藏窗口控制:" << hideWindowControls;

    // 窗口控制按钮区域
    QHBoxLayout *windowControlLayout = new QHBoxLayout();

    QPushButton *minimizeBtn = nullptr;
    QPushButton *maximizeBtn = nullptr;

    // 只在需要时创建窗口控制按钮
    if (!hideWindowControls) {
        // 最小化按钮
        minimizeBtn = new QPushButton("🗕", headerWidget);
        minimizeBtn->setStyleSheet(
            "QPushButton { "
            "   background-color: #f8f9fa; "
            "   color: #000000; "
            "   border: 2px solid #dee2e6; "
            "   border-radius: 6px; "
            "   padding: 8px 12px; "
            "   font-size: 12px; "
            "   font-weight: 600; "
            "   min-width: 30px; "
            "   max-width: 30px; "
            "} "
            "QPushButton:hover { "
            "   background-color: #e9ecef; "
            "   border: 2px solid #adb5bd; "
            "} "
            "QPushButton:pressed { "
            "   background-color: #dee2e6; "
            "}"
        );
        minimizeBtn->setFixedSize(35, 30);
        minimizeBtn->setToolTip("最小化");

        // 最大化按钮
        maximizeBtn = new QPushButton("🗖", headerWidget);
        maximizeBtn->setStyleSheet(minimizeBtn->styleSheet());
        maximizeBtn->setFixedSize(35, 30);
        maximizeBtn->setToolTip("最大化/还原");

        // 连接窗口控制按钮信号
        connect(minimizeBtn, &QPushButton::clicked, [parent]() {
            QWidget *topLevel = parent;
            while (topLevel->parentWidget()) {
                topLevel = topLevel->parentWidget();
            }
            if (topLevel) {
                topLevel->showMinimized();
            }
        });

        connect(maximizeBtn, &QPushButton::clicked, [parent]() {
            QWidget *topLevel = parent;
            while (topLevel->parentWidget()) {
                topLevel = topLevel->parentWidget();
            }
            if (topLevel) {
                if (topLevel->isMaximized()) {
                    topLevel->showNormal();
                } else {
                    topLevel->showMaximized();
                }
            }
        });
    }

    // 返回按钮
    QPushButton *backBtn = new QPushButton("← 返回", headerWidget);
    backBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #f8f9fa; "
        "   color: #000000; "
        "   border: 2px solid #dee2e6; "
        "   border-radius: 8px; "
        "   padding: 10px 20px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "   background-color: #e9ecef; "
        "   border: 2px solid #adb5bd; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #dee2e6; "
        "}"
    );
    backBtn->setFixedSize(90, 40);

    // 连接返回按钮信号
    connect(backBtn, &QPushButton::clicked, [this]() {
        emit pageChanged(0); // 返回主页面
    });

    // 根据页面类型决定是否添加窗口控制按钮
    if (!hideWindowControls) {
        windowControlLayout->addWidget(minimizeBtn);
        windowControlLayout->addWidget(maximizeBtn);
        windowControlLayout->addSpacing(10);
    }
    windowControlLayout->addWidget(backBtn);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addLayout(windowControlLayout);

    return headerWidget;
}

QWidget* UIManager::createContentContainer(QWidget *parent)
{
    QWidget *contentWidget = new QWidget(parent);
    contentWidget->setStyleSheet(
        "QWidget { "
        "   background-color: #ffffff; "
        "   border: 2px solid #333333; "
        "   border-radius: 8px; "
        "}"
    );
    return contentWidget;
}

// AI智能决策按钮样式更新
void UIManager::updateAIButtonStyle(QPushButton *button, bool enabled)
{
    if (!button) return;

    if (enabled) {
        // 开启状态 - 绿色渐变
        button->setStyleSheet(
            "QPushButton { "
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "       stop:0 #4CAF50, stop:1 #388E3C); "
            "   border: 2px solid #66BB6A; "
            "   border-radius: 10px; "
            "   color: white; "
            "   font-size: 13px; "
            "   font-weight: 600; "
            "   text-shadow: 1px 1px 3px rgba(0,0,0,0.5); "
            "} "
            "QPushButton:hover { "
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "       stop:0 #66BB6A, stop:1 #4CAF50); "
            "   border: 2px solid #81C784; "
            "   transform: scale(1.05); "
            "} "
            "QPushButton:pressed { "
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "       stop:0 #388E3C, stop:1 #2E7D32); "
            "   border: 2px solid #4CAF50; "
            "}"
        );
    } else {
        // 关闭状态 - 灰色渐变
        button->setStyleSheet(
            "QPushButton { "
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "       stop:0 #9E9E9E, stop:1 #757575); "
            "   border: 2px solid #BDBDBD; "
            "   border-radius: 10px; "
            "   color: white; "
            "   font-size: 13px; "
            "   font-weight: 600; "
            "   text-shadow: 1px 1px 3px rgba(0,0,0,0.5); "
            "} "
            "QPushButton:hover { "
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "       stop:0 #BDBDBD, stop:1 #9E9E9E); "
            "   border: 2px solid #E0E0E0; "
            "   transform: scale(1.05); "
            "} "
            "QPushButton:pressed { "
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "       stop:0 #757575, stop:1 #616161); "
            "   border: 2px solid #9E9E9E; "
            "}"
        );
    }
}

// 锁定/解锁手动遮光帘控制
void UIManager::lockManualCurtainControls(bool locked)
{
    qDebug() << QString("手动遮光帘控制%1").arg(locked ? "已锁定" : "已解锁");

    // 查找遮光帘控制页面的所有按钮并设置禁用状态
    QList<QPushButton*> curtainButtons = QApplication::activeWindow()->findChildren<QPushButton*>();

    for (QPushButton* button : curtainButtons) {
        // 根据按钮的objectName或文本来识别遮光帘控制按钮
        if (button->objectName().contains("curtain", Qt::CaseInsensitive) ||
            button->text().contains("打开") ||
            button->text().contains("关闭")) {
            button->setEnabled(!locked);

            // 更新按钮样式以显示禁用状态
            if (locked) {
                button->setStyleSheet(button->styleSheet() + "QPushButton { opacity: 0.5; }");
            } else {
                button->setStyleSheet(button->styleSheet().replace("QPushButton { opacity: 0.5; }", ""));
            }
        }
    }
}

void UIManager::createIrrigationControlPage(QWidget *page, QVBoxLayout *contentLayout)
{
    // 设置页面样式 - 与主页面完全一致
    page->setStyleSheet(QString(
        "QWidget { background: %1; } "
        "QLabel { color: #000000; font-weight: 500; }"
    ).arg(MAIN_BACKGROUND));

    // 设置布局 - 优化间距以适应更多内容
    contentLayout->setContentsMargins(20, 10, 20, 10);
    contentLayout->setSpacing(15);

    // 创建简洁的标题栏
    QWidget *titleBar = new QWidget(page);
    titleBar->setStyleSheet("QWidget { background: transparent; }");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *mainTitle = new QLabel("💧 智能灌溉系统", titleBar);
    mainTitle->setStyleSheet(
        "font-size: 24px; "
        "font-weight: 700; "
        "color: #2196F3; "
        "background: transparent;"
    );

    // 返回按钮 - 蓝色主题
    QPushButton *returnBtn = new QPushButton("← 返回", titleBar);
    returnBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #2196F3; "
        "   color: white; "
        "   border: none; "
        "   border-radius: 8px; "
        "   padding: 8px 16px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "   background-color: #1976D2; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #0D47A1; "
        "}"
    );

    // 连接返回按钮信号
    connect(returnBtn, &QPushButton::clicked, [this]() {
        emit pageChanged(0); // 返回主页面
    });

    titleLayout->addWidget(mainTitle);
    titleLayout->addStretch();
    titleLayout->addWidget(returnBtn);

    contentLayout->addWidget(titleBar);

    // 创建灌溉系统仪表盘
    QWidget *irrigationDashboard = new QWidget(page);
    irrigationDashboard->setFixedHeight(400);
    irrigationDashboard->setStyleSheet(
        "QWidget { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "       stop:0 #0D47A1, stop:0.3 #1565C0, stop:0.7 #1976D2, stop:1 #2196F3); "
        "   border-radius: 18px; "
        "}"
    );

    QVBoxLayout *dashboardLayout = new QVBoxLayout(irrigationDashboard);
    dashboardLayout->setContentsMargins(25, 20, 25, 20);
    dashboardLayout->setSpacing(18);

    // 标题区域 - 缩小字体和间距
    QLabel *dashboardTitle = new QLabel("💧 智能灌溉控制", irrigationDashboard);
    dashboardTitle->setStyleSheet(
        "font-size: 22px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.7);"
    );
    dashboardTitle->setAlignment(Qt::AlignCenter);
    dashboardLayout->addWidget(dashboardTitle);

    // 水泵状态显示区域 - 缩小间距
    QWidget *statusArea = new QWidget(irrigationDashboard);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusArea);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(15);

    // 创建状态卡片函数 - 缩小高度
    auto createStatusCard = [](QWidget *parent, const QString &icon, const QString &title, const QString &value, const QString &objectName = "") -> QWidget* {
        QWidget *card = new QWidget(parent);
        card->setFixedHeight(90);
        card->setStyleSheet(
            "QWidget { "
            "   background: rgba(255,255,255,0.25); "
            "   border-radius: 15px; "
            "}"
        );

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);
        cardLayout->setSpacing(5);

        QLabel *iconLabel = new QLabel(icon, card);
        iconLabel->setStyleSheet(
            "font-size: 24px; "
            "color: #FFFFFF; "
            "background: transparent; "
            "text-shadow: 2px 2px 4px rgba(0,0,0,0.6);"
        );
        iconLabel->setAlignment(Qt::AlignCenter);

        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet(
            "font-size: 12px; "
            "font-weight: 600; "
            "color: rgba(255,255,255,0.9); "
            "background: transparent;"
        );
        titleLabel->setAlignment(Qt::AlignCenter);

        QLabel *valueLabel = new QLabel(value, card);
        valueLabel->setStyleSheet(
            "font-size: 14px; "
            "font-weight: 700; "
            "color: white; "
            "background: transparent;"
        );
        valueLabel->setAlignment(Qt::AlignCenter);

        if (!objectName.isEmpty()) {
            valueLabel->setObjectName(objectName);
        }

        cardLayout->addWidget(iconLabel);
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabel);

        return card;
    };

    // 添加三个状态卡片
    statusLayout->addWidget(createStatusCard(statusArea, "💧", "水泵状态", "关闭", "pumpStatusValue"));
    statusLayout->addWidget(createStatusCard(statusArea, "🧪", "施药泵", "关闭", "fertilizerPumpStatusValue"));
    statusLayout->addWidget(createStatusCard(statusArea, "⚙️", "工作模式", "手动模式", "modeStatusValue"));

    dashboardLayout->addWidget(statusArea);

    // 控制区域 - 缩小间距
    QWidget *controlArea = new QWidget(irrigationDashboard);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlArea);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(12);

    QLabel *controlTitle = new QLabel("🚰 泵控制系统", controlArea);
    controlTitle->setStyleSheet(
        "font-size: 18px; "
        "font-weight: 700; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 2px 2px 4px rgba(0,0,0,0.7);"
    );
    controlTitle->setAlignment(Qt::AlignCenter);

    // 按钮控制区域 - 使用垂直布局容纳两行按钮，缩小间距
    QWidget *buttonArea = new QWidget(controlArea);
    QVBoxLayout *buttonMainLayout = new QVBoxLayout(buttonArea);
    buttonMainLayout->setContentsMargins(0, 0, 0, 0);
    buttonMainLayout->setSpacing(10);

    // 水泵控制行 - 缩小间距
    QWidget *pumpButtonRow = new QWidget(buttonArea);
    QHBoxLayout *pumpButtonLayout = new QHBoxLayout(pumpButtonRow);
    pumpButtonLayout->setContentsMargins(0, 0, 0, 0);
    pumpButtonLayout->setSpacing(15);

    // 水泵标签
    QLabel *pumpLabel = new QLabel("💧 水泵:", pumpButtonRow);
    pumpLabel->setStyleSheet(
        "font-size: 16px; "
        "font-weight: 600; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.7);"
    );
    pumpLabel->setFixedWidth(80);

    // 开启水泵按钮
    QPushButton *startPumpBtn = new QPushButton("开启", pumpButtonRow);
    startPumpBtn->setFixedSize(100, 40);
    startPumpBtn->setStyleSheet(
        "QPushButton { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #4CAF50, stop:1 #2E7D32); "
        "   color: white; "
        "   border: none; "
        "   border-radius: 12px; "
        "   font-size: 16px; "
        "   font-weight: 600; "
        "   text-shadow: 1px 1px 2px rgba(0,0,0,0.5); "
        "} "
        "QPushButton:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #66BB6A, stop:1 #388E3C); "
        "} "
        "QPushButton:pressed { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #2E7D32, stop:1 #1B5E20); "
        "}"
    );

    // 关闭水泵按钮
    QPushButton *stopPumpBtn = new QPushButton("关闭", pumpButtonRow);
    stopPumpBtn->setFixedSize(100, 40);
    stopPumpBtn->setStyleSheet(
        "QPushButton { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #F44336, stop:1 #C62828); "
        "   color: white; "
        "   border: none; "
        "   border-radius: 12px; "
        "   font-size: 16px; "
        "   font-weight: 600; "
        "   text-shadow: 1px 1px 2px rgba(0,0,0,0.5); "
        "} "
        "QPushButton:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #EF5350, stop:1 #D32F2F); "
        "} "
        "QPushButton:pressed { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #C62828, stop:1 #B71C1C); "
        "}"
    );

    // 连接按钮信号
    connect(startPumpBtn, &QPushButton::clicked, [this, irrigationDashboard]() {
        QLabel *pumpStatusValue = irrigationDashboard->findChild<QLabel*>("pumpStatusValue");

        // 更新UI状态
        if (pumpStatusValue) {
            pumpStatusValue->setText("运行中");
            pumpStatusValue->setStyleSheet(
                "font-size: 18px; "
                "font-weight: 700; "
                "color: #FFD700; "
                "background: transparent; "
                "text-shadow: 1px 1px 2px rgba(0,0,0,0.8);"
            );
        }

        // GPIO控制
        if (m_gpioController && m_gpioController->startPump()) {
            qDebug() << "水泵已开启 - GPIO3_A7置1";
        } else {
            qWarning() << "水泵开启失败";
        }
    });

    connect(stopPumpBtn, &QPushButton::clicked, [this, irrigationDashboard]() {
        QLabel *pumpStatusValue = irrigationDashboard->findChild<QLabel*>("pumpStatusValue");

        // 更新UI状态
        if (pumpStatusValue) {
            pumpStatusValue->setText("关闭");
            pumpStatusValue->setStyleSheet(
                "font-size: 18px; "
                "font-weight: 700; "
                "color: white; "
                "background: transparent;"
            );
        }

        // GPIO控制
        if (m_gpioController && m_gpioController->stopPump()) {
            qDebug() << "水泵已关闭 - GPIO3_A7置0";
        } else {
            qWarning() << "水泵关闭失败";
        }
    });

    // 添加水泵控制按钮到水泵行
    pumpButtonLayout->addWidget(pumpLabel);
    pumpButtonLayout->addWidget(startPumpBtn);
    pumpButtonLayout->addWidget(stopPumpBtn);
    pumpButtonLayout->addStretch();

    // 施药泵控制行 - 缩小间距
    QWidget *fertilizerButtonRow = new QWidget(buttonArea);
    QHBoxLayout *fertilizerButtonLayout = new QHBoxLayout(fertilizerButtonRow);
    fertilizerButtonLayout->setContentsMargins(0, 0, 0, 0);
    fertilizerButtonLayout->setSpacing(15);

    // 施药泵标签
    QLabel *fertilizerLabel = new QLabel("🧪 施药泵:", fertilizerButtonRow);
    fertilizerLabel->setStyleSheet(
        "font-size: 16px; "
        "font-weight: 600; "
        "color: white; "
        "background: transparent; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.7);"
    );
    fertilizerLabel->setFixedWidth(80);

    // 开启施药泵按钮
    QPushButton *startFertilizerBtn = new QPushButton("开启", fertilizerButtonRow);
    startFertilizerBtn->setFixedSize(100, 40);
    startFertilizerBtn->setStyleSheet(
        "QPushButton { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #FF9800, stop:1 #E65100); "
        "   color: white; "
        "   border: none; "
        "   border-radius: 10px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "   text-shadow: 1px 1px 2px rgba(0,0,0,0.5); "
        "} "
        "QPushButton:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #FFB74D, stop:1 #F57C00); "
        "} "
        "QPushButton:pressed { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #E65100, stop:1 #BF360C); "
        "}"
    );

    // 关闭施药泵按钮
    QPushButton *stopFertilizerBtn = new QPushButton("关闭", fertilizerButtonRow);
    stopFertilizerBtn->setFixedSize(100, 40);
    stopFertilizerBtn->setStyleSheet(
        "QPushButton { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #9E9E9E, stop:1 #616161); "
        "   color: white; "
        "   border: none; "
        "   border-radius: 10px; "
        "   font-size: 14px; "
        "   font-weight: 600; "
        "   text-shadow: 1px 1px 2px rgba(0,0,0,0.5); "
        "} "
        "QPushButton:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #BDBDBD, stop:1 #757575); "
        "} "
        "QPushButton:pressed { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #616161, stop:1 #424242); "
        "}"
    );

    // 连接施药泵按钮信号
    connect(startFertilizerBtn, &QPushButton::clicked, [this, irrigationDashboard]() {
        QLabel *fertilizerStatusValue = irrigationDashboard->findChild<QLabel*>("fertilizerPumpStatusValue");

        // 更新UI状态
        if (fertilizerStatusValue) {
            fertilizerStatusValue->setText("运行中");
            fertilizerStatusValue->setStyleSheet(
                "font-size: 18px; "
                "font-weight: 700; "
                "color: #FFD700; "
                "background: transparent; "
                "text-shadow: 1px 1px 2px rgba(0,0,0,0.8);"
            );
        }

        // GPIO控制
        if (m_gpioController && m_gpioController->startFertilizerPump()) {
            qDebug() << "施药泵已开启 - GPIO3_A1置1";
        } else {
            qWarning() << "施药泵开启失败";
        }
    });

    connect(stopFertilizerBtn, &QPushButton::clicked, [this, irrigationDashboard]() {
        QLabel *fertilizerStatusValue = irrigationDashboard->findChild<QLabel*>("fertilizerPumpStatusValue");

        // 更新UI状态
        if (fertilizerStatusValue) {
            fertilizerStatusValue->setText("关闭");
            fertilizerStatusValue->setStyleSheet(
                "font-size: 18px; "
                "font-weight: 700; "
                "color: white; "
                "background: transparent;"
            );
        }

        // GPIO控制
        if (m_gpioController && m_gpioController->stopFertilizerPump()) {
            qDebug() << "施药泵已关闭 - GPIO3_A1置0";
        } else {
            qWarning() << "施药泵关闭失败";
        }
    });

    // 添加施药泵控制按钮到施药泵行
    fertilizerButtonLayout->addWidget(fertilizerLabel);
    fertilizerButtonLayout->addWidget(startFertilizerBtn);
    fertilizerButtonLayout->addWidget(stopFertilizerBtn);
    fertilizerButtonLayout->addStretch();

    // 将两行添加到主按钮布局
    buttonMainLayout->addWidget(pumpButtonRow);
    buttonMainLayout->addWidget(fertilizerButtonRow);

    controlLayout->addWidget(controlTitle);
    controlLayout->addWidget(buttonArea);

    dashboardLayout->addWidget(controlArea);

    // 添加状态信息 - 缩小字体
    QLabel *statusLabel = new QLabel("智能控制灌溉系统，确保作物水分充足", irrigationDashboard);
    statusLabel->setStyleSheet(
        "font-size: 14px; "
        "color: white; "
        "background: transparent; "
        "text-align: center; "
        "text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
    );
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    dashboardLayout->addWidget(statusLabel);

    contentLayout->addWidget(irrigationDashboard);
    contentLayout->addStretch();

    qDebug() << "智能灌溉系统页面创建完成";
}
