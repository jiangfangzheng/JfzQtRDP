#include "serverconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>

namespace {

int boundedInt(const QJsonValue &value, int fallback, int minimum, int maximum)
{
    const int number = value.toInt(fallback);
    return qBound(minimum, number, maximum);
}

QString cleanText(const QString &text)
{
    QString result = text;
    result.replace(QLatin1Char('\r'), QLatin1Char(' '));
    result.replace(QLatin1Char('\n'), QLatin1Char(' '));
    return result.trimmed();
}

} // namespace

QString ServerConfigStore::filePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("server.json"));
}

AppConfig ServerConfigStore::defaultConfig()
{
    AppConfig config;
    config.maximized = false;
    config.width = 1920;
    config.height = 1080;

    ServerInfo first;
    first.serverName = QStringLiteral("jfz123");
    first.server = QStringLiteral("127.0.0.1");
    first.userName = QStringLiteral("Sandeepin");
    first.password = QStringLiteral("123456");
    first.port = 3389;

    ServerInfo second;
    second.serverName = QStringLiteral("JfzVictus-192");
    second.server = QStringLiteral("192.168.233.168");
    second.userName = QStringLiteral("Sandeepin");
    second.password = QStringLiteral("1");
    second.port = 3389;

    config.servers = {first, second};
    return config;
}

AppConfig ServerConfigStore::loadOrCreate(QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    const QString path = filePath();
    QFile file(path);
    if (!file.exists()) {
        const AppConfig config = defaultConfig();
        QString writeError;
        if (!save(config, &writeError) && errorMessage) {
            *errorMessage = QStringLiteral("无法创建默认配置文件 %1：%2").arg(path, writeError);
        }
        return config;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法读取 %1：%2").arg(path, file.errorString());
        }
        return defaultConfig();
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("server.json 格式错误（第 %1 字节：%2），已使用默认配置。")
                                .arg(parseError.offset)
                                .arg(parseError.errorString());
        }
        return defaultConfig();
    }

    return fromJson(document.object());
}

bool ServerConfigStore::save(const AppConfig &config, QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    const QString path = filePath();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    const QJsonDocument document(toJson(config));
    if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

QJsonObject ServerConfigStore::toJson(const AppConfig &config)
{
    QJsonArray serverArray;
    for (const ServerInfo &server : config.servers) {
        QJsonObject item;
        item.insert(QStringLiteral("ServerName"), cleanText(server.serverName));
        item.insert(QStringLiteral("Server"), cleanText(server.server));
        item.insert(QStringLiteral("UserName"), server.userName);
        item.insert(QStringLiteral("Password"), server.password);
        item.insert(QStringLiteral("Port"), qBound(1, server.port, 65535));
        serverArray.append(item);
    }

    QJsonObject root;
    root.insert(QStringLiteral("Maximized"), config.maximized);
    root.insert(QStringLiteral("Width"), qBound(320, config.width, 16384));
    root.insert(QStringLiteral("Height"), qBound(240, config.height, 16384));
    root.insert(QStringLiteral("ServerList"), serverArray);
    return root;
}

AppConfig ServerConfigStore::fromJson(const QJsonObject &root)
{
    AppConfig config;
    config.maximized = root.value(QStringLiteral("Maximized")).toBool(false);
    config.width = boundedInt(root.value(QStringLiteral("Width")), 1920, 320, 16384);
    config.height = boundedInt(root.value(QStringLiteral("Height")), 1080, 240, 16384);

    const QJsonArray serverArray = root.value(QStringLiteral("ServerList")).toArray();
    for (const QJsonValue &value : serverArray) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject item = value.toObject();
        ServerInfo server;
        server.server = cleanText(item.value(QStringLiteral("Server")).toString());
        if (server.server.isEmpty()) {
            continue;
        }
        server.serverName = cleanText(item.value(QStringLiteral("ServerName")).toString());
        if (server.serverName.isEmpty()) {
            server.serverName = server.server;
        }
        server.userName = item.value(QStringLiteral("UserName")).toString();
        server.password = item.value(QStringLiteral("Password")).toString();
        server.port = boundedInt(item.value(QStringLiteral("Port")), 3389, 1, 65535);
        config.servers.append(server);
    }

    return config;
}
