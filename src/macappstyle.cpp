// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file macappstyle.cpp
/// \brief Implements the macOS-flavoured application style.
///

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "macappstyle.h"
#include "macthemefactory.h"
#include "application.h"

#include <QBrush>
#include <QDockWidget>
#include <QFontDatabase>
#include <QHash>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionDockWidget>
#include <QStyleOptionTab>
#include <QTabBar>

namespace {

using oclero::qlementine::ActiveState;
using oclero::qlementine::CheckState;
using oclero::qlementine::ColorRole;
using oclero::qlementine::FocusState;
using oclero::qlementine::MouseState;
using oclero::qlementine::SelectionState;
using oclero::qlementine::Status;
using oclero::qlementine::Theme;

// Light mode macOS colors
namespace Light {
    constexpr QRgb kCanvas         = 0xffffff;
    constexpr QRgb kChrome         = 0xf2f2f7;
    constexpr QRgb kChromeStrong   = 0xe5e5ea;
    constexpr QRgb kChromePressed  = 0xd1d1d6;
    constexpr QRgb kBorder         = 0xd1d1d6;
    constexpr QRgb kBorderActive   = 0xaeaeb2;
    constexpr QRgb kText           = 0x000000;
    constexpr QRgb kMutedText      = 0x8e8e93;
    constexpr QRgb kDisabledText   = 0xc7c7cc;
    constexpr QRgb kBlue           = 0x007aff;
    constexpr QRgb kBlueHover      = 0x1a8aff;
    constexpr QRgb kBluePressed    = 0x0062cc;
    constexpr QRgb kBlueDisabled   = 0xb3d7ff;
    constexpr QRgb kBlueDeepPress  = 0x0051a8;
    constexpr QRgb kGreen          = 0x34c759;
    constexpr QRgb kIconNormal     = 0x3c3c43;
    constexpr QRgb kIconActive     = 0x0062cc;
    constexpr QRgb kChromeDimmed   = 0xf4f6f8;
    constexpr QRgb kCanvasWarm     = 0xfefefe;
}

// Dark mode macOS colors
namespace Dark {
    constexpr QRgb kCanvas         = 0x1c1c1e;
    constexpr QRgb kChrome         = 0x2c2c2e;
    constexpr QRgb kChromeStrong   = 0x3a3a3c;
    constexpr QRgb kChromePressed  = 0x48484a;
    constexpr QRgb kBorder         = 0x38383a;
    constexpr QRgb kBorderActive   = 0x545456;
    constexpr QRgb kText           = 0xffffff;
    constexpr QRgb kMutedText      = 0x8e8e93;
    constexpr QRgb kDisabledText   = 0x48484a;
    constexpr QRgb kBlue           = 0x0a84ff;
    constexpr QRgb kBlueHover      = 0x409cff;
    constexpr QRgb kBluePressed    = 0x0071e3;
    constexpr QRgb kBlueDisabled   = 0x0a3d73;
    constexpr QRgb kGreen          = 0x30d158;
    constexpr QRgb kIconNormal     = 0xebebf5;
    constexpr QRgb kIconActive     = 0x409cff;
}

///
/// \brief transparent
/// \param rgb
/// \return
///
QColor transparent(QRgb rgb)
{
    QColor color(rgb);
    color.setAlpha(0);
    return color;
}

///
/// \brief alpha
/// \param rgb
/// \param value
/// \return
///
QColor alpha(QRgb rgb, int value)
{
    QColor color(rgb);
    color.setAlpha(value);
    return color;
}

///
/// \brief colorRef
/// \param rgb
/// \return
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
/// \brief transparentRef
/// \param rgb
/// \return
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
/// \brief modelBackgroundColor
/// \param index
/// \return
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
/// \brief MacAppStyle::MacAppStyle
/// \param parent
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
/// \brief MacAppStyle::updateTheme
///
void MacAppStyle::updateTheme()
{
    setTheme(isDarkMode() ? _darkTheme : _lightTheme);
}

///
/// \brief MacAppStyle::isDarkMode
/// \return
///
bool MacAppStyle::isDarkMode() const
{
    return theApp() && theApp()->theme().isDark();
}

///
/// \brief MacAppStyle::drawControl
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
/// \brief MacAppStyle::buttonBackgroundColor
///
QColor const& MacAppStyle::buttonBackgroundColor(MouseState mouse, ColorRole role, const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineAppStyle::buttonBackgroundColor(mouse, role, widget);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
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
                return colorRef(kChromeStrong);
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
/// \brief MacAppStyle::buttonForegroundColor
///
QColor const& MacAppStyle::buttonForegroundColor(MouseState mouse, ColorRole role, const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineAppStyle::buttonForegroundColor(mouse, role, widget);

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief MacAppStyle::iconForegroundColor
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
/// \brief MacAppStyle::listItemBackgroundColor
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
            return mouse == MouseState::Disabled ? QColor(0x3a3a3c) : QColor(0x0a3d73);
        switch (mouse) {
            case MouseState::Hovered:
                return QColor(0x3a3a3c);
            case MouseState::Pressed:
                return QColor(0x48484a);
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
            return mouse == MouseState::Disabled ? QColor(0xe5e5ea) : QColor(0xd9eaff);
        switch (mouse) {
            case MouseState::Hovered:
                return QColor(0xf2f2f7);
            case MouseState::Pressed:
                return QColor(0xe5e5ea);
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
/// \brief MacAppStyle::listItemForegroundColor
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
/// \brief MacAppStyle::splitterColor
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
/// \brief MacAppStyle::tabBackgroundColor
///
QColor const& MacAppStyle::tabBackgroundColor(MouseState mouse, SelectionState selected) const
{
    const bool isSelected = selected == SelectionState::Selected;

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromePressed);
            case MouseState::Hovered:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromeStrong);
            case MouseState::Disabled:
                return transparentRef(kChrome);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return isSelected ? colorRef(kCanvas) : transparentRef(kChrome);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromePressed);
            case MouseState::Hovered:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromeStrong);
            case MouseState::Disabled:
                return transparentRef(kChrome);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return isSelected ? colorRef(kCanvas) : transparentRef(kChrome);
        }
    }
}

///
/// \brief MacAppStyle::tabBarBackgroundColor
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
/// \brief MacAppStyle::tabForegroundColor
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
/// \brief MacAppStyle::tableHeaderBgColor
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
/// \brief MacAppStyle::tableHeaderFgColor
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
/// \brief MacAppStyle::tableLineColor
///
QColor const& MacAppStyle::tableLineColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kBorder);
    else
        return colorRef(Light::kChromeStrong);
}

///
/// \brief MacAppStyle::textFieldBackgroundColor
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
/// \brief MacAppStyle::textFieldBorderColor
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
/// \brief MacAppStyle::toolBarBackgroundColor
///
QColor const& MacAppStyle::toolBarBackgroundColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kChrome);
    else
        return colorRef(Light::kChrome);
}

///
/// \brief MacAppStyle::toolBarBorderColor
///
QColor const& MacAppStyle::toolBarBorderColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kBorder);
    else
        return colorRef(Light::kBorder);
}

///
/// \brief MacAppStyle::toolTipForegroundColor
///
QColor const& MacAppStyle::toolTipForegroundColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kText);
    else
        return colorRef(Light::kCanvas);
}

///
/// \brief MacAppStyle::toolButtonBackgroundColor
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
/// \brief MacAppStyle::toolButtonForegroundColor
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
