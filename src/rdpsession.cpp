#include "rdpsession.h"

#include <QtAxContainer/QAxObject>
#include <QtAxContainer/QAxWidget>

#include <QDebug>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

constexpr int kPollIntervalMs = 500;
constexpr int kInitialConnectTimeoutMs = 30000;
constexpr int kInitialDisconnectGraceMs = 5000;
constexpr int kReconnectTimeoutMs = 12000;
constexpr int kMaxReconnectAttempts = 3;

QString connectionBarText(const ServerInfo &server)
{
    return QStringLiteral("%1:%2").arg(server.server).arg(server.port);
}

} // namespace

RdpSession::RdpSession(const ServerInfo &server, QWidget *parent)
    : QWidget(parent)
    , m_server(server)
{
    setObjectName(QStringLiteral("RdpSession"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("RdpSessionStack"));
    root->addWidget(m_stack);

    m_livePage = new QWidget(m_stack);
    auto *liveLayout = new QVBoxLayout(m_livePage);
    liveLayout->setContentsMargins(0, 0, 0, 0);
    liveLayout->setSpacing(0);

    m_banner = new QLabel(m_livePage);
    m_banner->setObjectName(QStringLiteral("RdpStatusBanner"));
    m_banner->setContentsMargins(10, 4, 10, 4);
    m_banner->setMinimumHeight(28);
    m_banner->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_banner->setVisible(false);

    m_ax = new QAxWidget(m_livePage);
    m_ax->setObjectName(QStringLiteral("RdpActiveX"));
    m_ax->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_ax->setMinimumSize(1, 1);

    liveLayout->addWidget(m_banner);
    liveLayout->addWidget(m_ax, 1);

    m_failurePage = new QWidget(m_stack);
    auto *failureLayout = new QVBoxLayout(m_failurePage);
    failureLayout->setContentsMargins(32, 32, 32, 32);
    failureLayout->setSpacing(10);
    failureLayout->addStretch(1);

    m_failureTitle = new QLabel(m_failurePage);
    m_failureTitle->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_failureTitle->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() + 2.0);
    titleFont.setBold(true);
    m_failureTitle->setFont(titleFont);
    m_failureTitle->setWordWrap(true);

    m_failureDetail = new QLabel(m_failurePage);
    m_failureDetail->setAlignment(Qt::AlignCenter);
    m_failureDetail->setWordWrap(true);
    m_failureDetail->setTextInteractionFlags(Qt::TextSelectableByMouse);

    failureLayout->addWidget(m_failureTitle);
    failureLayout->addWidget(m_failureDetail);
    failureLayout->addStretch(1);

    m_stack->addWidget(m_livePage);
    m_stack->addWidget(m_failurePage);
    m_stack->setCurrentWidget(m_livePage);

    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &RdpSession::pollState);
}

RdpSession::~RdpSession()
{
    prepareToClose();
}

QString RdpSession::serverName() const
{
    return m_server.serverName;
}

const ServerInfo &RdpSession::serverInfo() const
{
    return m_server;
}

bool RdpSession::isConnected() const
{
    return m_connected;
}

void RdpSession::start()
{
    if (m_started) {
        return;
    }
    m_started = true;
    m_phase = Phase::Connecting;

    if (!createControl()) {
        showFailure(tr("无法启动远程桌面"),
                    tr("未能加载 Windows RDP ActiveX 控件（mstscax.dll）。\n"
                       "请确认系统为 Windows 且远程桌面客户端组件可用。"));
        return;
    }

    m_stack->setCurrentWidget(m_livePage);
    setBanner(tr("正在连接 %1 …").arg(connectionBarText(m_server)));

    // Let the native ActiveX host window be created and laid out before
    // configuring and starting the session.
    QTimer::singleShot(250, this, [this] {
        if (m_closing) {
            return;
        }
        applyConnectionSettings();
        beginConnect();
    });
}

bool RdpSession::createControl()
{
    static const QStringList classIds = {
        QStringLiteral("{1DF7C823-B2D4-4B54-975A-F2AC5D7CF8B8}"), // version 12
        QStringLiteral("{A0C63C30-F08D-4AB4-907C-34905D770C7D}"), // version 11
        QStringLiteral("{8B918B82-7985-4C24-89DF-C33AD2BBFBCD}"), // version 10
        QStringLiteral("{A3BC03A0-041D-42E3-AD22-882B7865C9C5}"), // version 9
        QStringLiteral("MsTscAx.MsTscAx"),
    };

    for (const QString &classId : classIds) {
        if (m_ax->setControl(classId)) {
            qInfo().noquote() << "RDP: ActiveX control created:" << classId;
            return true;
        }
    }

    qWarning() << "RDP: no usable MSTSC ActiveX control was found";
    return false;
}

void RdpSession::applyConnectionSettings()
{
    QSize target = size();
    if (target.width() < 200 || target.height() < 150) {
        target = m_ax->size();
    }
    target.setWidth(qMax(200, target.width()));
    target.setHeight(qMax(200, target.height()));

    m_ax->setProperty("StartConnected", false);
    m_ax->setProperty("Server", m_server.server);
    m_ax->setProperty("UserName", m_server.userName);
    m_ax->setProperty("Domain", QString());
    m_ax->setProperty("DesktopWidth", target.width());
    m_ax->setProperty("DesktopHeight", target.height());
    m_ax->setProperty("ColorDepth", 32);
    m_ax->setProperty("FullScreen", false);

    QAxObject *advanced = m_ax->querySubObject("AdvancedSettings9");
    if (!advanced) {
        advanced = m_ax->querySubObject("AdvancedSettings8");
    }
    if (!advanced) {
        advanced = m_ax->querySubObject("AdvancedSettings");
    }

    if (advanced) {
        advanced->setProperty("RDPPort", qBound(1, m_server.port, 65535));
        if (!m_server.password.isEmpty()) {
            advanced->dynamicCall("SetClearTextPassword(QString)", m_server.password);
        }
        advanced->setProperty("SmartSizing", true);
        advanced->setProperty("EnableAutoReconnect", false);
        advanced->setProperty("EnableCredSspSupport", true);
        advanced->setProperty("DisplayConnectionBar", false);
        advanced->setProperty("PinConnectionBar", false);
        advanced->setProperty("RedirectClipboard", true);
        advanced->setProperty("GrabFocusOnConnect", true);
    } else {
        qWarning() << "RDP: advanced settings interface is unavailable";
    }

    qInfo().noquote() << QStringLiteral("RDP: connecting to %1:%2 as %3, desktop %4x%5")
                             .arg(m_server.server)
                             .arg(m_server.port)
                             .arg(m_server.userName)
                             .arg(target.width())
                             .arg(target.height());

    // The host window must be visible for the ActiveX control to paint.
    m_ax->show();
}

void RdpSession::beginConnect()
{
    if (m_closing) {
        return;
    }
    m_connected = false;
    m_attemptElapsed.start();
    m_ax->dynamicCall("Connect()");
    m_pollTimer.start();
}

void RdpSession::scheduleReconnect()
{
    if (m_closing) {
        return;
    }
    m_pollTimer.stop();
    if (m_reconnectAttempt >= kMaxReconnectAttempts) {
        showFailure(tr("连接已断开"),
                    tr("自动重连 %1 次仍然失败。\n"
                       "服务器：%2\n\n"
                       "可点击标签页中键关闭此连接。")
                        .arg(kMaxReconnectAttempts)
                        .arg(connectionBarText(m_server)));
        return;
    }

    ++m_reconnectAttempt;
    m_phase = Phase::Reconnecting;
    m_stack->setCurrentWidget(m_livePage);
    setBanner(tr("连接意外断开，正在自动重连（%1/%2）…")
                  .arg(m_reconnectAttempt)
                  .arg(kMaxReconnectAttempts));

    QTimer::singleShot(1200, this, [this] {
        if (m_closing || m_phase != Phase::Reconnecting) {
            return;
        }
        beginReconnectAttempt();
    });
}

void RdpSession::beginReconnectAttempt()
{
    if (m_closing) {
        return;
    }
    m_attemptElapsed.start();
    m_ax->dynamicCall("Connect()");
    m_pollTimer.start();
    qInfo().noquote() << QStringLiteral("RDP: reconnect attempt %1/%2")
                             .arg(m_reconnectAttempt)
                             .arg(kMaxReconnectAttempts);
}

void RdpSession::onConnected()
{
    if (m_connected) {
        return;
    }
    m_connected = true;
    m_everConnected = true;
    m_phase = Phase::Connected;
    m_reconnectAttempt = 0;
    setBanner(QString());
    m_banner->setVisible(false);
    m_stack->setCurrentWidget(m_livePage);
    m_ax->setFocus(Qt::OtherFocusReason);
    qInfo().noquote() << "RDP: connected to" << connectionBarText(m_server);
}

void RdpSession::onUnexpectedDisconnect()
{
    if (m_closing || m_phase == Phase::Failed || m_phase == Phase::Reconnecting) {
        return;
    }

    m_connected = false;
    if (!m_everConnected) {
        showFailure(tr("连接失败"),
                    tr("无法连接到服务器。\n"
                       "服务器：%1\n\n"
                       "请检查服务器地址、端口、账号和密码。")
                        .arg(connectionBarText(m_server)));
        return;
    }

    qWarning().noquote() << "RDP: connection to" << connectionBarText(m_server)
                         << "was lost; starting automatic reconnect";
    m_phase = Phase::Reconnecting;
    m_reconnectAttempt = 0;
    m_pollTimer.stop();
    m_stack->setCurrentWidget(m_livePage);
    setBanner(tr("连接意外断开，正在准备自动重连…"));
    QTimer::singleShot(800, this, [this] { scheduleReconnect(); });
}

void RdpSession::pollState()
{
    if (m_closing) {
        return;
    }

    const int state = connectedState();
    const qint64 elapsed = m_attemptElapsed.isValid() ? m_attemptElapsed.elapsed() : 0;

    if (m_phase == Phase::Connected) {
        if (state != 1) {
            onUnexpectedDisconnect();
        }
        return;
    }

    if (m_phase == Phase::Connecting) {
        if (state == 1) {
            onConnected();
            return;
        }
        if (state == 0 && elapsed >= kInitialDisconnectGraceMs) {
            showFailure(tr("连接失败"),
                        tr("无法连接到服务器。\n"
                           "服务器：%1\n\n"
                           "请检查服务器地址、端口、账号和密码。")
                            .arg(connectionBarText(m_server)));
            return;
        }
        if (elapsed >= kInitialConnectTimeoutMs) {
            showFailure(tr("连接超时"),
                        tr("连接服务器超时。\n"
                           "服务器：%1\n\n"
                           "请检查网络或服务器状态。")
                            .arg(connectionBarText(m_server)));
        }
        return;
    }

    if (m_phase == Phase::Reconnecting) {
        if (state == 1) {
            onConnected();
            return;
        }
        if (elapsed >= kReconnectTimeoutMs) {
            m_pollTimer.stop();
            if (m_reconnectAttempt >= kMaxReconnectAttempts) {
                showFailure(tr("连接已断开"),
                            tr("自动重连 %1 次仍然失败。\n"
                               "服务器：%2\n\n"
                               "可点击标签页中键关闭此连接。")
                                .arg(kMaxReconnectAttempts)
                                .arg(connectionBarText(m_server)));
            } else {
                scheduleReconnect();
            }
        }
    }
}

int RdpSession::connectedState() const
{
    if (!m_ax || m_ax->isNull()) {
        return 0;
    }
    const QVariant value = m_ax->property("Connected");
    if (!value.isValid()) {
        return 2;
    }
    return value.toInt();
}

void RdpSession::showFailure(const QString &title, const QString &detail)
{
    m_phase = Phase::Failed;
    m_connected = false;
    m_pollTimer.stop();
    m_banner->setVisible(false);
    m_failureTitle->setText(title);
    m_failureDetail->setText(detail);
    m_stack->setCurrentWidget(m_failurePage);
    qWarning().noquote() << "RDP:" << title << "-" << connectionBarText(m_server);
}

void RdpSession::setBanner(const QString &text)
{
    if (text.isEmpty()) {
        m_banner->clear();
        m_banner->setVisible(false);
        return;
    }
    m_banner->setText(text);
    m_banner->setVisible(true);
}

void RdpSession::prepareToClose()
{
    if (m_closing) {
        return;
    }
    m_closing = true;
    m_pollTimer.stop();
    m_connected = false;
    qInfo().noquote() << "RDP: disconnecting" << connectionBarText(m_server);
    if (m_ax && !m_ax->isNull()) {
        m_ax->dynamicCall("Disconnect()");
    }
}
