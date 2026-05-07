// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionButton>

#include "appstyle.h"
#include "loggingcategories.h"
#include "widgets/themedpushbutton.h"
#include "widgets/themedtoolbutton.h"

///
/// \brief createBaseStyle
/// \param baseStyleName Base style name to create.
/// \return Created style for the name, or the current application style.
///
static QStyle *createBaseStyle(const QString &baseStyleName)
{
    if (baseStyleName.isEmpty())
        return QApplication::style();

    QStyle *style = QStyleFactory::create(baseStyleName);
    if (style) {
        return style;
    }

    qCWarning(lcApp).noquote()
        << QCoreApplication::translate(
               "AppStyle",
               "Failed to create base style \"%1\". Falling back to current application style.")
               .arg(baseStyleName);

    return QApplication::style();
}

///
/// \brief AppStyle::AppStyle
/// \param baseStyleName Base style name to use, or an empty string to wrap the current style.
///
AppStyle::AppStyle(const QString &baseStyleName)
    : QProxyStyle(createBaseStyle(baseStyleName))
{
}

///
/// \brief AppStyle::baseStyle
/// \return The innermost non-proxy base style of the application style stack.
///
const QStyle *AppStyle::baseStyle()
{
    const QStyle *base = QApplication::style();
    while (const auto *proxy = qobject_cast<const QProxyStyle *>(base))
        base = proxy->baseStyle();
    return base;
}

///
/// \brief AppStyle::isFusionStyle
/// \return True if the application base style is Fusion.
///
bool AppStyle::isFusionStyle()
{
    const QStyle *base = baseStyle();
    return base && base->name().compare(QLatin1String("fusion"), Qt::CaseInsensitive) == 0;
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
