#include "mainwindow.h"
#include "serverconfig.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStyle>
#include <QStyleFactory>

#include <cstdio>

namespace {

QFile g_logFile;
QMutex g_logMutex;

const char *messageLevel(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return "DEBUG";
    case QtInfoMsg:
        return "INFO";
    case QtWarningMsg:
        return "WARN";
    case QtCriticalMsg:
        return "ERROR";
    case QtFatalMsg:
        return "FATAL";
    }
    return "LOG";
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    Q_UNUSED(context)

    const QString line = QStringLiteral("[%1] [%2] %3\n")
                             .arg(QDateTime::currentDateTime().toString(
                                      QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                  QString::fromLatin1(messageLevel(type)), message);

    {
        QMutexLocker locker(&g_logMutex);
        if (g_logFile.isOpen()) {
            g_logFile.write(line.toUtf8());
            g_logFile.flush();
        }
    }

    std::fputs(line.toLocal8Bit().constData(), stderr);
    std::fflush(stderr);
}

void openLogFile()
{
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("JfzQtRDP.log"));
    g_logFile.setFileName(path);
    (void)g_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("JfzQtRDP"));
    QApplication::setApplicationDisplayName(QStringLiteral("Windows RDP 客户端"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("Jfz"));

    openLogFile();
    qInstallMessageHandler(messageHandler);

    if (QStyleFactory::keys().contains(QStringLiteral("windows11"), Qt::CaseInsensitive)) {
        if (QStyle *windows11Style =
                QStyleFactory::create(QStringLiteral("windows11"))) {
            QApplication::setStyle(windows11Style);
        }
    }

    qInfo().noquote() << "JfzQtRDP 1.0.0 starting"
                      << "| style:" << (app.style() ? app.style()->objectName() : QString())
                      << "| Qt:" << qVersion();

    QString configError;
    const AppConfig config = ServerConfigStore::loadOrCreate(&configError);
    qInfo().noquote() << "server.json:" << ServerConfigStore::filePath()
                      << "| max:" << config.maximized << "| size:" << config.width
                      << "x" << config.height << "| servers:" << config.servers.size();
    if (!configError.isEmpty()) {
        qWarning().noquote() << "configuration issue:" << configError;
    }

    MainWindow window(config, configError);
    window.applyInitialWindowState();

    const int result = app.exec();
    qInfo() << "exit code" << result;
    return result;
}
