#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

struct ServerInfo
{
    QString serverName;
    QString server;
    QString userName;
    QString password;
    int port = 3389;
};

struct AppConfig
{
    bool maximized = false;
    int width = 1920;
    int height = 1080;
    QList<ServerInfo> servers;
};

class ServerConfigStore
{
public:
    static QString filePath();
    static AppConfig defaultConfig();

    // Loads server.json next to the executable. If it does not exist, writes a
    // default file first. On a read/parse error the defaults are returned and
    // errorMessage is filled in; the bad file is never overwritten.
    static AppConfig loadOrCreate(QString *errorMessage);

    static bool save(const AppConfig &config, QString *errorMessage);

private:
    static QJsonObject toJson(const AppConfig &config);
    static AppConfig fromJson(const QJsonObject &root);
};
