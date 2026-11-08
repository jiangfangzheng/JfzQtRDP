#pragma once

#include "serverconfig.h"

#include <QMainWindow>

class QLabel;
class QStackedWidget;
class QTabBar;
class RdpSession;
class TitleBar;
class TitleTabBar;

namespace QWK {
class WidgetWindowAgent;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const AppConfig &config, const QString &configError,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

    void applyInitialWindowState();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void setupWindowAgent();
    void createHomeTab();
    void applyWindowOptions();
    void openConnection(const ServerInfo &server, const QString &tabName,
                        bool savedConnection);
    int findExistingConnection(const QString &nameKey, const QString &endpointKey) const;
    void activateTab(int index);
    void closeTab(int index);
    bool confirmCloseTab(int index);
    void updateMaximizeButton();
    void ensureNotMaximizedWhenFixed();
    void syncWindowAppearance();
    bool confirmClose();

    AppConfig m_config;
    QString m_configError;
    QWK::WidgetWindowAgent *m_windowAgent = nullptr;
    TitleBar *m_titleBar = nullptr;
    TitleTabBar *m_tabBar = nullptr;
    QStackedWidget *m_stack = nullptr;
    bool m_fixedSize = false;
    bool m_closeConfirmed = false;
    bool m_restoringFromMaximize = false;
};
