#pragma once

#include "serverconfig.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;

class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(const AppConfig &config, QWidget *parent = nullptr);

signals:
    void connectRequested(const ServerInfo &server, const QString &tabName, bool savedConnection);

private:
    QWidget *createManualGroup();
    QWidget *createAppearanceGroup();
    QWidget *createSavedGroup();
    void connectManual();
    void connectSelected();
    void updateSelectedServer();
    void syncAppearanceControls();

    AppConfig m_config;

    QLineEdit *m_serverEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QSpinBox *m_portSpin = nullptr;
    QPushButton *m_linkButton = nullptr;

    QWidget *m_appearanceGroup = nullptr;
    QComboBox *m_styleCombo = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QLabel *m_appearanceHint = nullptr;

    QListWidget *m_serverList = nullptr;
    QPushButton *m_savedConnectButton = nullptr;
    QLabel *m_infoLabel = nullptr;
    QLabel *m_serverValue = nullptr;
    QLabel *m_userValue = nullptr;
    QLabel *m_passwordValue = nullptr;
    QLabel *m_portValue = nullptr;
};
