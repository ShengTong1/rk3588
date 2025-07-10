#include "network/weather_service.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

WeatherService::WeatherService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_autoUpdateTimer(new QTimer(this))
    , m_city("沈阳")
    , m_apiKey("")
    , m_autoUpdateEnabled(false)
    , m_updateInterval(30)
    , m_isUpdating(false)
{
    // 连接网络管理器信号
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &WeatherService::onNetworkReplyFinished);

    // 设置自动更新定时器
    connect(m_autoUpdateTimer, &QTimer::timeout,
            this, &WeatherService::onAutoUpdateTimer);

    qDebug() << "天气服务创建完成";
}

WeatherService::~WeatherService()
{
    qDebug() << "天气服务已销毁";
}

void WeatherService::fetchWeatherData()
{
    if (m_isUpdating) {
        qDebug() << "天气数据正在更新中，跳过本次请求";
        return;
    }

    // 使用用户提供的和风天气API信息
    const QString apiHost = "pg6apvwmx9.re.qweatherapi.com";
    const QString apiKey = "bed023ef80af43f89e00481c5f02f2aa";
    const QString city = "101070101"; // 沈阳的城市代码

    qDebug() << QString("开始获取%1的和风天气数据...").arg(m_city);
    m_isUpdating = true;

    // 构建和风天气实时天气API URL
    QUrl weatherUrl(QString("https://%1/v7/weather/now?location=%2&key=%3")
            .arg(apiHost)
            .arg(city)
            .arg(apiKey));

    qDebug() << "实时天气请求URL:" << weatherUrl.toString();

    // 发送天气数据请求
    QNetworkRequest weatherRequest(weatherUrl);
    weatherRequest.setRawHeader("User-Agent", "Qt-WeatherApp/1.0");
    weatherRequest.setRawHeader("Accept", "application/json");
    m_networkManager->get(weatherRequest);

    // 请求分钟级降水预报
    QUrl precipitationUrl(QString("https://%1/v7/minutely/5m?location=%2&key=%3")
            .arg(apiHost)
            .arg(city)
            .arg(apiKey));

    qDebug() << "降水预报请求URL:" << precipitationUrl.toString();

    QNetworkRequest precipitationRequest(precipitationUrl);
    precipitationRequest.setRawHeader("User-Agent", "Qt-WeatherApp/1.0");
    precipitationRequest.setRawHeader("Accept", "application/json");
    m_networkManager->get(precipitationRequest);

    // 请求天气预警信息
    QUrl warningUrl(QString("https://%1/v7/warning/now?location=%2&key=%3")
            .arg(apiHost)
            .arg(city)
            .arg(apiKey));

    qDebug() << "天气预警请求URL:" << warningUrl.toString();

    QNetworkRequest warningRequest(warningUrl);
    warningRequest.setRawHeader("User-Agent", "Qt-WeatherApp/1.0");
    warningRequest.setRawHeader("Accept", "application/json");
    m_networkManager->get(warningRequest);
}

void WeatherService::setCity(const QString &city)
{
    if (m_city != city) {
        m_city = city;
        qDebug() << "城市设置为:" << city;

        // 如果自动更新已启用，立即获取新城市的天气数据
        if (m_autoUpdateEnabled && !m_isUpdating) {
            fetchWeatherData();
        }
    }
}

void WeatherService::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
    qDebug() << "API密钥已设置";
}

void WeatherService::setAutoUpdate(bool enabled, int intervalMinutes)
{
    m_autoUpdateEnabled = enabled;
    m_updateInterval = intervalMinutes;

    if (enabled) {
        m_autoUpdateTimer->start(intervalMinutes * 60 * 1000); // 转换为毫秒
        qDebug() << QString("自动更新已启用，间隔: %1分钟").arg(intervalMinutes);

        // 立即获取一次天气数据
        if (!m_isUpdating) {
            fetchWeatherData();
        }
    } else {
        m_autoUpdateTimer->stop();
        qDebug() << "自动更新已禁用";
    }
}

void WeatherService::onNetworkReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("网络请求失败: %1").arg(reply->errorString());
        qWarning() << error;
        emit updateFailed(error);
        reply->deleteLater();
        m_isUpdating = false;
        return;
    }

    const QByteArray data = reply->readAll();
    const QString url = reply->url().toString();
    qDebug() << "收到响应数据，URL:" << url;

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonObject root = doc.object();

    // 根据URL判断响应类型
    if (url.contains("/weather/now")) {
        // 解析实时天气数据
        parseWeatherResponse(root);
    } else if (url.contains("/warning/now")) {
        // 解析天气预警数据
        parseWarningResponse(root);
    } else if (url.contains("/minutely/")) {
        // 解析降水预报数据
        parsePrecipitationResponse(root);
    }

    reply->deleteLater();
    m_isUpdating = false;
}

void WeatherService::parseWeatherResponse(const QJsonObject &root)
{
    qDebug() << "解析实时天气响应:" << QJsonDocument(root).toJson(QJsonDocument::Compact);

    if (root["code"].toString() != "200") {
        QString error = QString("和风天气API错误: code=%1").arg(root["code"].toString());
        qWarning() << error;
        emit updateFailed(error);
        return;
    }

    const QJsonObject now = root["now"].toObject();

    WeatherData weatherData;
    weatherData.cityName = "沈阳"; // 固定城市名
    weatherData.temperature = now["temp"].toString();
    weatherData.humidity = now["humidity"].toString();
    weatherData.description = now["text"].toString();
    weatherData.windDirection = now["windDir"].toString();
    weatherData.windSpeed = now["windSpeed"].toString();
    weatherData.pressure = now["pressure"].toString();
    weatherData.visibility = now["vis"].toString();
    weatherData.feelLike = now["feelsLike"].toString();
    weatherData.updateTime = now["obsTime"].toString();

    // 添加更多字段
    weatherData.uvIndex = now["uv"].toString();
    weatherData.airQuality = ""; // 实时天气API不包含空气质量，需要单独请求
    weatherData.solarRadiation = now["dew"].toString(); // 使用露点温度作为太阳辐射的替代指标
    weatherData.precipitation = now["precip"].toString(); // 降水量

    weatherData.isValid = !weatherData.temperature.isEmpty() && !weatherData.description.isEmpty();

    if (weatherData.isValid) {
        m_currentWeather = weatherData;
        emit weatherDataUpdated(weatherData);
        qDebug() << QString("和风天气数据解析成功: %1, %2℃, 湿度%3%, %4, 风向%5, 风速%6km/h")
                 .arg(weatherData.cityName).arg(weatherData.temperature)
                 .arg(weatherData.humidity).arg(weatherData.description)
                 .arg(weatherData.windDirection).arg(weatherData.windSpeed);
    } else {
        QString error = "天气数据字段不完整";
        qWarning() << error;
        qDebug() << "now对象内容:" << QJsonDocument(now).toJson(QJsonDocument::Compact);
        emit updateFailed(error);
    }
}

void WeatherService::parseWarningResponse(const QJsonObject &root)
{
    qDebug() << "解析天气预警响应:" << QJsonDocument(root).toJson(QJsonDocument::Compact);

    if (root["code"].toString() != "200") {
        qWarning() << "天气预警API错误:" << root["code"].toString();
        // 发送空预警信息
        WeatherWarning warning;
        warning.title = "暂无预警信息";
        warning.level = "无";
        warning.type = "无";
        warning.description = "当前无天气预警";
        warning.isValid = true;

        m_currentWarning = warning;
        emit warningUpdated(warning);
        return;
    }

    const QJsonArray warningArray = root["warning"].toArray();

    WeatherWarning warning;

    if (!warningArray.isEmpty()) {
        // 取第一个预警信息
        const QJsonObject warningObj = warningArray[0].toObject();

        warning.title = warningObj["title"].toString();
        warning.level = warningObj["level"].toString();
        warning.type = warningObj["type"].toString();
        warning.description = warningObj["text"].toString();
        warning.startTime = warningObj["startTime"].toString();
        warning.endTime = warningObj["endTime"].toString();
        warning.isValid = true;

        qDebug() << QString("天气预警解析成功: %1 - %2级").arg(warning.title).arg(warning.level);
    } else {
        // 无预警信息
        warning.title = "暂无预警信息";
        warning.level = "无";
        warning.type = "无";
        warning.description = "当前无天气预警";
        warning.isValid = true;

        qDebug() << "当前无天气预警信息";
    }

    m_currentWarning = warning;
    emit warningUpdated(warning);
}

void WeatherService::parsePrecipitationResponse(const QJsonObject &root)
{
    qDebug() << "解析降水预报响应:" << QJsonDocument(root).toJson(QJsonDocument::Compact);

    if (root["code"].toString() != "200") {
        qWarning() << "降水预报API错误:" << root["code"].toString();
        // 即使API失败，也要发送一个默认的降水预报
        PrecipitationForecast forecast;
        forecast.summary = "降水预报服务暂不可用";
        forecast.probability = "未知";
        forecast.intensity = "未知";
        forecast.type = "未知";
        forecast.isValid = true;

        m_currentPrecipitation = forecast;
        emit precipitationUpdated(forecast);
        return;
    }

    PrecipitationForecast forecast;
    forecast.summary = root["summary"].toString();

    const QJsonArray minutely = root["minutely"].toArray();
    if (!minutely.isEmpty()) {
        // 计算降水概率和强度
        int precipCount = 0;
        double totalPrecip = 0.0;
        int totalMinutes = minutely.size();

        for (const QJsonValue &value : minutely) {
            const QJsonObject minute = value.toObject();
            double precip = minute["precip"].toDouble();
            if (precip > 0) {
                precipCount++;
                totalPrecip += precip;
            }
        }

        if (totalMinutes > 0) {
            forecast.probability = QString("%1%").arg(precipCount * 100 / totalMinutes);
        } else {
            forecast.probability = "0%";
        }

        if (totalPrecip > 0) {
            forecast.intensity = QString("%.1fmm").arg(totalPrecip);
            forecast.type = "降雨";
        } else {
            forecast.intensity = "无降水";
            forecast.type = "无降水";
        }

        qDebug() << QString("降水分析: 总分钟数=%1, 有降水分钟数=%2, 总降水量=%.2fmm")
                    .arg(totalMinutes).arg(precipCount).arg(totalPrecip);
    } else {
        // 如果没有分钟级数据，提供基本信息
        if (forecast.summary.isEmpty()) {
            forecast.summary = "未来2小时无明显降水";
        }
        forecast.probability = "0%";
        forecast.intensity = "无降水";
        forecast.type = "无降水";
        qDebug() << "没有分钟级降水数据，使用默认信息";
    }

    // 确保总是有有效的降水预报数据
    forecast.isValid = true;

    m_currentPrecipitation = forecast;
    emit precipitationUpdated(forecast);
    qDebug() << QString("降水预报数据解析完成: %1, 概率%2, 强度%3")
                .arg(forecast.summary).arg(forecast.probability).arg(forecast.intensity);
}

void WeatherService::onAutoUpdateTimer()
{
    qDebug() << "自动更新天气数据";
    fetchWeatherData();
}

QString WeatherService::buildApiUrl() const
{
    // 使用心知天气API
    QString encodedCity = QString::fromUtf8(QUrl::toPercentEncoding(m_city));
    return QString("http://api.seniverse.com/v3/weather/now.json?key=%1&location=%2&language=zh-Hans&unit=c")
           .arg(m_apiKey)
           .arg(encodedCity);
}

// parseWeatherResponse函数已移除，直接在onNetworkReplyFinished中处理

QString WeatherService::formatWeatherDisplay(const WeatherData &data) const
{
    if (!data.isValid) {
        return "天气数据无效";
    }

    QString display = QString("🌍 %1天气实况\n").arg(data.cityName);
    display += QString("🌡️ 温度: %1℃ (体感%2℃)\n").arg(data.temperature).arg(data.feelLike);
    display += QString("💧 湿度: %1%  🌪️ 风向: %2\n").arg(data.humidity).arg(data.windDirection);
    display += QString("💨 风速: %1km/h  📊 气压: %2hPa\n").arg(data.windSpeed).arg(data.pressure);
    display += QString("☀️ 天气: %1").arg(data.description);

    return display;
}



QString WeatherService::formatPrecipitationDisplay(const PrecipitationForecast &forecast) const
{
    if (!forecast.isValid) {
        return "降水预报数据无效";
    }

    QString display = "🌧️ 降水预报\n";
    display += QString("📝 概况: %1\n").arg(forecast.summary);
    display += QString("📊 概率: %1  💧 强度: %2\n").arg(forecast.probability).arg(forecast.intensity);
    display += QString("🌦️ 类型: %1").arg(forecast.type);

    return display;
}
