#pragma once

#include "serverconfig.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

class QAxWidget;
class QLabel;
class QStackedWidget;

class RdpSession : public QWidget
{
    Q_OBJECT
public:
    explicit RdpSession(const ServerInfo &server, QWidget *parent = nullptr);
    ~RdpSession() override;

    QString serverName() const;
    const ServerInfo &serverInfo() const;
    bool isConnected() const;

    void start();
    void prepareToClose();
    void focusInput();

private:
    enum class Phase {
        Idle,
        Connecting,
        Connected,
        Reconnecting,
        Failed,
    };

    bool createControl();
    void applyConnectionSettings();
    void beginConnect();
    void beginReconnectAttempt();
    void scheduleReconnect();
    void pollState();
    void onConnected();
    void onUnexpectedDisconnect();
    void showFailure(const QString &title, const QString &detail);
    void setBanner(const QString &text);
    int connectedState() const;

    ServerInfo m_server;
    QAxWidget *m_ax = nullptr;
    QLabel *m_banner = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_livePage = nullptr;
    QWidget *m_failurePage = nullptr;
    QLabel *m_failureTitle = nullptr;
    QLabel *m_failureDetail = nullptr;
    QTimer m_pollTimer;
    QElapsedTimer m_attemptElapsed;
    Phase m_phase = Phase::Idle;
    int m_reconnectAttempt = 0;
    bool m_connected = false;
    bool m_everConnected = false;
    bool m_closing = false;
    bool m_started = false;
};
