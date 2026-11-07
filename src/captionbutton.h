#pragma once

#include <QAbstractButton>
#include <QColor>

class QEnterEvent;
class QEvent;
class QPainter;
class QPaintEvent;
class QRectF;

class CaptionButton : public QAbstractButton
{
    Q_OBJECT
public:
    enum class Glyph {
        Minimize,
        Maximize,
        Restore,
        Close,
    };
    Q_ENUM(Glyph)

    explicit CaptionButton(Glyph glyph, QWidget *parent = nullptr);

    Glyph glyph() const;
    void setGlyph(Glyph glyph);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    bool isDarkPalette() const;
    void drawFallbackGlyph(QPainter &painter, const QRectF &area, const QColor &color) const;

    Glyph m_glyph;
};
