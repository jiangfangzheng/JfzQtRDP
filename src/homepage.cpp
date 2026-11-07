#include "homepage.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

HomePage::HomePage(const AppConfig &config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
{
    setObjectName(QStringLiteral("HomePage"));

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);
    root->addWidget(createManualGroup(), 0, Qt::AlignTop);
    root->addWidget(createSavedGroup(), 1);
}

QWidget *HomePage::createManualGroup()
{
    auto *group = new QGroupBox(tr("手动连接"), this);
    group->setObjectName(QStringLiteral("ManualConnectionGroup"));
    group->setMinimumWidth(440);
    group->setMaximumWidth(560);
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *outer = new QVBoxLayout(group);
    outer->setContentsMargins(12, 16, 12, 12);
    outer->setSpacing(10);

    auto *form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(8);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_serverEdit = new QLineEdit(QStringLiteral("192.168.233.254"), group);
    m_serverEdit->setObjectName(QStringLiteral("ManualServerEdit"));
    m_serverEdit->setClearButtonEnabled(true);
    m_serverEdit->setPlaceholderText(tr("服务器地址或主机名"));

    m_userEdit = new QLineEdit(QStringLiteral("Sandeepin"), group);
    m_userEdit->setObjectName(QStringLiteral("ManualUserEdit"));
    m_userEdit->setClearButtonEnabled(true);
    m_userEdit->setPlaceholderText(tr("登录账号"));

    m_passwordEdit = new QLineEdit(group);
    m_passwordEdit->setObjectName(QStringLiteral("ManualPasswordEdit"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("登录密码"));

    m_portSpin = new QSpinBox(group);
    m_portSpin->setObjectName(QStringLiteral("ManualPortSpin"));
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(3389);
    m_portSpin->setKeyboardTracking(false);
    m_portSpin->setAlignment(Qt::AlignRight);
    m_portSpin->setFixedWidth(140);

    form->addRow(tr("服务器："), m_serverEdit);
    form->addRow(tr("账号："), m_userEdit);
    form->addRow(tr("密码："), m_passwordEdit);
    form->addRow(tr("端口："), m_portSpin);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->addStretch(1);
    m_linkButton = new QPushButton(tr("Link"), group);
    m_linkButton->setObjectName(QStringLiteral("LinkButton"));
    m_linkButton->setAutoDefault(false);
    m_linkButton->setDefault(false);
    m_linkButton->setMinimumWidth(96);
    buttonRow->addWidget(m_linkButton);

    outer->addLayout(form);
    outer->addLayout(buttonRow);

    connect(m_linkButton, &QPushButton::clicked, this, &HomePage::connectManual);
    connect(m_serverEdit, &QLineEdit::returnPressed, this, &HomePage::connectManual);
    connect(m_userEdit, &QLineEdit::returnPressed, this, &HomePage::connectManual);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &HomePage::connectManual);

    return group;
}

QWidget *HomePage::createSavedGroup()
{
    auto *group = new QGroupBox(tr("保存的服务器列表"), this);
    group->setObjectName(QStringLiteral("SavedServersGroup"));
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *outer = new QVBoxLayout(group);
    outer->setContentsMargins(12, 16, 12, 12);
    outer->setSpacing(10);

    m_infoLabel = new QLabel(group);
    m_infoLabel->setObjectName(QStringLiteral("ServerLayoutInfo"));
    m_infoLabel->setText(tr("最大化：%1    宽度：%2    高度：%3")
                             .arg(m_config.maximized ? tr("是") : tr("否"))
                             .arg(m_config.width)
                             .arg(m_config.height));
    outer->addWidget(m_infoLabel);

    auto *content = new QHBoxLayout;
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(12);

    m_serverList = new QListWidget(group);
    m_serverList->setObjectName(QStringLiteral("SavedServerList"));
    m_serverList->setMinimumWidth(280);
    m_serverList->setMinimumHeight(260);
    m_serverList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serverList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *paramsGroup = new QGroupBox(tr("参数信息"), group);
    paramsGroup->setObjectName(QStringLiteral("ServerParametersGroup"));
    paramsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *paramsLayout = new QFormLayout(paramsGroup);
    paramsLayout->setContentsMargins(12, 16, 12, 12);
    paramsLayout->setHorizontalSpacing(12);
    paramsLayout->setVerticalSpacing(10);
    paramsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

    m_serverValue = new QLabel(QStringLiteral("—"), paramsGroup);
    m_userValue = new QLabel(QStringLiteral("—"), paramsGroup);
    m_passwordValue = new QLabel(QStringLiteral("—"), paramsGroup);
    m_portValue = new QLabel(QStringLiteral("—"), paramsGroup);
    for (QLabel *label : {m_serverValue, m_userValue, m_passwordValue, m_portValue}) {
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setWordWrap(true);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    paramsLayout->addRow(tr("服务器："), m_serverValue);
    paramsLayout->addRow(tr("账号："), m_userValue);
    paramsLayout->addRow(tr("密码："), m_passwordValue);
    paramsLayout->addRow(tr("端口："), m_portValue);

    content->addWidget(m_serverList, 1);
    content->addWidget(paramsGroup, 1);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->addStretch(1);
    m_savedConnectButton = new QPushButton(tr("连接"), group);
    m_savedConnectButton->setObjectName(QStringLiteral("SavedConnectButton"));
    m_savedConnectButton->setAutoDefault(false);
    m_savedConnectButton->setMinimumWidth(110);
    buttonRow->addWidget(m_savedConnectButton);

    outer->addLayout(content, 1);
    outer->addLayout(buttonRow);

    for (const ServerInfo &server : m_config.servers) {
        m_serverList->addItem(server.serverName);
    }

    if (m_serverList->count() > 0) {
        m_serverList->setCurrentRow(0);
    } else {
        m_savedConnectButton->setEnabled(false);
        m_infoLabel->setText(m_infoLabel->text()
                             + tr("    （server.json 中 ServerList 为空）"));
    }

    connect(m_serverList, &QListWidget::currentRowChanged, this,
            &HomePage::updateSelectedServer);
    connect(m_serverList, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *) { connectSelected(); });
    connect(m_savedConnectButton, &QPushButton::clicked, this, &HomePage::connectSelected);

    updateSelectedServer();
    return group;
}

void HomePage::connectManual()
{
    const QString server = m_serverEdit->text().trimmed();
    const QString user = m_userEdit->text().trimmed();
    if (server.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请填写服务器地址。"));
        m_serverEdit->setFocus();
        return;
    }
    if (user.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请填写登录账号。"));
        m_userEdit->setFocus();
        return;
    }

    ServerInfo info;
    info.serverName = server;
    info.server = server;
    info.userName = user;
    info.password = m_passwordEdit->text();
    info.port = m_portSpin->value();

    emit connectRequested(info, info.serverName);
}

void HomePage::connectSelected()
{
    const int row = m_serverList->currentRow();
    if (row < 0 || row >= m_config.servers.size()) {
        return;
    }

    const ServerInfo info = m_config.servers.at(row);
    emit connectRequested(info, info.serverName);
}

void HomePage::updateSelectedServer()
{
    const int row = m_serverList->currentRow();
    const bool valid = row >= 0 && row < m_config.servers.size();
    m_savedConnectButton->setEnabled(valid);

    if (!valid) {
        for (QLabel *label : {m_serverValue, m_userValue, m_passwordValue, m_portValue}) {
            label->setText(QStringLiteral("—"));
        }
        return;
    }

    const ServerInfo &server = m_config.servers.at(row);
    m_serverValue->setText(server.server);
    m_userValue->setText(server.userName);
    m_passwordValue->setText(server.password.isEmpty() ? tr("（空）") : server.password);
    m_portValue->setText(QString::number(server.port));
}
