#pragma once

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QString>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QGuiApplication>
#include <QStyleHints>
#endif

namespace AppIcons {

///
/// \brief isDarkTheme
/// \return
///
inline bool isDarkTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return false;
#endif
}

///
/// \brief themed
/// \param icon
/// \return
///
inline QIcon themed(const QString &icon)
{
    return QIcon(QString(":/icons/%1/%2").arg(isDarkTheme() ? "dark" : "light", icon));
}

///
/// \brief bindIcon
/// \param action
/// \param icon
///
inline void bindIcon(QAction *action, const QString &icon)
{
    auto refresh = [action, icon] {
        action->setIcon(themed(icon));
    };
    refresh();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                     action, [refresh](Qt::ColorScheme) { refresh(); });
#endif
}

}
