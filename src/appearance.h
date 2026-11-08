#pragma once

#include <QObject>
#include <QPalette>
#include <QString>
#include <QStringList>

enum class ThemeMode {
    System = 0,
    Light = 1,
    Dark = 2,
};

class AppearanceController : public QObject
{
    Q_OBJECT
public:
    static AppearanceController *instance();

    static QStringList supportedStyles();
    static QString defaultStyle();

    QString selectedStyle() const;
    ThemeMode themeMode() const;
    QString effectiveStyle() const;
    bool isDark() const;
    bool isStyleFallback() const;

    bool setStyle(const QString &styleName);
    void setThemeMode(ThemeMode mode);

signals:
    void stateChanged();

private:
    explicit AppearanceController(QObject *parent = nullptr);

    void apply();
    static QPalette createDarkPalette();

    QString m_selectedStyle;
    ThemeMode m_themeMode = ThemeMode::System;
    QString m_effectiveStyle;
    bool m_dark = false;
    bool m_applying = false;
};
