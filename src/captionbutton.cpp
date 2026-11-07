#include "captionbutton.h"

#include <QEvent>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QPainter>
#include <QStyleHints>
#include <QToolTip>

namespace {

QString glyphCharacter(CaptionButton::Glyph glyph)
{
    switch (glyph) {
    case CaptionButton::Glyph::Minimize:
        return QString(QChar(0xE921));
    case CaptionButton::Glyph::Maximize:
        return QString(QChar(0xE922));
    case CaptionButton::Glyph::Restore:
        return QString(QChar(0xE923));
    case CaptionButton::Glyph::Close:
        return QString(QChar(0xE8BB));
    }
    return {};
}

bool hasFluentIconFont()
{
    static const bool available = [] {
        const QStringList families = QFontDatabase::families();
        return families.contains(QStringLiteral("Segoe Fluent Icons"))
            || families.contains(QStringLiteral("Segoe MDL2 Assets"));
    }();
    return available;
}

} // namespace

CaptionButton::CaptionButton(Glyph glyph, QWidget *parent)
    : QAbstractButton(parent)
    , m_glyph(glyph)
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::ArrowCursor);
    setAttribute(Qt::WA_Hover, true);
    setToolTip(glyph == Glyph::Close ? tr("关闭")
                                     : glyph == Glyph::Minimize ? tr("最小化")
                                                                : tr("最大化"));
}

CaptionButton::Glyph CaptionButton::glyph() const
{
    return m_glyph;
}

void CaptionButton::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    setToolTip(glyph == Glyph::Close ? tr("关闭")
                                     : glyph == Glyph::Minimize ? tr("最小化")
                                                                : glyph == Glyph::Restore
                                      ? tr("还原")
                                      : tr("最大化"));
    update();
}

QSize CaptionButton::sizeHint() const
{
    return QSize(46, 32);
}

bool CaptionButton::isDarkPalette() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return palette().color(QPalette::Window).lightness() < 128;
#endif
}

void CaptionButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool closeButton = m_glyph == Glyph::Close;
    const bool hovered = underMouse() && isEnabled();
    const bool pressed = isDown();

    if (hovered) {
        QColor background;
        if (closeButton) {
            background = pressed ? QColor(0xB0, 0x2A, 0x1E) : QColor(0xC4, 0x2B, 0x1C);
        } else {
            background = isDarkPalette() ? QColor(255, 255, 255, pressed ? 42 : 24)
                                         : QColor(0, 0, 0, pressed ? 32 : 18);
        }
        painter.fillRect(rect(), background);
    }

    QColor foreground = closeButton && hovered ? QColor(Qt::white)
                                               : palette().color(QPalette::WindowText);
    if (!isEnabled()) {
        foreground.setAlpha(110);
    }

    const QRectF area = QRectF(rect());
    if (hasFluentIconFont()) {
        QFont font;
        const QStringList families = QFontDatabase::families();
        font.setFamily(families.contains(QStringLiteral("Segoe Fluent Icons"))
                           ? QStringLiteral("Segoe Fluent Icons")
                           : QStringLiteral("Segoe MDL2 Assets"));
        font.setPointSizeF(10.0);
        font.setWeight(QFont::Normal);
        painter.setFont(font);
        painter.setPen(foreground);
        painter.drawText(area, Qt::AlignCenter, glyphCharacter(m_glyph));
    } else {
        drawFallbackGlyph(painter, area, foreground);
    }
}

void CaptionButton::drawFallbackGlyph(QPainter &painter, const QRectF &area, const QColor &color) const
{
    QPen pen(color);
    pen.setWidthF(1.0);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const QPointF center = area.center();
    const qreal half = 5.0;

    switch (m_glyph) {
    case Glyph::Minimize:
        painter.drawLine(QPointF(center.x() - half, center.y() + 5.0),
                         QPointF(center.x() + half, center.y() + 5.0));
        break;
    case Glyph::Maximize:
        painter.drawRect(QRectF(center.x() - half, center.y() - half, half * 2.0, half * 2.0));
        break;
    case Glyph::Restore:
        painter.drawRect(QRectF(center.x() - half + 2.0, center.y() - half, half * 2.0 - 1.0,
                                half * 2.0 - 1.0));
        painter.drawRect(QRectF(center.x() - half, center.y() - half + 2.0, half * 2.0 - 1.0,
                                half * 2.0 - 1.0));
        break;
    case Glyph::Close:
        painter.drawLine(QPointF(center.x() - half, center.y() - half),
                         QPointF(center.x() + half, center.y() + half));
        painter.drawLine(QPointF(center.x() + half, center.y() - half),
                         QPointF(center.x() - half, center.y() + half));
        break;
    }
}

void CaptionButton::enterEvent(QEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    update();
}

void CaptionButton::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    update();
}

void CaptionButton::changeEvent(QEvent *event)
{
    QAbstractButton::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::FontChange) {
        update();
    }
}
