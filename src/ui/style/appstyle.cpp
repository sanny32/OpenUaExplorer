// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QCoreApplication>
#include <QDebug>
#include <QPainter>
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
/// \brief Returns the neutral hover fill used by unstriped Windows 11 item views.
/// \param palette Palette the view is drawn with.
/// \return Subtle neutral fill for the current appearance.
///
QColor neutralHoverColor(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightness() < 128 ? QColor(0xff, 0xff, 0xff, 15)
                                                             : QColor(0x00, 0x00, 0x00, 10);
}

///
/// \brief Copies a selected item option and resolves its system selection brush.
/// \param option Selected item option to copy.
/// \param selected Receives the adjusted option.
/// \return True when the option represents a selected view item.
///
bool systemSelectionOption(const QStyleOption *option, QStyleOptionViewItem *selected)
{
    const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option);
    if (!item || !item->state.testFlag(QStyle::State_Selected))
        return false;

    *selected = *item;
    QPalette::ColorGroup group = QPalette::Inactive;
    if (!item->state.testFlag(QStyle::State_Enabled))
        group = QPalette::Disabled;
    else if (item->state.testFlag(QStyle::State_Active))
        group = QPalette::Active;
    selected->backgroundBrush = item->palette.brush(group, QPalette::Highlight);
    return true;
}

///
/// \brief Replaces a hovered item background with the neutral hover fill.
/// \param option Hovered item option to copy.
/// \param hovered Receives the adjusted option.
/// \return True when the option represents a hovered, unselected view item.
///
bool systemHoverOption(const QStyleOption *option, QStyleOptionViewItem *hovered)
{
    const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option);
    if (!item || item->state.testFlag(QStyle::State_Selected)
        || !item->state.testFlag(QStyle::State_MouseOver)) {
        return false;
    }

    *hovered = *item;
    hovered->backgroundBrush = neutralHoverColor(item->palette);
    hovered->state.setFlag(QStyle::State_MouseOver, false);
    return true;
}

///
/// \brief Reports whether a row primitive covers the branch strip of a tree row.
/// \param option Row option being painted.
/// \return True when the row primitive paints the indentation holding the disclosure chevron.
///
/// A tree paints the row background under every cell and then lets the delegate paint the cell
/// on top of it, so a translucent hover fill applied in both places lands twice. The branch
/// strip is the one part of the row no delegate covers, so it is the only place where the row
/// primitive still has to lay the hover fill down itself.
///
bool paintsBranchStrip(const QStyleOptionViewItem &option)
{
    return option.features.testFlag(QStyleOptionViewItem::IsDecorationForRootColumn);
}

///
/// \brief Reports whether the current paint target is an item view or its viewport.
/// \param widget Widget supplied by the style caller, if any.
/// \param painter Painter whose device supplies the missing widget context.
/// \return True when the primitive belongs to an item view.
///
bool paintsItemView(const QWidget *widget, const QPainter *painter)
{
    const QWidget *paintWidget = widget;
    if (!paintWidget && painter && painter->device()
        && painter->device()->devType() == QInternal::Widget) {
        paintWidget = static_cast<const QWidget *>(painter->device());
    }

    if (qobject_cast<const QAbstractItemView *>(paintWidget))
        return true;

    const auto *view = paintWidget
        ? qobject_cast<const QAbstractItemView *>(paintWidget->parentWidget())
        : nullptr;
    return view && view->viewport() == paintWidget;
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
/// \brief Reports whether the native style needs item selection and hover normalised.
/// \param widget Widget being painted.
/// \return True for item views using the Windows 11 base style.
///
bool AppStyle::usesNativeItemStateOverride(const QWidget *widget) const
{
    if (!qobject_cast<const QAbstractItemView *>(widget))
        return false;

    const QStyle *base = QProxyStyle::baseStyle();
    while (const auto *proxy = qobject_cast<const QProxyStyle *>(base))
        base = proxy->baseStyle();
    return base && base->name().compare(QLatin1String("windows11"), Qt::CaseInsensitive) == 0;
}

///
/// \brief Draws item-view controls with system selection and neutral hover fills.
/// \param element Control element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void AppStyle::drawControl(ControlElement element, const QStyleOption *option,
                           QPainter *painter, const QWidget *widget) const
{
    QStyleOptionViewItem focusless;
    const QStyleOption *drawOption = option;
    if (element == CE_ItemViewItem && qobject_cast<const QAbstractItemView *>(widget)) {
        if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            focusless = *item;
            focusless.state.setFlag(QStyle::State_HasFocus, false);
            drawOption = &focusless;
        }
    }

    if (element == CE_ItemViewItem && usesNativeItemStateOverride(widget)) {
        QStyleOptionViewItem adjusted;
        if (systemSelectionOption(drawOption, &adjusted)) {
            QCommonStyle::drawControl(element, &adjusted, painter, widget);
            return;
        }
        if (systemHoverOption(drawOption, &adjusted)) {
            QCommonStyle::drawControl(element, &adjusted, painter, widget);
            return;
        }
    }
    QProxyStyle::drawControl(element, drawOption, painter, widget);
}

///
/// \brief Draws item-view rows with system fills, one hover coat, and no cell focus frames.
/// \param element Primitive element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void AppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                             QPainter *painter, const QWidget *widget) const
{
    if (element == PE_FrameFocusRect && paintsItemView(widget, painter)) {
        return;
    }

    if ((element == PE_PanelItemViewItem || element == PE_PanelItemViewRow)
        && usesNativeItemStateOverride(widget)) {
        QStyleOptionViewItem adjusted;
        if (systemSelectionOption(option, &adjusted)) {
            painter->fillRect(adjusted.rect, adjusted.backgroundBrush);
            return;
        }
        if (systemHoverOption(option, &adjusted)) {
            if (element == PE_PanelItemViewItem) {
                painter->fillRect(adjusted.rect, adjusted.backgroundBrush);
                return;
            }
            const QBrush hover = adjusted.backgroundBrush;
            adjusted.backgroundBrush = QBrush();
            QProxyStyle::drawPrimitive(element, &adjusted, painter, widget);
            if (paintsBranchStrip(adjusted))
                painter->fillRect(adjusted.rect, hover);
            return;
        }
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
