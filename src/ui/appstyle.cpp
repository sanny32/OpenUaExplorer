// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QPalette>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionButton>
#include <QStyleOptionMenuItem>
#include <QStyleOptionViewItem>
#include <QWidget>

#include "appstyle.h"
#include "loggingcategories.h"
#include "widgets/coloredpushbutton.h"
#include "widgets/themedpushbutton.h"
#include "widgets/themedtoolbutton.h"

///
/// \brief Creates the base style to proxy, falling back to the application style.
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
/// \brief Constructs the proxy style around the named base style.
/// \param baseStyleName Base style name to use, or an empty string to wrap the current style.
///
AppStyle::AppStyle(const QString &baseStyleName)
    : QProxyStyle(createBaseStyle(baseStyleName))
{
}

///
/// \brief Resolves the innermost concrete base style.
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
/// \brief Reports whether the active base style is the Fusion style.
/// \return True if the application base style is Fusion.
///
bool AppStyle::isFusionStyle()
{
    const QStyle *base = baseStyle();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return base && base->name().compare(QLatin1String("fusion"), Qt::CaseInsensitive) == 0;
#else
    return base && QString::fromLatin1(base->metaObject()->className())
        .contains(QLatin1String("Fusion"), Qt::CaseInsensitive);
#endif
}

///
/// \brief Draws a control element, forcing highlighted text colour on the attributes tree.
/// \param element Control element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void AppStyle::drawControl(ControlElement element, const QStyleOption *option,
                           QPainter *painter, const QWidget *widget) const
{
    if (element == CE_ItemViewItem && widget
        && widget->objectName() == QLatin1String("attributesTree")
        && (option->state.testFlag(State_Selected)
            || option->state.testFlag(State_MouseOver))) {
        if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            QStyleOptionViewItem opt(*item);
            opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::HighlightedText));
            QProxyStyle::drawControl(element, &opt, painter, widget);
            return;
        }
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

///
/// \brief Returns the geometry of a sub-element, adding padding to headers and item text.
/// \param element Sub-element to locate.
/// \param option Style option carrying the element geometry.
/// \param widget Widget the element belongs to.
/// \return Adjusted sub-element rectangle.
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
/// \brief Enforces minimum heights and widths for selected control types.
/// \param type Contents type being measured.
/// \param option Style option carrying the element state.
/// \param contentsSize Size requested by the contents.
/// \param widget Widget the element belongs to.
/// \return Size clamped to the style's minimum dimensions.
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
    case CT_MenuItem:
        if (const auto *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menuItem->menuItemType != QStyleOptionMenuItem::Separator) {
                size.setHeight(qMax(size.height(), menuItemMinHeight));
            }
        }
        break;
    case CT_MenuBarItem:
        size.setHeight(qMax(size.height(), menuBarItemMinHeight));
        break;
    case CT_PushButton:
        if (qobject_cast<const ThemedPushButton *>(widget) != nullptr
            || qobject_cast<const ColoredPushButton *>(widget) != nullptr) {
            size.setWidth(qMax(size.width(), pushButtonMinWidth));
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
