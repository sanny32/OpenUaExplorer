#pragma once

#include <QApplication>
#include <QIcon>
#include <QString>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QGuiApplication>
#include <QStyleHints>
#endif

namespace AppIcons {

inline bool isDarkTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
#endif
}

inline QIcon themed(const QString &name, const QString &ext = ".svg")
{
    return QIcon(QString(":/icons/%1/%2%3").arg(isDarkTheme() ? "dark" : "light", name, ext));
}

}
