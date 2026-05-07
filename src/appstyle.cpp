// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appstyle.cpp
/// \brief Implements the application proxy style.
///

#include <QStyleOptionButton>
#ifdef HAS_QTDBUS
#include <QApplication>
#include <QDBusConnection>
#include <QDBusVariant>
#include <QDebug>
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
    qDebug() << "SettingChanged:" << group << key << value.variant();

    if (group != QLatin1String("org.freedesktop.appearance")
        || key != QLatin1String("color-scheme")) {
        return;
    }

    // 1 = dark, 2 = light, 0 = no preference
    const uint scheme = value.variant().toUInt();
    qDebug() << "color-scheme value:" << scheme;

    AppStyle::applyColorScheme(scheme == 1);
}
#endif

static QPalette fusionPalette(bool darkAppearance)
{
    const QColor windowText = darkAppearance ? QColor(240, 240, 240) : Qt::black;
    const QColor backGround = darkAppearance ? QColor(50, 50, 50) : QColor(239, 239, 239);
    const QColor light = backGround.lighter(150);
    const QColor mid = backGround.darker(130);
    const QColor midLight = mid.lighter(110);
    const QColor base = darkAppearance ? backGround.darker(140) : Qt::white;
    const QColor disabledBase(backGround);
    const QColor dark = backGround.darker(150);
    const QColor darkDisabled = QColor(209, 209, 209).darker(110);
    const QColor text = darkAppearance ? windowText : Qt::black;
    const QColor highlight = QColor(48, 140, 198);
    const QColor highlightedText = darkAppearance ? windowText : Qt::white;
    const QColor disabledText = darkAppearance ? QColor(130, 130, 130) : QColor(190, 190, 190);
    const QColor button = backGround;
    const QColor shadow = dark.darker(135);
    const QColor disabledShadow = shadow.lighter(150);
    const QColor disabledHighlight(145, 145, 145);
    QColor placeholder = text;
    placeholder.setAlpha(128);

    QPalette p(windowText, backGround, light, dark, mid, text, base);
    p.setBrush(QPalette::Midlight,        midLight);
    p.setBrush(QPalette::Button,          button);
    p.setBrush(QPalette::Shadow,          shadow);
    p.setBrush(QPalette::HighlightedText, highlightedText);
    p.setBrush(QPalette::Disabled, QPalette::Text,       disabledText);
    p.setBrush(QPalette::Disabled, QPalette::WindowText, disabledText);
    p.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledText);
    p.setBrush(QPalette::Disabled, QPalette::Base,       disabledBase);
    p.setBrush(QPalette::Disabled, QPalette::Dark,       darkDisabled);
    p.setBrush(QPalette::Disabled, QPalette::Shadow,     disabledShadow);
    p.setBrush(QPalette::Active,   QPalette::Highlight,  highlight);
    p.setBrush(QPalette::Inactive, QPalette::Highlight,  highlight);
    p.setBrush(QPalette::Disabled, QPalette::Highlight,  disabledHighlight);
    p.setBrush(QPalette::Active,   QPalette::Accent,     highlight);
    p.setBrush(QPalette::Inactive, QPalette::Accent,     highlight);
    p.setBrush(QPalette::Disabled, QPalette::Accent,     disabledHighlight);
    p.setBrush(QPalette::PlaceholderText, placeholder);
    if (darkAppearance)
        p.setBrush(QPalette::Link, highlight);
    return p;
}

void AppStyle::applyColorScheme(bool dark)
{
    QApplication::setPalette(fusionPalette(dark));
}

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
