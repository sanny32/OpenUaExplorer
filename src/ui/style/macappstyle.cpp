// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file macappstyle.cpp
/// \brief Implements the macOS-flavoured application style.
///

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "macappstyle.h"
#include "macthemefactory.h"
#include "macpalette.h"
#include "application.h"
#include "widgets/maintoolbutton.h"
#include "widgets/themedtoolbutton.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QBrush>
#include <QDockWidget>
#include <QFontDatabase>
#include <QHash>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>
#include <QStyleOptionDockWidget>
#include <QStyleOptionTab>
#include <QTabBar>

using namespace MacPalette;

namespace {

using oclero::qlementine::ActiveState;
using oclero::qlementine::CheckState;
using oclero::qlementine::ColorRole;
using oclero::qlementine::FocusState;
using oclero::qlementine::MouseState;
using oclero::qlementine::SelectionState;
using oclero::qlementine::Status;
using oclero::qlementine::Theme;

///
/// \brief Builds a fully transparent colour from an RGB value.
/// \param rgb Source RGB value.
/// \return Colour with alpha set to zero.
///
QColor transparent(QRgb rgb)
{
    QColor color(rgb);
    color.setAlpha(0);
    return color;
}

///
/// \brief Builds a colour from an RGB value with an explicit alpha.
/// \param rgb Source RGB value.
/// \param value Alpha channel, 0–255.
/// \return Colour with the given opacity.
///
QColor alpha(QRgb rgb, int value)
{
    QColor color(rgb);
    color.setAlpha(value);
    return color;
}

///
/// \brief Returns a cached QColor for an RGB value, suitable for by-reference style getters.
/// \param rgb Source RGB value.
/// \return Reference to the cached colour.
///
const QColor& colorRef(QRgb rgb)
{
    static QHash<QRgb, QColor> colors;
    auto it = colors.find(rgb);
    if (it == colors.end())
        it = colors.insert(rgb, QColor(rgb));
    return it.value();
}

///
/// \brief Returns a cached transparent QColor for an RGB value.
/// \param rgb Source RGB value.
/// \return Reference to the cached transparent colour.
///
const QColor& transparentRef(QRgb rgb)
{
    static QHash<QRgb, QColor> colors;
    auto it = colors.find(rgb);
    if (it == colors.end())
        it = colors.insert(rgb, transparent(rgb));
    return it.value();
}

///
/// \brief Extracts an item's background colour from its Qt::BackgroundRole data.
/// \param index Model index to inspect.
/// \return The item's background colour, or an invalid colour when none is set.
///
QColor modelBackgroundColor(const QModelIndex& index)
{
    const QVariant background = index.data(Qt::BackgroundRole);
    if (!background.isValid())
        return {};

    const QBrush brush = qvariant_cast<QBrush>(background);
    if (brush.style() != Qt::NoBrush && brush.color().isValid())
        return brush.color();

    const QColor color = qvariant_cast<QColor>(background);
    if (color.isValid())
        return color;

    return {};
}

} // namespace

///
/// \brief Builds the macOS style with its light/dark themes and tracks color-scheme changes.
/// \param parent Owning QObject.
///
MacAppStyle::MacAppStyle(QObject* parent)
    : QlementineAppStyle(parent)
    , _lightTheme(MacThemeFactory::makeLightTheme())
    , _darkTheme(MacThemeFactory::makeDarkTheme())
{
    updateTheme();

    connect(&theApp()->theme(), &AppTheme::colorSchemeChanged,
            this, [this]() { updateTheme(); });
}

///
/// \brief Switches the active theme to match the current light/dark mode.
///
void MacAppStyle::updateTheme()
{
    setTheme(isDarkMode() ? _darkTheme : _lightTheme);
}

///
/// \brief Reports whether the application theme is currently dark.
/// \return True when the active scheme is dark.
///
bool MacAppStyle::isDarkMode() const
{
    return theApp() && theApp()->theme().isDark();
}

///
/// \brief Custom-draws macOS dock-widget titles; everything else defers to the base style.
/// \param element Control element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void MacAppStyle::drawControl(ControlElement element, const QStyleOption* option,
                               QPainter* painter, const QWidget* widget) const
{
    if (element == CE_DockWidgetTitle) {
        const auto* dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
        if (!dockOption) {
            QlementineAppStyle::drawControl(element, option, painter, widget);
            return;
        }

        const QRect rect = dockOption->rect;
        const QRgb bgColor = isDarkMode() ? Dark::kChrome : Light::kChrome;
        const QRgb borderColor = isDarkMode() ? Dark::kBorder : Light::kBorder;
        const QRgb textColor = isDarkMode() ? Dark::kText : Light::kText;

        painter->save();
        painter->fillRect(rect, colorRef(bgColor));
        painter->setPen(QPen(colorRef(borderColor), 1));
        painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

        QRect textRect = rect.adjusted(6, 0, -42, 0);
        if (textRect.isValid()) {
            QFont font = painter->font();
            font.setPointSize(qMax(11, font.pointSize()));
            painter->setFont(font);
            painter->setPen(colorRef(textColor));
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                              painter->fontMetrics().elidedText(dockOption->title, Qt::ElideRight, textRect.width()));
        }
        painter->restore();
        return;
    }

    QlementineAppStyle::drawControl(element, option, painter, widget);
}

///
/// \brief Draws an outlined bezel behind ThemedToolButtons; everything else defers to the base style.
/// \param element Primitive element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void MacAppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                 QPainter* painter, const QWidget* widget) const
{
    if (element == PE_PanelButtonTool && option
        && qobject_cast<const ThemedToolButton*>(widget)
        && !qobject_cast<const MainToolButton*>(widget)
        && !option->state.testFlag(State_On)) {
        drawOutlinedToolButton(option, painter);
        return;
    }

    if (element == PE_FrameTabWidget && option) {
        const QColor& canvas = isDarkMode() ? colorRef(Dark::kAlternateRow) : colorRef(Light::kAlternateRow);
        const qreal radius = theme().borderRadius * 1.5;
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addRoundedRect(QRectF(option->rect), radius, radius);
        path.addRect(QRectF(option->rect).adjusted(0, 0, 0, -radius));
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(path.simplified(), canvas);
        painter->restore();
    }

    QlementineAppStyle::drawPrimitive(element, option, painter, widget);
}

///
/// \brief Runs base polishing, then keeps item-view icons in their original colours.
/// \param widget Widget being polished.
///
void MacAppStyle::polish(QWidget* widget)
{
    QlementineAppStyle::polish(widget);

    if (qobject_cast<QAbstractItemView*>(widget) || qobject_cast<MainToolButton*>(widget))
        setAutoIconColor(widget, oclero::qlementine::AutoIconColor::None);
}

///
/// \brief Paints the macOS bezel (fill + border) behind an outlined tool button.
/// \param option Style option carrying the tool-button state.
/// \param painter Painter to draw with.
///
void MacAppStyle::drawOutlinedToolButton(const QStyleOption* option, QPainter* painter) const
{
    const bool enabled = option->state.testFlag(State_Enabled);
    const bool pressed = option->state.testFlag(State_Sunken);
    const bool hovered = option->state.testFlag(State_MouseOver);

    QRgb fillRgb;
    QRgb borderRgb;
    if (isDarkMode()) {
        using namespace Dark;
        if (!enabled) {
            fillRgb = kChrome;         borderRgb = kBorder;
        } else if (pressed) {
            fillRgb = kChromePressed;  borderRgb = kBorderActive;
        } else if (hovered) {
            fillRgb = kChromePressed;  borderRgb = kBorderActive;
        } else {
            fillRgb = kChromeStrong;   borderRgb = kBorderActive;
        }
    } else {
        using namespace Light;
        if (!enabled) {
            fillRgb = kChromeDimmed;   borderRgb = kChromeStrong;
        } else if (pressed) {
            fillRgb = kChromePressed;  borderRgb = kBorderActive;
        } else if (hovered) {
            fillRgb = kChromeStrong;   borderRgb = kBorderActive;
        } else {
            fillRgb = kCanvasWarm;     borderRgb = kBorder;
        }
    }

    // Match the base style's 6px radius / 1px border so bezels line up with the
    // accent fill of the checked button in the same row.
    constexpr qreal kRadius = 6.0;
    const QRectF rect = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(colorRef(borderRgb), 1.0));
    painter->setBrush(colorRef(fillRgb));
    painter->drawRoundedRect(rect, kRadius, kRadius);
    painter->restore();
}

///
/// \brief Background colour for buttons; only checked toggles keep the accent fill.
/// \param mouse Mouse interaction state.
/// \param role Button colour role.
/// \param widget Button widget, used to tell checked toggles from default buttons.
/// \return Reference to the resolved background colour.
///
QColor const& MacAppStyle::buttonBackgroundColor(MouseState mouse, ColorRole role, const QWidget* widget) const
{
    if (role == ColorRole::Primary) {
        const auto* button = qobject_cast<const QAbstractButton*>(widget);
        if (button && button->isChecked())
            return QlementineAppStyle::buttonBackgroundColor(mouse, role, widget);
    }

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(0x545456);
            case MouseState::Hovered:
                return colorRef(kChromePressed);
            case MouseState::Disabled:
                return colorRef(kChrome);
            case MouseState::Transparent:
                return transparentRef(kChromeStrong);
            case MouseState::Normal:
            default:
                return colorRef(kChromeStrong);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(0xdcdcdc);
            case MouseState::Disabled:
                return colorRef(kChromeDimmed);
            case MouseState::Transparent:
                return transparentRef(kChromeStrong);
            case MouseState::Normal:
            default:
                return colorRef(kCanvasWarm);
        }
    }
}

///
/// \brief Foreground colour for buttons; only checked toggles keep the accent foreground.
/// \param mouse Mouse interaction state.
/// \param role Button colour role.
/// \param widget Button widget, used to tell checked toggles from default buttons.
/// \return Reference to the resolved foreground colour.
///
QColor const& MacAppStyle::buttonForegroundColor(MouseState mouse, ColorRole role, const QWidget* widget) const
{
    if (role == ColorRole::Primary) {
        // Mirrors buttonBackgroundColor(): accent text only on checked toggles.
        const auto* button = qobject_cast<const QAbstractButton*>(widget);
        if (button && button->isChecked())
            return QlementineAppStyle::buttonForegroundColor(mouse, role, widget);
    }

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief Icon tint for secondary roles; primary roles defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Icon colour role.
/// \return Reference to the resolved icon colour.
///
QColor const& MacAppStyle::iconForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineAppStyle::iconForegroundColor(mouse, role);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kIconActive);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kIconNormal);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kIconActive);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kIconNormal);
        }
    }
}

///
/// \brief Background colour for a list/tree item, honouring per-row model colours when unselected.
/// \param mouse Mouse interaction state.
/// \param selected Selection state.
/// \param focus Focus state (unused).
/// \param active Active state (unused).
/// \param index Model index of the item.
/// \param widget Owning view (unused).
/// \return Resolved item background colour.
///
QColor MacAppStyle::listItemBackgroundColor(MouseState mouse, SelectionState selected, FocusState focus,
                                             ActiveState active, const QModelIndex& index,
                                             const QWidget* widget) const
{
    Q_UNUSED(focus)
    Q_UNUSED(active)
    Q_UNUSED(widget)

    const bool isSelected = selected == SelectionState::Selected;
    const QColor rowColor = modelBackgroundColor(index);

    if (isDarkMode()) {
        using namespace Dark;
        if (isSelected)
            return mouse == MouseState::Disabled ? QColor(kChromeStrong) : QColor(kBlueDisabled);
        switch (mouse) {
            case MouseState::Hovered:
                return QColor(kChromeStrong);
            case MouseState::Pressed:
                return QColor(kChromePressed);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                if (rowColor.isValid())
                    return rowColor;
                return transparent(kCanvas);
        }
    } else {
        using namespace Light;
        if (isSelected)
            return mouse == MouseState::Disabled ? QColor(kChromeStrong) : QColor(0xd9eaff);
        switch (mouse) {
            case MouseState::Hovered:
                return QColor(kChrome);
            case MouseState::Pressed:
                return QColor(kChromeStrong);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                if (rowColor.isValid())
                    return rowColor;
                return transparent(kCanvas);
        }
    }
}

///
/// \brief Text colour for a list/tree item.
/// \param mouse Mouse interaction state.
/// \param selected Selection state (unused).
/// \param focus Focus state (unused).
/// \param active Active state (unused).
/// \return Reference to the resolved item text colour.
///
QColor const& MacAppStyle::listItemForegroundColor(MouseState mouse, SelectionState selected,
                                                    FocusState focus, ActiveState active) const
{
    Q_UNUSED(selected)
    Q_UNUSED(focus)
    Q_UNUSED(active)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief Colour of splitter handles, brightening on hover/press.
/// \param mouse Mouse interaction state.
/// \return Reference to the resolved splitter colour.
///
QColor const& MacAppStyle::splitterColor(MouseState mouse) const
{
    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kBorderActive);
            default:
                return colorRef(kBorder);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kBorderActive);
            default:
                return colorRef(kBorder);
        }
    }
}

///
/// \brief Background colour of a tab; the selected tab matches the canvas.
/// \param mouse Mouse interaction state.
/// \param selected Selection state.
/// \return Reference to the resolved tab background colour.
///
QColor const& MacAppStyle::tabBackgroundColor(MouseState mouse, SelectionState selected) const
{
    const bool isSelected = selected == SelectionState::Selected;

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return isSelected ? colorRef(kAlternateRow) : colorRef(kChromePressed);
            case MouseState::Hovered:
                return isSelected ? colorRef(kAlternateRow) : colorRef(kChromeStrong);
            case MouseState::Disabled:
                return transparentRef(kChrome);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return isSelected ? colorRef(kAlternateRow) : transparentRef(kChrome);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return isSelected ? colorRef(kAlternateRow) : colorRef(kChromePressed);
            case MouseState::Hovered:
                return isSelected ? colorRef(kAlternateRow) : colorRef(kChromeStrong);
            case MouseState::Disabled:
                return transparentRef(kChrome);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return isSelected ? colorRef(kAlternateRow) : transparentRef(kChrome);
        }
    }
}

///
/// \brief Background colour of the tab bar behind the tabs.
/// \param mouse Mouse interaction state.
/// \return Reference to the resolved tab-bar background colour.
///
QColor const& MacAppStyle::tabBarBackgroundColor(MouseState mouse) const
{
    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kChromeStrong) : colorRef(kChrome);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kChromeStrong) : colorRef(kChrome);
    }
}

///
/// \brief Text colour of a tab label.
/// \param mouse Mouse interaction state.
/// \param selected Selection state (unused).
/// \return Reference to the resolved tab text colour.
///
QColor const& MacAppStyle::tabForegroundColor(MouseState mouse, SelectionState selected) const
{
    Q_UNUSED(selected)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief Background colour of a table header section.
/// \param mouse Mouse interaction state.
/// \param checked Sort/check state (unused).
/// \return Reference to the resolved header background colour.
///
QColor const& MacAppStyle::tableHeaderBgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
                return colorRef(kChrome);
            default:
                return colorRef(kChrome);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
                return colorRef(kChrome);
            default:
                return colorRef(kCanvas);
        }
    }
}

///
/// \brief Text colour of a table header section.
/// \param mouse Mouse interaction state.
/// \param checked Sort/check state (unused).
/// \return Reference to the resolved header text colour.
///
QColor const& MacAppStyle::tableHeaderFgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief Colour of the grid lines between table cells.
/// \return Reference to the resolved grid-line colour.
///
QColor const& MacAppStyle::tableLineColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kBorder);
    else
        return colorRef(Light::kChromeStrong);
}

///
/// \brief Background colour of text input fields.
/// \param mouse Mouse interaction state.
/// \param status Validation status (unused).
/// \return Reference to the resolved field background colour.
///
QColor const& MacAppStyle::textFieldBackgroundColor(MouseState mouse, Status status) const
{
    Q_UNUSED(status)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kChrome) : colorRef(kCanvas);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kChrome) : colorRef(kCanvas);
    }
}

///
/// \brief Border colour of text input fields; non-default statuses defer to the base style.
/// \param mouse Mouse interaction state.
/// \param focus Focus state.
/// \param status Validation status.
/// \return Reference to the resolved field border colour.
///
QColor const& MacAppStyle::textFieldBorderColor(MouseState mouse, FocusState focus, Status status) const
{
    if (status != Status::Default)
        return QlementineAppStyle::textFieldBorderColor(mouse, focus, status);

    if (isDarkMode()) {
        using namespace Dark;
        if (mouse == MouseState::Disabled)
            return colorRef(kChrome);
        if (focus == FocusState::Focused)
            return colorRef(kBlue);
        if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
            return colorRef(kBorderActive);
        return colorRef(kBorder);
    } else {
        using namespace Light;
        if (mouse == MouseState::Disabled)
            return colorRef(kChromeStrong);
        if (focus == FocusState::Focused)
            return colorRef(kBlue);
        if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
            return colorRef(kBorderActive);
        return colorRef(kBorder);
    }
}

///
/// \brief Background colour of toolbars.
/// \return Reference to the resolved toolbar background colour.
///
QColor const& MacAppStyle::toolBarBackgroundColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kChrome);
    else
        return colorRef(Light::kChrome);
}

///
/// \brief Border colour of toolbars.
/// \return Reference to the resolved toolbar border colour.
///
QColor const& MacAppStyle::toolBarBorderColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kBorder);
    else
        return colorRef(Light::kBorder);
}

///
/// \brief Text colour of tooltips.
/// \return Reference to the resolved tooltip text colour.
///
QColor const& MacAppStyle::toolTipForegroundColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kText);
    else
        return colorRef(Light::kCanvas);
}

///
/// \brief Background colour for secondary tool buttons; primary roles defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Tool-button colour role.
/// \return Reference to the resolved background colour.
///
QColor const& MacAppStyle::toolButtonBackgroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineAppStyle::toolButtonBackgroundColor(mouse, role);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return transparentRef(kChrome);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return transparentRef(kChrome);
        }
    }
}

///
/// \brief Foreground colour for secondary tool buttons; primary roles defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Tool-button colour role.
/// \return Reference to the resolved foreground colour.
///
QColor const& MacAppStyle::toolButtonForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineAppStyle::toolButtonForegroundColor(mouse, role);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kIconActive);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kText);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kBlueDeepPress);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kText);
        }
    }
}

#endif // HAVE_QLEMENTINE_APP_STYLE
