QT       += core gui widgets network

CONFIG += c++11

# 项目信息
TARGET = wonderfulnewworld
TEMPLATE = app
VERSION = 1.0.0

# 包含路径
INCLUDEPATH += include

# 源文件 - 按功能模块组织
SOURCES += \
    main.cpp \
    src/core/mainwindow.cpp \
    src/ui/ui_manager.cpp \
    src/hardware/pwm_controller.cpp \
    src/hardware/gpio_controller.cpp \
    src/hardware/gy30_sensor.cpp \
    src/hardware/gy30_light_sensor.cpp \
    src/device/curtain_controller.cpp \
    src/ai/ai_decision_manager.cpp \
    src/integration/yolov8_integration.cpp \
    src/network/weather_service.cpp \
    src/network/mqtt_service.cpp \
    src/system/window_manager.cpp

# 头文件 - 按功能模块组织
HEADERS += \
    include/core/mainwindow.h \
    include/ui/ui_manager.h \
    include/hardware/pwm_controller.h \
    include/hardware/gpio_controller.h \
    include/hardware/gy30_sensor.h \
    include/hardware/gy30_light_sensor.h \
    include/device/curtain_controller.h \
    include/ai/ai_decision_manager.h \
    include/integration/yolov8_integration.h \
    include/network/weather_service.h \
    include/network/mqtt_service.h \
    include/config/aliyun_config.h \
    include/config/gpio_config.h \
    include/config/ai_config.h \
    include/system/window_manager.h \


# UI文件
FORMS += \
    mainwindow.ui

# 部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
