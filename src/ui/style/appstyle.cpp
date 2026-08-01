// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QAbstractItemView>
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

namespace {

///
/// \brief Returns the muted selection fill the Windows 11 style uses for unstriped views.
/// \param palette Palette the view is drawn with.
/// \return Translucent fill matching the style's fillSubtleSecondary colour.
///
QColor subtleHighlight(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightness() < 128 ? QColor(0xff, 0xff, 0xff, 15)
                                                             : QColor(0x00, 0x00, 0x00, 10);
}

///
/// \brief Copies a view-item option with its selection fill muted.
/// \param option Option to copy; must be a QStyleOptionViewItem.
/// \param muted Receives the copy when the cast succeeds.
/// \return True when the caller should paint with the muted copy.
///
bool muteSelection(const QStyleOption *option, QStyleOptionViewItem *muted)
{
    const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option);
    if (!item)
        return false;
    *muted = *item;
    muted->palette.setColor(QPalette::Highlight, subtleHighlight(muted->palette));
    return true;
}

} // namespace

///
/// \brief Constructs the proxy style around the current application style.
/// \param parent Parent object.
///
AppStyle::AppStyle(QObject *parent)
    : AppStyle(QString())
{
    setParent(parent);
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
    return base && base->name().compare(QLatin1String("fusion"), Qt::CaseInsensitive) == 0;
}

///
/// \brief Reports whether this style must mute a view's selection fill.
/// \param widget Widget being painted.
/// \return True for a striped item view proxied over the Windows 11 style.
///
/// qwindows11style picks palette.highlight() over its muted fill purely by
/// alternatingRowColors(), so striped views alone jump to the OS accent colour.
///
bool AppStyle::mutesSelection(const QWidget *widget) const
{
    const auto *view = qobject_cast<const QAbstractItemView *>(widget);
    if (!view || !view->alternatingRowColors())
        return false;

    const QStyle *base = QProxyStyle::baseStyle();
    while (const auto *proxy = qobject_cast<const QProxyStyle *>(base))
        base = proxy->baseStyle();
    return base && base->name().compare(QLatin1String("windows11"), Qt::CaseInsensitive) == 0;
}

///
/// \brief Draws a control element, muting the selection fill of striped item views.
/// \param element Control element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void AppStyle::drawControl(ControlElement element, const QStyleOption *option,
                           QPainter *painter, const QWidget *widget) const
{
    QStyleOptionViewItem muted;
    if (element == CE_ItemViewItem && mutesSelection(widget)
        && muteSelection(option, &muted)) {
        QProxyStyle::drawControl(element, &muted, painter, widget);
        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

///
/// \brief Draws a primitive element, muting the selection fill of striped tree views.
/// \param element Primitive element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void AppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                             QPainter *painter, const QWidget *widget) const
{
    QStyleOptionViewItem muted;
    if (element == PE_PanelItemViewRow && mutesSelection(widget)
        && muteSelection(option, &muted)) {
        QProxyStyle::drawPrimitive(element, &muted, painter, widget);
        return;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
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
        if (const auto *button = qobject_cast<const ThemedToolButton *>(widget)) {
            if (!button->linkStyle()) {
                size.setHeight(qMax(size.height(), controlMinHeight));
            }
        }
        break;
    default:
        break;
    }

    return size;
}
