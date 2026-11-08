#include "titlebar.h"

#include "captionbutton.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QStyle>

TitleTabBar::TitleTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setDocumentMode(true);
    setDrawBase(false);
    setExpanding(false);
    setMovable(false);
    setTabsClosable(false);
    setUsesScrollButtons(true);
    setElideMode(Qt::ElideRight);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setFixedHeight(32);
    setContextMenuPolicy(Qt::PreventContextMenu);

    connect(this, &QTabBar::currentChanged, this, [this](int) {
        refreshSelectionAppearance();
    });
}

void TitleTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        if (tabAt(event->position().toPoint()) >= 0) {
            event->accept();
            return;
        }
    }
    QTabBar::mousePressEvent(event);
}

void TitleTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        const int index = tabAt(event->position().toPoint());
        if (index >= 0) {
            emit middleClicked(index);
            event->accept();
            return;
        }
    }
    QTabBar::mouseReleaseEvent(event);
}

void TitleTabBar::paintEvent(QPaintEvent *event)
{
    QTabBar::paintEvent(event);

    const int index = currentIndex();
    if (index < 0) {
        return;
    }

    const QRect tab = tabRect(index);
    if (!tab.isValid() || tab.width() < 24) {
        return;
    }

    QColor accent = palette().color(QPalette::Accent);
    if (!accent.isValid()) {
        accent = palette().color(QPalette::Highlight);
    }
    if (!accent.isValid()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);

    const QRectF marker(tab.left() + 6.0, tab.bottom() - 2.5,
                        tab.width() - 12.0, 2.5);
    painter.drawRoundedRect(marker, 1.25, 1.25);
}

void TitleTabBar::changeEvent(QEvent *event)
{
    QTabBar::changeEvent(event);
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::StyleChange) {
        refreshSelectionAppearance();
    }
}

void TitleTabBar::tabInserted(int index)
{
    QTabBar::tabInserted(index);
    refreshSelectionAppearance();
}

void TitleTabBar::tabRemoved(int index)
{
    QTabBar::tabRemoved(index);
    refreshSelectionAppearance();
}

QSize TitleTabBar::tabSizeHint(int index) const
{
    QSize size = QTabBar::tabSizeHint(index);
    size.setHeight(height());
    return size;
}

void TitleTabBar::refreshSelectionAppearance()
{
    refreshTabTextColors();
    update();
}

void TitleTabBar::refreshTabTextColors()
{
    QColor accent = palette().color(QPalette::Accent);
    if (!accent.isValid()) {
        accent = palette().color(QPalette::Highlight);
    }

    const int selected = currentIndex();
    for (int i = 0; i < count(); ++i) {
        setTabTextColor(i, i == selected ? accent : QColor());
    }
}

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("TitleBar"));
    setFixedHeight(32);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(0);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName(QStringLiteral("TitleIcon"));
    m_iconLabel->setFixedSize(28, 32);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    const QIcon icon = window() ? window()->windowIcon() : QIcon();
    const QIcon displayIcon = icon.isNull()
        ? QApplication::style()->standardIcon(QStyle::SP_ComputerIcon)
        : icon;
    m_iconLabel->setPixmap(displayIcon.pixmap(16, 16));
    m_iconLabel->setToolTip(QStringLiteral("JfzQtRDP"));

    m_tabBar = new TitleTabBar(this);
    m_tabBar->setObjectName(QStringLiteral("TitleTabBar"));

    m_minimizeButton = new CaptionButton(CaptionButton::Glyph::Minimize, this);
    m_minimizeButton->setObjectName(QStringLiteral("MinimizeButton"));
    m_maximizeButton = new CaptionButton(CaptionButton::Glyph::Maximize, this);
    m_maximizeButton->setObjectName(QStringLiteral("MaximizeButton"));
    m_closeButton = new CaptionButton(CaptionButton::Glyph::Close, this);
    m_closeButton->setObjectName(QStringLiteral("CloseButton"));

    layout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);
    layout->addWidget(m_tabBar, 0, Qt::AlignVCenter);
    layout->addStretch(1);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);
}

TitleTabBar *TitleBar::tabBar() const
{
    return m_tabBar;
}

QLabel *TitleBar::iconLabel() const
{
    return m_iconLabel;
}

CaptionButton *TitleBar::minimizeButton() const
{
    return m_minimizeButton;
}

CaptionButton *TitleBar::maximizeButton() const
{
    return m_maximizeButton;
}

CaptionButton *TitleBar::closeButton() const
{
    return m_closeButton;
}

void TitleBar::setMaximizeEnabled(bool enabled)
{
    m_maximizeButton->setEnabled(enabled);
    m_maximizeButton->setToolTip(enabled ? tr("最大化") : tr("窗口大小已锁定"));
}

void TitleBar::setMaximized(bool maximized)
{
    m_maximizeButton->setGlyph(maximized ? CaptionButton::Glyph::Restore
                                         : CaptionButton::Glyph::Maximize);
}
