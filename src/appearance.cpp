#include "appearance.h"

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QGuiApplication>
#include <QOperatingSystemVersion>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

namespace {

QString normalizedStyleName(const QString &name)
{
    return name.trimmed().toLower();
}

QString themeName(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::System:
        return QStringLiteral("system");
    case ThemeMode::Light:
        return QStringLiteral("light");
    case ThemeMode::Dark:
        return QStringLiteral("dark");
    }
    return QStringLiteral("unknown");
}

} // namespace

AppearanceController *AppearanceController::instance()
{
    static AppearanceController *controller = new AppearanceController(qApp);
    return controller;
}

QStringList AppearanceController::supportedStyles()
{
    static const QStringList requested = {
        QStringLiteral("windows11"),
        QStringLiteral("windowsvista"),
        QStringLiteral("windows"),
        QStringLiteral("fusion"),
    };
    static const QStringList available = [] {
        const QStringList keys = QStyleFactory::keys();
        QStringList result;
        for (const QString &name : requested) {
            for (const QString &key : keys) {
                if (key.compare(name, Qt::CaseInsensitive) == 0) {
                    result.append(name);
                    break;
                }
            }
        }
        if (result.isEmpty()) {
            result.append(QStringLiteral("fusion"));
        }
        return result;
    }();
    return available;
}

QString AppearanceController::defaultStyle()
{
    const QStringList styles = supportedStyles();
#ifdef Q_OS_WIN
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11
        && styles.contains(QStringLiteral("windows11"))) {
        return QStringLiteral("windows11");
    }
#endif
    return styles.contains(QStringLiteral("windowsvista"))
        ? QStringLiteral("windowsvista")
        : styles.first();
}

AppearanceController::AppearanceController(QObject *parent)
    : QObject(parent)
    , m_selectedStyle(defaultStyle())
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            [this](Qt::ColorScheme) {
                if (!m_applying) {
                    apply();
                }
            });

    apply();
}

QString AppearanceController::selectedStyle() const
{
    return m_selectedStyle;
}

ThemeMode AppearanceController::themeMode() const
{
    return m_themeMode;
}

QString AppearanceController::effectiveStyle() const
{
    return m_effectiveStyle;
}

bool AppearanceController::isDark() const
{
    return m_dark;
}

bool AppearanceController::isStyleFallback() const
{
    return m_dark && m_selectedStyle == QStringLiteral("windowsvista");
}

bool AppearanceController::setStyle(const QString &styleName)
{
    const QString normalized = normalizedStyleName(styleName);
    if (!supportedStyles().contains(normalized)) {
        return false;
    }
    if (m_selectedStyle == normalized) {
        return true;
    }

    m_selectedStyle = normalized;
    apply();
    return true;
}

void AppearanceController::setThemeMode(ThemeMode mode)
{
    if (m_themeMode == mode) {
        return;
    }
    m_themeMode = mode;
    apply();
}

void AppearanceController::apply()
{
    if (m_applying) {
        return;
    }
    m_applying = true;

    QStyleHints *hints = QGuiApplication::styleHints();
    switch (m_themeMode) {
    case ThemeMode::System:
        hints->unsetColorScheme();
        break;
    case ThemeMode::Light:
        hints->setColorScheme(Qt::ColorScheme::Light);
        break;
    case ThemeMode::Dark:
        hints->setColorScheme(Qt::ColorScheme::Dark);
        break;
    }

    const bool dark = m_themeMode == ThemeMode::Dark
        || (m_themeMode == ThemeMode::System
            && hints->colorScheme() == Qt::ColorScheme::Dark);

    QString effective = m_selectedStyle;
    if (dark && effective == QStringLiteral("windowsvista")) {
        // WindowsVista uses native themed controls that do not support a dark
        // palette. Use Fusion for a readable dark UI in this combination.
        effective = QStringLiteral("fusion");
    }

    const QString currentStyle = QApplication::style()
        ? QApplication::style()->objectName().toLower()
        : QString();
    if (currentStyle != effective) {
        if (QStyle *style = QStyleFactory::create(effective)) {
            QApplication::setStyle(style);
        } else {
            qWarning().noquote() << "Appearance: cannot create style" << effective;
            effective = currentStyle;
        }
    }

    QPalette palette;
    if (dark) {
        palette = createDarkPalette();
    } else {
        palette = QApplication::style()->standardPalette();
        if (effective == QStringLiteral("windows11")
            || effective == QStringLiteral("windowsvista")
            || effective == QStringLiteral("windows")) {
            // Keep the native Qt Windows styles' light surface consistent with
            // the Windows 11 window background.
            palette.setColor(QPalette::Window, QColor(0xF3, 0xF3, 0xF3));
        }
        palette.setColor(QPalette::Accent, palette.color(QPalette::Highlight));
    }
    QApplication::setPalette(palette);

    m_effectiveStyle = QApplication::style()->objectName().toLower();
    m_dark = dark;
    m_applying = false;

    qInfo().noquote() << "Appearance: selected style" << m_selectedStyle
                      << "| effective" << m_effectiveStyle
                      << "| theme" << themeName(m_themeMode)
                      << "| dark" << m_dark;

    emit stateChanged();
}

QPalette AppearanceController::createDarkPalette()
{
    QPalette palette;
    const QColor window(30, 30, 30);
    const QColor base(45, 45, 45);
    const QColor button(60, 60, 60);
    const QColor text(240, 240, 240);
    const QColor disabled(127, 127, 127);
    const QColor accent(0, 120, 215);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::PlaceholderText, QColor(160, 160, 160));
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, QColor(255, 90, 90));
    palette.setColor(QPalette::Link, QColor(100, 160, 255));
    palette.setColor(QPalette::LinkVisited, QColor(180, 120, 255));
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Accent, accent);

    palette.setColor(QPalette::Light, QColor(82, 82, 82));
    palette.setColor(QPalette::Midlight, QColor(70, 70, 70));
    palette.setColor(QPalette::Mid, QColor(45, 45, 45));
    palette.setColor(QPalette::Dark, QColor(20, 20, 20));
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0));

    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(36, 36, 36));

    return palette;
}
