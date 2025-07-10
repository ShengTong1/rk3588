#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
QT_END_NAMESPACE

/**
 * @brief 天气数据服务
 *
 * 负责获取和解析天气数据，提供温室环境参考信息
 * 支持自动更新和错误重试机制
 */
class WeatherService : public QObject
{
    Q_OBJECT

public:
    // 增强的天气数据结构
    struct WeatherData {
        QString cityName;        // 城市名称
        QString temperature;     // 温度
        QString humidity;        // 湿度
        QString description;     // 天气描述
        QString windDirection;   // 风向
        QString windSpeed;       // 风速
        QString pressure;        // 气压
        QString visibility;      // 能见度
        QString uvIndex;         // 紫外线指数
        QString airQuality;      // 空气质量
        QString feelLike;        // 体感温度
        QString updateTime;      // 更新时间
        QString solarRadiation;  // 太阳辐射
        QString precipitation;   // 降水量
        bool isValid;           // 数据是否有效

        WeatherData() : isValid(false) {}
    };

    // 天气预警数据结构
    struct WeatherWarning {
        QString title;           // 预警标题
        QString level;           // 预警级别
        QString type;            // 预警类型
        QString description;     // 预警描述
        QString startTime;       // 开始时间
        QString endTime;         // 结束时间
        bool isValid;           // 数据是否有效

        WeatherWarning() : isValid(false) {}
    };



    // 降水预报数据结构
    struct PrecipitationForecast {
        QString summary;         // 降水概况
        QString probability;     // 降水概率
        QString intensity;       // 降水强度
        QString type;            // 降水类型
        bool isValid;

        PrecipitationForecast() : isValid(false) {}
    };

    explicit WeatherService(QObject *parent = nullptr);
    ~WeatherService();

    // 主要功能接口
    void fetchWeatherData();             // 获取天气数据
    void setCity(const QString &city);   // 设置城市
    void setApiKey(const QString &apiKey); // 设置API密钥
    void setAutoUpdate(bool enabled, int intervalMinutes = 30); // 设置自动更新

    // 状态查询
    WeatherData getCurrentWeather() const { return m_currentWeather; }
    WeatherWarning getCurrentWarning() const { return m_currentWarning; }
    PrecipitationForecast getCurrentPrecipitation() const { return m_currentPrecipitation; }
    bool isUpdating() const { return m_isUpdating; }

    // 格式化显示函数
    QString formatWeatherDisplay(const WeatherData &data) const; // 格式化显示文本
    QString formatPrecipitationDisplay(const PrecipitationForecast &forecast) const; // 格式化降水预报显示

signals:
    void weatherDataUpdated(const WeatherData &data); // 天气数据更新信号
    void warningUpdated(const WeatherWarning &warning); // 天气预警更新信号
    void precipitationUpdated(const PrecipitationForecast &forecast); // 降水预报更新信号
    void updateFailed(const QString &error);          // 更新失败信号

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply); // 网络请求完成处理
    void onAutoUpdateTimer();                          // 自动更新定时器

private:
    // 网络组件
    QNetworkAccessManager *m_networkManager;
    QTimer *m_autoUpdateTimer;

    // 配置参数
    QString m_city;                      // 城市名称
    QString m_apiKey;                    // API密钥
    bool m_autoUpdateEnabled;            // 自动更新开关
    int m_updateInterval;                // 更新间隔(分钟)

    // 状态变量
    WeatherData m_currentWeather;        // 当前天气数据
    WeatherWarning m_currentWarning;     // 当前天气预警
    PrecipitationForecast m_currentPrecipitation; // 当前降水预报
    bool m_isUpdating;                   // 是否正在更新

    // 内部功能函数
    QString buildApiUrl() const;         // 构建API URL
    QString buildPrecipitationUrl() const; // 构建降水预报API URL

    // 数据解析函数
    void parseWeatherResponse(const QJsonObject &root);      // 解析天气响应
    void parseWarningResponse(const QJsonObject &root);      // 解析天气预警响应
    void parsePrecipitationResponse(const QJsonObject &root); // 解析降水预报响应
};

#endif // WEATHER_SERVICE_H
