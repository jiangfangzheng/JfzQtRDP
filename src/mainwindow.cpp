#include "mainwindow.h"

#include "appearance.h"
#include "captionbutton.h"
#include "homepage.h"
#include "rdpsession.h"
#include "titlebar.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStringList>
#include <QStyle>
#include <QTabBar>
#include <QTimer>

#include <QWKWidgets/widgetwindowagent.h>

MainWindow::MainWindow(const AppConfig &config, const QString &configError, QWidget *parent)
    : QMainWindow(parent)
    , m_config(config)
    , m_configError(configError)
{
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setWindowTitle(tr("JfzQtRDP - Windows RDP 客户端"));
    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon));

    setupWindowAgent();

    connect(AppearanceController::instance(), &AppearanceController::stateChanged,
            this, &MainWindow::syncWindowAppearance);
    syncWindowAppearance();

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("MainStack"));
    setCentralWidget(m_stack);

    connect(m_tabBar, &QTabBar::currentChanged, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_tabBar, &TitleTabBar::middleClicked, this, &MainWindow::closeTab);

    createHomeTab();
    applyWindowOptions();

    if (!m_configError.isEmpty()) {
        const QString error = m_configError;
        QTimer::singleShot(0, this, [this, error] {
            QMessageBox::warning(this, tr("配置文件"),
                                 tr("读取 server.json 时遇到问题：\n%1\n\n"
                                    "程序已使用默认配置启动，原文件未被覆盖。")
                                     .arg(error));
        });
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupWindowAgent()
{
    m_windowAgent = new QWK::WidgetWindowAgent(this);
    m_windowAgent->setup(this);

    m_titleBar = new TitleBar(this);
    m_tabBar = m_titleBar->tabBar();
    setMenuWidget(m_titleBar);

    m_windowAgent->setTitleBar(m_titleBar);
    m_windowAgent->setHitTestVisible(m_tabBar, true);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::WindowIcon,
                                   m_titleBar->iconLabel());
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize,
                                   m_titleBar->minimizeButton());
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize,
                                   m_titleBar->maximizeButton());
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Close,
                                   m_titleBar->closeButton());

    connect(m_titleBar->minimizeButton(), &QAbstractButton::clicked, this,
            &QWidget::showMinimized);
    connect(m_titleBar->maximizeButton(), &QAbstractButton::clicked, this, [this] {
        if (m_fixedSize) {
            return;
        }
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(m_titleBar->closeButton(), &QAbstractButton::clicked, this, &QWidget::close);
}

void MainWindow::createHomeTab()
{
    auto *home = new HomePage(m_config, this);
    m_stack->addWidget(home);
    const int index = m_tabBar->addTab(tr("Home"));
    m_tabBar->setTabToolTip(index, tr("主页"));
    m_tabBar->setCurrentIndex(index);
    m_stack->setCurrentIndex(index);

    connect(home, &HomePage::connectRequested, this, &MainWindow::openConnection);
}

void MainWindow::applyWindowOptions()
{
    const int titleBarHeight = m_titleBar->height();

    if (m_config.maximized) {
        m_fixedSize = false;
        setMinimumSize(900, 600);
        resize(m_config.width, m_config.height + titleBarHeight);
        m_titleBar->setMaximizeEnabled(true);
        return;
    }

    m_fixedSize = true;
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setFixedSize(m_config.width, m_config.height + titleBarHeight);
    m_titleBar->setMaximizeEnabled(false);
    updateMaximizeButton();

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        move(available.center() - QPoint(width() / 2, height() / 2));
    }
}

void MainWindow::applyInitialWindowState()
{
    if (m_config.maximized) {
        showMaximized();
    } else {
        show();
    }

    QTimer::singleShot(0, this, &MainWindow::syncWindowAppearance);
}

void MainWindow::openConnection(const ServerInfo &server, const QString &tabName,
                                bool savedConnection)
{
    QString name = tabName.trimmed();
    name.replace(QLatin1Char('\r'), QLatin1Char(' '));
    name.replace(QLatin1Char('\n'), QLatin1Char(' '));
    if (name.isEmpty()) {
        name = server.server;
    }

    const QString nameKey = savedConnection
        ? QStringLiteral("name:") + server.serverName.trimmed().toCaseFolded()
        : QString();
    const QString endpointKey =
        QStringLiteral("endpoint:") + QStringList{server.server.trimmed().toCaseFolded(),
                                                  server.userName.trimmed().toCaseFolded(),
                                                  QString::number(server.port)}
                                        .join(QChar(0x1F));

    const int existingIndex = findExistingConnection(nameKey, endpointKey);
    if (existingIndex >= 0) {
        qInfo().noquote() << "RDP duplicate request:" << name
                          << "-> switching to existing tab" << existingIndex;
        activateTab(existingIndex);
        return;
    }

    auto *session = new RdpSession(server, m_stack);
    session->setObjectName(QStringLiteral("RdpSession"));
    m_stack->addWidget(session);

    const int index = m_tabBar->addTab(name);
    m_tabBar->setTabData(index, QStringList{nameKey, endpointKey});
    m_tabBar->setTabToolTip(index, tr("服务器：%1\n账号：%2\n端口：%3\n\n"
                                      "鼠标中键点击标签页可断开连接")
                                   .arg(server.server, server.userName)
                                   .arg(server.port));
    m_tabBar->setCurrentIndex(index);
    m_stack->setCurrentIndex(index);
    qInfo().noquote() << "RDP tab opened:" << name << "| target:" << server.server
                      << ":" << server.port;

    QTimer::singleShot(0, session, &RdpSession::start);
}

int MainWindow::findExistingConnection(const QString &nameKey,
                                       const QString &endpointKey) const
{
    for (int i = 1; i < m_tabBar->count(); ++i) {
        const QStringList keys = m_tabBar->tabData(i).toStringList();
        if (keys.size() < 2) {
            continue;
        }
        if (!nameKey.isEmpty() && keys.at(0) == nameKey) {
            return i;
        }
        if (!endpointKey.isEmpty() && keys.at(1) == endpointKey) {
            return i;
        }
    }
    return -1;
}

void MainWindow::activateTab(int index)
{
    if (index < 0 || index >= m_tabBar->count() || index >= m_stack->count()) {
        return;
    }

    m_tabBar->setCurrentIndex(index);
    m_stack->setCurrentIndex(index);

    if (isMinimized()) {
        showNormal();
    }
    raise();
    activateWindow();

    if (auto *session = qobject_cast<RdpSession *>(m_stack->widget(index))) {
        session->focusInput();
    }
}

void MainWindow::closeTab(int index)
{
    if (index <= 0 || index >= m_tabBar->count() || index >= m_stack->count()) {
        return;
    }

    QWidget *page = m_stack->widget(index);
    auto *session = qobject_cast<RdpSession *>(page);
    if (session && !confirmCloseTab(index)) {
        return;
    }

    if (session) {
        session->prepareToClose();
    }

    const int oldCurrent = m_tabBar->currentIndex();
    int newCurrent = oldCurrent;

    {
        const QSignalBlocker tabBlocker(m_tabBar);
        const QSignalBlocker stackBlocker(m_stack);

        m_tabBar->removeTab(index);
        m_stack->removeWidget(page);
        if (page) {
            page->deleteLater();
        }

        if (index < oldCurrent) {
            --newCurrent;
        } else if (index == oldCurrent) {
            newCurrent = qMin(index, m_tabBar->count() - 1);
        }
        newCurrent = qBound(0, newCurrent, qMax(0, m_tabBar->count() - 1));

        m_tabBar->setCurrentIndex(newCurrent);
        m_stack->setCurrentIndex(newCurrent);
    }

    m_tabBar->refreshSelectionAppearance();
}

bool MainWindow::confirmCloseTab(int index)
{
    const QString tabName = m_tabBar->tabText(index);

    QMessageBox box(this);
    box.setWindowTitle(tr("断开连接"));
    box.setIcon(QMessageBox::Question);
    box.setTextFormat(Qt::PlainText);
    box.setText(tr("确定要断开并关闭标签页“%1”吗？").arg(tabName));
    box.setInformativeText(tr("该远程桌面连接将被断开。"));

    auto *yesButton = box.addButton(tr("是"), QMessageBox::YesRole);
    auto *noButton = box.addButton(tr("否"), QMessageBox::NoRole);
    box.setDefaultButton(noButton);
    box.setEscapeButton(noButton);
    yesButton->setAutoDefault(false);
    noButton->setAutoDefault(true);

    QTimer::singleShot(0, noButton, [noButton] {
        noButton->setFocus(Qt::OtherFocusReason);
    });

    box.exec();
    return box.clickedButton() == yesButton;
}

void MainWindow::updateMaximizeButton()
{
    m_titleBar->setMaximized(isMaximized());
    m_titleBar->setMaximizeEnabled(!m_fixedSize);
}

void MainWindow::syncWindowAppearance()
{
    if (!m_windowAgent) {
        return;
    }

    const bool dark = AppearanceController::instance()->isDark();
    m_windowAgent->setWindowAttribute(QStringLiteral("dark-mode"), dark);
    if (m_titleBar) {
        m_titleBar->update();
        m_tabBar->refreshSelectionAppearance();
    }
    update();
}

void MainWindow::ensureNotMaximizedWhenFixed()
{
    if (!m_fixedSize || !isMaximized() || m_restoringFromMaximize) {
        return;
    }

    m_restoringFromMaximize = true;
    QTimer::singleShot(0, this, [this] {
        if (m_fixedSize && isMaximized()) {
            showNormal();
        }
        m_restoringFromMaximize = false;
    });
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButton();
        ensureNotMaximizedWhenFixed();
    }
}

bool MainWindow::confirmClose()
{
    QMessageBox box(this);
    box.setWindowTitle(tr("退出确认"));
    box.setIcon(QMessageBox::Question);
    box.setText(tr("确定要关闭 JfzQtRDP 吗？"));
    box.setInformativeText(tr("关闭后，所有远程桌面连接都会断开。"));

    auto *yesButton = box.addButton(tr("是"), QMessageBox::YesRole);
    auto *noButton = box.addButton(tr("否"), QMessageBox::NoRole);
    box.setDefaultButton(noButton);
    box.setEscapeButton(noButton);
    yesButton->setAutoDefault(false);
    noButton->setAutoDefault(true);

    QTimer::singleShot(0, noButton, [noButton] {
        noButton->setFocus(Qt::OtherFocusReason);
    });

    box.exec();
    return box.clickedButton() == yesButton;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_closeConfirmed) {
        event->accept();
        return;
    }

    if (!confirmClose()) {
        event->ignore();
        return;
    }

    m_closeConfirmed = true;
    const QList<RdpSession *> sessions = findChildren<RdpSession *>();
    for (RdpSession *session : sessions) {
        session->prepareToClose();
    }
    event->accept();
}
