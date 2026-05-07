// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appstyle.cpp
/// \brief Implements the application proxy style.
///

#include <QStyleOptionButton>
#ifdef HAS_QTDBUS
#include <QDBusConnection>
#include <QDBusVariant>
#include <QGuiApplication>
#include <QStyleHints>
#endif

#include "appstyle.h"
#include "widgets/themedpushbutton.h"
#include "widgets/themedtoolbutton.h"

AppStyle::AppStyle(QStyle *style)
    : QProxyStyle(style)
{
#ifdef HAS_QTDBUS
    QDBusConnection::sessionBus().connect(
        QString(),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("SettingChanged"),
        this,
        SLOT(onPortalSettingChanged(QString, QString, QDBusVariant)));
#endif
}

#ifdef HAS_QTDBUS
void AppStyle::onPortalSettingChanged(const QString &group, const QString &key,
                                      const QDBusVariant &value)
{
    if (group != QLatin1String("org.freedesktop.appearance")
        || key != QLatin1String("color-scheme")) {
        return;
    }

    // 1 = dark, 2 = light, 0 = no preference
    const uint scheme = value.variant().toUInt();
    const Qt::ColorScheme colorScheme = (scheme == 1) ? Qt::ColorScheme::Dark
                                                      : Qt::ColorScheme::Light;
    QGuiApplication::styleHints()->setColorScheme(colorScheme);
}
#endif

///
/// \brief AppStyle::subElementRect
/// \param element
/// \param option
/// \param widget
/// \return
///
QRect AppStyle::subElementRect(SubElement element, const QStyleOption *option,
                                const QWidget *widget) const
{
    QRect rect = QProxyStyle::subElementRect(element, option, widget);

    switch (element) {
    case SE_HeaderLabel:
        return rect.adjusted(textHMargin, textVMargin, -textHMargin, -textVMargin);
    case SE_ItemViewItemText:
        return rect.adjusted(textHMargin, 0, -textHMargin, 0);
    default:
        return rect;
    }
}

///
/// \brief AppStyle::sizeFromContents
/// \param type
/// \param option
/// \param contentsSize
/// \param widget
/// \return
///
QSize AppStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                 const QSize &contentsSize,
                                 const QWidget *widget) const
{
    QSize size = QProxyStyle::sizeFromContents(type, option, contentsSize, widget);

    switch (type) {
    case CT_ComboBox:
    case CT_LineEdit:
    case CT_SpinBox:
    case CT_TabBarTab:
        size.setHeight(qMax(size.height(), controlMinHeight));
        break;
    case CT_PushButton:
        if (qobject_cast<const ThemedPushButton *>(widget) != nullptr) {
            size.setHeight(qMax(size.height(), controlMinHeight));
        }
        break;
    case CT_ToolButton:
        if (qobject_cast<const ThemedToolButton *>(widget) != nullptr) {
            size.setHeight(qMax(size.height(), controlMinHeight));
        }
        break;
    default:
        break;
    }

    return size;
}
