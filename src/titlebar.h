#pragma once

#include <QTabBar>
#include <QWidget>

class CaptionButton;
class QEvent;
class QLabel;
class QMouseEvent;
class QPaintEvent;

class TitleTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit TitleTabBar(QWidget *parent = nullptr);
    void refreshSelectionAppearance();

signals:
    void middleClicked(int index);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;
    void tabInserted(int index) override;
    void tabRemoved(int index) override;
    QSize tabSizeHint(int index) const override;

private:
    void refreshTabTextColors();
};

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    TitleTabBar *tabBar() const;
    QLabel *iconLabel() const;
    CaptionButton *minimizeButton() const;
    CaptionButton *maximizeButton() const;
    CaptionButton *closeButton() const;

    void setMaximizeEnabled(bool enabled);
    void setMaximized(bool maximized);

private:
    TitleTabBar *m_tabBar = nullptr;
    QLabel *m_iconLabel = nullptr;
    CaptionButton *m_minimizeButton = nullptr;
    CaptionButton *m_maximizeButton = nullptr;
    CaptionButton *m_closeButton = nullptr;
};
