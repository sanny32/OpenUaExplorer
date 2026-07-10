// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file qlementineappstyle.cpp
/// \brief Implements the Qlementine-based application style.
///

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "qlementineappstyle.h"
#include "qlementinethemefactory.h"
#include "application.h"
#include <oclero/qlementine/icons/QlementineIcons.hpp>

#include <QComboBox>
#include <QDockWidget>
#include <QPushButton>
#include <QBrush>
#include <QHash>
#include <QLineEdit>
#include <QLabel>
#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QStyleOptionButton>
#include <QStyleOptionDockWidget>
#include <QStyleOptionMenuItem>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QStyleOptionViewItem>
#include <QApplication>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTextBrowser>
#include <QTabBar>
#include <QToolBar>
#include <QMenu>
#include <QFocusFrame>
#include <QToolButton>
#include <QWidget>

#include <algorithm>

namespace {

using oclero::qlementine::ActiveState;
using oclero::qlementine::CheckState;
using oclero::qlementine::ColorRole;
using oclero::qlementine::FocusState;
using oclero::qlementine::MouseState;
using oclero::qlementine::SelectionState;
using oclero::qlementine::Status;
using oclero::qlementine::Theme;

namespace Light {
constexpr QRgb kCanvas = 0xffffff;
constexpr QRgb kChrome = 0xf5f5f5;
constexpr QRgb kChromeStrong = 0xececec;
constexpr QRgb kChromePressed = 0xe0e0e0;
constexpr QRgb kChromeMuted = 0xfefefe;
constexpr QRgb kChromeDisabled = 0xf4f6f8;
constexpr QRgb kSurfaceDisabled = 0xf2f2f2;
constexpr QRgb kBorder = 0xe3e6ea;
constexpr QRgb kBorderActive = 0xcdd1d6;
constexpr QRgb kBorderMuted = 0xe6e9ee;
constexpr QRgb kBorderDisabled = 0xebeef2;
constexpr QRgb kText = 0x1f2937;
constexpr QRgb kTextMuted = 0x4f627a;
constexpr QRgb kTextEmphasis = 0x24364d;
constexpr QRgb kDisabledText = 0xb2bac5;
constexpr QRgb kBlue = 0x007aff;
constexpr QRgb kBlueHover = 0x1a8aff;
constexpr QRgb kBluePressed = 0x0062cc;
constexpr QRgb kBlueDisabled = 0xb3d7ff;
constexpr QRgb kSelection = 0xd9eaff;
constexpr QRgb kSelectionDisabled = 0xeaeaea;
constexpr QRgb kItemHover = 0xf0f0f0;
constexpr QRgb kItemPressed = 0xe5e5e5;
constexpr QRgb kTooltipBase = 0x1f2937;
constexpr QRgb kTooltipText = 0xffffff;
}

namespace Dark {
constexpr QRgb kCanvas = 0x1e1f24;
constexpr QRgb kChrome = 0x272a30;
constexpr QRgb kChromeStrong = 0x333841;
constexpr QRgb kChromePressed = 0x414854;
constexpr QRgb kChromeMuted = 0x2d3138;
constexpr QRgb kChromeDisabled = 0x24272d;
constexpr QRgb kSurfaceDisabled = 0x21242a;
constexpr QRgb kBorder = 0x3b414c;
constexpr QRgb kBorderActive = 0x5b6575;
constexpr QRgb kBorderMuted = 0x313740;
constexpr QRgb kBorderDisabled = 0x2a2e36;
constexpr QRgb kText = 0xe7ecf3;
constexpr QRgb kTextMuted = 0xa0abbb;
constexpr QRgb kTextEmphasis = 0xffffff;
constexpr QRgb kDisabledText = 0x6c7582;
constexpr QRgb kBlue = 0x409cff;
constexpr QRgb kBlueHover = 0x66b2ff;
constexpr QRgb kBluePressed = 0x1f8bff;
constexpr QRgb kBlueDisabled = 0x27476a;
constexpr QRgb kSelection = 0x1d3c5c;
constexpr QRgb kSelectionDisabled = 0x2b3139;
constexpr QRgb kItemHover = 0x2e333b;
constexpr QRgb kItemPressed = 0x373d46;
constexpr QRgb kTooltipBase = 0x111318;
constexpr QRgb kTooltipText = 0xe7ecf3;
}

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

///
/// \brief Checks whether a button option represents an indicator without adjacent content.
/// \param option Style option to inspect.
/// \return True when the button has neither text nor icon.
///
bool isStandaloneIndicatorButton(const QStyleOption* option)
{
    const auto* buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
    return buttonOption && buttonOption->text.isEmpty() && buttonOption->icon.isNull();
}

///
/// \brief Tints a pixmap to a single colour while preserving its alpha.
/// \param src Source pixmap.
/// \param color Colour to tint with.
/// \return Recoloured copy of the pixmap.
///
QPixmap tintedPixmap(const QPixmap& src, const QColor& color)
{
    if (src.isNull())
        return src;

    QPixmap result(src.size());
    result.setDevicePixelRatio(src.devicePixelRatio());
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.drawPixmap(0, 0, src);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), color);
    painter.end();

    return result;
}

///
/// \brief Computes the padding that insets the active-tab indicator to the tab shape.
/// \param option Tab style option carrying the tab's position in the bar.
/// \param spacing Base spacing of the active theme.
/// \return Margins to remove from the tab rect.
///
QMargins tabShapePadding(const QStyleOptionTab& option, int spacing)
{
    const bool isFirst = option.position == QStyleOptionTab::OnlyOneTab
        || option.position == QStyleOptionTab::Beginning;
    const bool isLast = option.position == QStyleOptionTab::OnlyOneTab
        || option.position == QStyleOptionTab::End;
    const bool notBesideSelected = option.selectedPosition == QStyleOptionTab::NotAdjacent;
    const bool onlyOneTab = option.position == QStyleOptionTab::OnlyOneTab;
    const bool isMovedTab = notBesideSelected && onlyOneTab;

    return QMargins(isMovedTab || isFirst ? spacing : 0,
                    spacing / 2,
                    isMovedTab || isLast ? spacing : 0,
                    0);
}

} // namespace

///
/// \brief Builds the style, initialises the icon theme, and tracks app color-scheme changes.
/// \param parent Owning QObject.
///
QlementineAppStyle::QlementineAppStyle(QObject* parent)
    : QlementineStyle(parent)
{
    setAutoIconColor(oclero::qlementine::AutoIconColor::ForegroundColor);

    oclero::qlementine::icons::initializeIconTheme();
    QIcon::setThemeName(QStringLiteral("qlementine"));

    const Theme baseTheme = theme();
    _lightTheme = QlementineThemeFactory::make(baseTheme, false);
    _darkTheme = QlementineThemeFactory::make(baseTheme, true);
    updateTheme();

    connect(&theApp()->theme(), &AppTheme::colorSchemeChanged,
            this, [this]() { updateTheme(); });

    connect(this, &QlementineStyle::themeChanged, this, [this]() {
        for (auto* widget : QApplication::allWidgets())
            polishWidget(widget);
    });
}

///
/// \brief Switches the active Qlementine theme to match the current light/dark mode.
///
void QlementineAppStyle::updateTheme()
{
    setTheme(isDarkMode() ? _darkTheme : _lightTheme);
}

///
/// \brief Reports whether the application theme is currently dark.
/// \return True when the active scheme is dark.
///
bool QlementineAppStyle::isDarkMode() const
{
    return theApp() && theApp()->theme().isDark();
}

///
/// \brief Applies per-widget tweaks (tooltip roles, non-bold buttons, MDI/text-browser colours).
/// \param widget Widget being polished.
///
void QlementineAppStyle::polishWidget(QWidget* widget) const
{
    if (auto* label = qobject_cast<QLabel*>(widget)) {
        if (label->inherits("QTipLabel")) {
            label->setForegroundRole(QPalette::ToolTipText);
        } else {
            label->setForegroundRole(QPalette::WindowText);
        }
    }

    if (qobject_cast<QPushButton*>(widget) || qobject_cast<QToolButton*>(widget)) {
        auto font = widget->font();
        font.setBold(false);
        widget->setFont(font);
    }

    if (auto* mdiArea = qobject_cast<QMdiArea*>(widget))
        mdiArea->setBackground(QBrush(theme().backgroundColorMain1));

    if (qobject_cast<QTextBrowser*>(widget)) {
        auto pal = widget->palette();
        const QColor canvas = QApplication::palette().color(QPalette::Base);
        pal.setColor(QPalette::Base, canvas);
        pal.setColor(QPalette::Window, canvas);
        widget->setPalette(pal);
    }
}

///
/// \brief Background colour for secondary buttons; primary buttons defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Button colour role.
/// \param widget Button widget (unused).
/// \return Reference to the resolved background colour.
///
QColor const& QlementineAppStyle::buttonBackgroundColor(MouseState mouse, ColorRole role,
                                                  const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineStyle::buttonBackgroundColor(mouse, role, widget);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kChromeDisabled : Light::kChromeDisabled);
        case MouseState::Transparent:
            return transparentRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kChromeMuted : Light::kChromeMuted);
    }
}

///
/// \brief Foreground colour for secondary buttons; primary buttons defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Button colour role.
/// \param widget Button widget (unused).
/// \return Reference to the resolved foreground colour.
///
QColor const& QlementineAppStyle::buttonForegroundColor(MouseState mouse, ColorRole role,
                                                 const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineStyle::buttonForegroundColor(mouse, role, widget);

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief Custom-draws elements Qlementine lacks: text-under-icon tool buttons, centred item
///        text, un-recoloured menu icons, dock titles, and the active-MDI tab indicator.
/// \param element Control element to render.
/// \param option Style option carrying the element state.
/// \param painter Painter to draw with.
/// \param widget Widget the element belongs to.
///
void QlementineAppStyle::drawControl(ControlElement element, const QStyleOption* option,
                              QPainter* painter, const QWidget* widget) const
{
    // Qlementine does not implement the "text under icon" tool button layout, so
    // draw it ourselves: icon centred on top, label centred below.
    if (element == CE_ToolButtonLabel) {
        if (const auto* tb = qstyleoption_cast<const QStyleOptionToolButton*>(option)) {
            if (tb->toolButtonStyle == Qt::ToolButtonTextUnderIcon
                && !tb->icon.isNull() && !tb->text.isEmpty()) {
                const QRect rect = tb->rect;
                const auto buttonState = tb->state;
                const MouseState mouse = !(buttonState & State_Enabled)
                    ? MouseState::Disabled
                    : (buttonState & State_Sunken)
                          ? MouseState::Pressed
                          : (buttonState & State_MouseOver)
                                ? MouseState::Hovered
                                : MouseState::Normal;
                const QColor fgColor = toolButtonForegroundColor(mouse, ColorRole::Secondary);

                const QIcon::Mode iconMode = mouse == MouseState::Disabled
                    ? QIcon::Disabled : QIcon::Normal;
                const QIcon::State iconState = (buttonState & State_On) ? QIcon::On : QIcon::Off;
                const qreal dpr = widget ? widget->devicePixelRatioF() : qApp->devicePixelRatio();
                QPixmap pm = tb->icon.pixmap(tb->iconSize, dpr, iconMode, iconState);
                if (autoIconColor(widget) != oclero::qlementine::AutoIconColor::None)
                    pm = tintedPixmap(pm, fgColor);

                const qreal pmDpr = pm.devicePixelRatio() > 0 ? pm.devicePixelRatio() : 1.0;
                const int pmW = qRound(pm.width() / pmDpr);
                const int pmH = qRound(pm.height() / pmDpr);

                const int gap = 2;
                const int textH = tb->fontMetrics.height();
                const int totalH = pmH + gap + textH;
                const int top = rect.y() + (rect.height() - totalH) / 2;
                const int iconX = rect.x() + (rect.width() - pmW) / 2;

                painter->save();
                painter->drawPixmap(QRect(iconX, top, pmW, pmH), pm);

                const QRect textRect(rect.x(), top + pmH + gap, rect.width(), textH);
                const QString elided = tb->fontMetrics.elidedText(
                    tb->text, Qt::ElideRight, rect.width(), Qt::TextSingleLine);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(fgColor);
                painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, elided);
                painter->restore();
                return;
            }
        }
    }

    if (element == CE_ItemViewItem) {
        const auto* itemOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);
        const bool centeredText = itemOption
                               && itemOption->displayAlignment.testFlag(Qt::AlignHCenter)
                               && !itemOption->text.isEmpty();
        if (centeredText) {
            QStyleOptionViewItem noTextOption(*itemOption);
            noTextOption.text.clear();
            QlementineStyle::drawControl(element, &noTextOption, painter, widget);

            const bool isList = qobject_cast<const QListView*>(widget) != nullptr;
            const int spacing = theme().spacing;
            const int hPadding = isList ? spacing : spacing / 2;
            const bool hasCheck = itemOption->features.testFlag(QStyleOptionViewItem::HasCheckIndicator);
            const int checkSpace = hasCheck ? theme().iconSize.width() + spacing : 0;
            const bool hasIcon = itemOption->features.testFlag(QStyleOptionViewItem::HasDecoration)
                              && !itemOption->icon.isNull();
            const int iconWidth = hasIcon ? itemOption->decorationSize.width() : 0;
            const int iconSpace = hasIcon ? iconWidth + (iconWidth > 0 ? spacing : 0) : 0;
            QRect textRect = itemOption->rect.marginsRemoved(QMargins(hPadding, 0, hPadding, 0));
            textRect.adjust(checkSpace + iconSpace, 0, 0, 0);

            if (textRect.width() > 0) {
                const MouseState mouse = !(itemOption->state & State_Enabled)
                                       ? MouseState::Disabled
                                       : (itemOption->state & State_Sunken)
                                             ? MouseState::Pressed
                                             : (itemOption->state & State_MouseOver)
                                                   ? MouseState::Hovered
                                                   : MouseState::Normal;
                const SelectionState selected = (itemOption->state & State_Selected)
                                             ? SelectionState::Selected
                                             : SelectionState::NotSelected;
                const FocusState focus = (widget && widget->hasFocus() && selected == SelectionState::Selected)
                                      ? FocusState::Focused
                                      : FocusState::NotFocused;
                const ActiveState active = (itemOption->state & State_Active)
                                        ? ActiveState::Active
                                        : ActiveState::NotActive;
                const QColor textColor = listItemForegroundColor(mouse, selected, focus, active);
                const QString elidedText = itemOption->fontMetrics.elidedText(
                    itemOption->text, Qt::ElideRight, textRect.width(), Qt::TextSingleLine);

                painter->save();
                painter->setFont(itemOption->font);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(textColor);
                painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextSingleLine, elidedText);
                painter->restore();
            }
            return;
        }
    }

    if (element == CE_MenuItem) {
        if (const auto* opt = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
            if (!opt->icon.isNull()) {
                const auto* menu = qobject_cast<const QMenu*>(widget);
                if (menu) {
                    const auto& actions = menu->actions();
                    const auto it = std::find_if(actions.cbegin(), actions.cend(), [&](const QAction* a) {
                        return a->text() == opt->text && !a->icon().isNull();
                    });
                    if (it != actions.cend() && (*it)->property("qlementineNoRecolor").toBool()) {
                        const auto savedColor = autoIconColor(widget);
                        setAutoIconColor(const_cast<QWidget*>(widget), oclero::qlementine::AutoIconColor::None);
                        QlementineStyle::drawControl(element, option, painter, widget);
                        setAutoIconColor(const_cast<QWidget*>(widget), savedColor);
                        return;
                    }
                }
            }
        }
    }

    if (element == CE_DockWidgetTitle) {
        const auto* dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
        if (!dockOption) {
            QlementineStyle::drawControl(element, option, painter, widget);
            return;
        }

        const QRect rect = dockOption->rect;
        const bool darkMode = isDarkMode();
        painter->save();
        painter->fillRect(rect, colorRef(darkMode ? Dark::kChrome : Light::kChrome));
        painter->setPen(QPen(colorRef(darkMode ? Dark::kBorder : Light::kBorder), 1));
        painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

        QRect textRect = rect.adjusted(6, 0, -42, 0);
        if (textRect.isValid()) {
            painter->setPen(colorRef(darkMode ? Dark::kText : Light::kText));
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                              painter->fontMetrics().elidedText(dockOption->title, Qt::ElideRight, textRect.width()));
        }
        painter->restore();
        return;
    }

    QlementineStyle::drawControl(element, option, painter, widget);

    if (element != CE_TabBarTab || !widget)
        return;

    const auto* tabBar = qobject_cast<const QTabBar*>(widget);
    const auto* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
    if (!tabBar || !tabOption)
        return;

    if (!(tabOption->state & State_Selected))
        return;

    if (!widget->property("mdiIndicatorActive").toBool())
        return;

    QRect r = tabBar->tabRect(tabBar->currentIndex());
    if (!r.isValid())
        r = tabOption->rect;

    r = r.marginsRemoved(tabShapePadding(*tabOption, theme().spacing));
    r = r.adjusted(1, 0, -1, -1);
    const QColor color = widget->palette().highlight().color();
    constexpr int thickness = 2;
    const qreal radius = theme().borderRadius;

    painter->save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(r), radius, radius);
    painter->setClipPath(clipPath);

    switch (tabBar->shape()) {
        case QTabBar::RoundedSouth:
        case QTabBar::TriangularSouth:
            painter->fillRect(r.left(), r.bottom() - thickness + 1, r.width(), thickness, color);
            break;
        case QTabBar::RoundedWest:
        case QTabBar::TriangularWest:
            painter->fillRect(r.left(), r.top(), thickness, r.height(), color);
            break;
        case QTabBar::RoundedEast:
        case QTabBar::TriangularEast:
            painter->fillRect(r.right() - thickness + 1, r.top(), thickness, r.height(), color);
            break;
        case QTabBar::RoundedNorth:
        case QTabBar::TriangularNorth:
        default:
            painter->fillRect(r.left(), r.top(), r.width(), thickness, color);
            break;
    }

    painter->restore();
}

///
/// \brief Icon tint for secondary roles; primary roles defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Icon colour role.
/// \return Reference to the resolved icon colour.
///
QColor const& QlementineAppStyle::iconForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::iconForegroundColor(mouse, role);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kBluePressed : Light::kBluePressed);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kDisabledText : Light::kDisabledText);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kTextMuted : Light::kTextMuted);
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
QColor QlementineAppStyle::listItemBackgroundColor(MouseState mouse, SelectionState selected, FocusState focus,
                                            ActiveState active, const QModelIndex& index,
                                            const QWidget* widget) const
{
    Q_UNUSED(focus)
    Q_UNUSED(active)
    Q_UNUSED(widget)

    const bool darkMode = isDarkMode();
    const bool isSelected = selected == SelectionState::Selected;
    const QColor rowColor = modelBackgroundColor(index);
    if (isSelected)
        return mouse == MouseState::Disabled
            ? QColor(darkMode ? Dark::kSelectionDisabled : Light::kSelectionDisabled)
            : QColor(darkMode ? Dark::kSelection : Light::kSelection);

    switch (mouse) {
        case MouseState::Hovered:
            return QColor(darkMode ? Dark::kItemHover : Light::kItemHover);
        case MouseState::Pressed:
            return QColor(darkMode ? Dark::kItemPressed : Light::kItemPressed);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            if (rowColor.isValid())
                return rowColor;
            return transparent(darkMode ? Dark::kCanvas : Light::kCanvas);
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
QColor const& QlementineAppStyle::listItemForegroundColor(MouseState mouse, SelectionState selected,
                                                   FocusState focus, ActiveState active) const
{
    Q_UNUSED(selected)
    Q_UNUSED(focus)
    Q_UNUSED(active)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief Overrides dock, splitter, and toolbar metrics; keeps pressed icons from shifting.
/// \param metric Pixel metric being queried.
/// \param option Style option for the metric.
/// \param widget Widget the metric applies to.
/// \return Metric value in pixels.
///
int QlementineAppStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    if (widget && widget->property("preservePressedIconAlignment").toBool()) {
        switch (metric) {
            case PM_ButtonShiftHorizontal:
            case PM_ButtonShiftVertical:
                return 0;
            default:
                break;
        }
    }

    switch (metric) {
        case PM_DockWidgetFrameWidth:
        case PM_DockWidgetSeparatorExtent:
            return 1;
        case PM_DockWidgetHandleExtent:
            return 22;
        case PM_DockWidgetTitleMargin:
        case PM_DockWidgetTitleBarButtonMargin:
            return 4;
        case PM_SplitterWidth:
            return 1;
        case PM_ToolBarIconSize:
        case PM_SmallIconSize:
            return 16;
        default:
            break;
    }

    return QlementineStyle::pixelMetric(metric, option, widget);
}

///
/// \brief Runs base polishing then the app-specific per-widget tweaks.
/// \param widget Widget being polished.
///
void QlementineAppStyle::polish(QWidget* widget)
{
    QlementineStyle::polish(widget);
    polishWidget(widget);
}

///
/// \brief Adjusts sizing for line edits, standalone indicators, and main-toolbar tool buttons.
/// \param type Contents type being measured.
/// \param option Style option carrying the element state.
/// \param contentsSize Size requested by the contents.
/// \param widget Widget the element belongs to.
/// \return Adjusted contents size.
///
QSize QlementineAppStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
                                    const QSize& contentsSize, const QWidget* widget) const
{
    QSize size = QlementineStyle::sizeFromContents(type, option, contentsSize, widget);

    if (type == CT_LineEdit && qobject_cast<const QLineEdit*>(widget))
        size.setWidth(qMax(size.width(), contentsSize.width()));

    if ((type == CT_CheckBox || type == CT_RadioButton) && isStandaloneIndicatorButton(option)) {
        const bool isRadio = type == CT_RadioButton;
        const int indicatorWidth = pixelMetric(isRadio ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth,
                                               option, widget);
        const int indicatorHeight = pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight,
                                                option, widget);
        size.setWidth(indicatorWidth);
        size.setHeight(qMax(size.height(), indicatorHeight));
    }

    if (type == CT_ToolButton && widget && widget->parentWidget()
        && widget->parentWidget()->objectName() == QStringLiteral("mainToolBar")) {
        if (const auto* tb = qstyleoption_cast<const QStyleOptionToolButton*>(option);
            tb && tb->toolButtonStyle == Qt::ToolButtonTextUnderIcon
            && !tb->icon.isNull() && !tb->text.isEmpty()) {
            const int gap = 2;
            const int vPadding = 8;
            const int needed = tb->iconSize.height() + gap + tb->fontMetrics.height() + vPadding;
            size.setHeight(qMax(size.height(), needed));
        } else {
            size.rheight() = qMax(size.height(), 36);
            size.rwidth() += 4;
        }
    }

    return size;
}

///
/// \brief Centres standalone check-box and radio-button indicators within their rect.
/// \param element Sub-element to locate.
/// \param option Style option carrying the element geometry.
/// \param widget Widget the element belongs to.
/// \return Resolved sub-element rectangle.
///
QRect QlementineAppStyle::subElementRect(SubElement element, const QStyleOption* option,
                                         const QWidget* widget) const
{
    if ((element == SE_CheckBoxIndicator || element == SE_RadioButtonIndicator)
        && option && isStandaloneIndicatorButton(option)) {
        const bool isRadio = element == SE_RadioButtonIndicator;
        const QSize indicatorSize(pixelMetric(isRadio ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth,
                                              option, widget),
                                  pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight,
                                              option, widget));
        return QStyle::alignedRect(option->direction, Qt::AlignCenter, indicatorSize, option->rect);
    }

    return QlementineStyle::subElementRect(element, option, widget);
}

///
/// \brief Colour of splitter handles, brightening on hover/press.
/// \param mouse Mouse interaction state.
/// \return Reference to the resolved splitter colour.
///
QColor const& QlementineAppStyle::splitterColor(MouseState mouse) const
{
    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kBorderActive : Light::kBorderActive);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kBorder : Light::kBorder);
    }
}

///
/// \brief Keeps main-toolbar buttons on their own toolButtonStyle and guards QFocusFrame against
///        MDI subwindows to avoid mapTo() warnings on close.
/// \param hint Style hint being queried.
/// \param option Style option for the hint.
/// \param widget Widget the hint applies to.
/// \param returnData Optional structured return data.
/// \return Style hint value.
///
int QlementineAppStyle::styleHint(StyleHint hint, const QStyleOption* option,
                           const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (hint == SH_ToolButtonStyle) {
        const auto* toolButton = qobject_cast<const QToolButton*>(widget);
        const auto* parent = toolButton ? toolButton->parentWidget() : nullptr;
        if (parent && parent->objectName() == QStringLiteral("mainToolBar"))
            return toolButton->toolButtonStyle();
    }

    // QMdiSubWindow::isWindow()==false causes QFocusFrame to escape the subwindow hierarchy → mapTo warnings on close.
    if (hint == SH_FocusFrame_AboveWidget && widget) {
        if (qobject_cast<const QToolButton*>(widget))
            return 0;

        const auto* focusFrame = qobject_cast<const QFocusFrame*>(widget);
        // styleHint(SH_FocusFrame_AboveWidget) is called from QFocusFrame::setWidget() BEFORE
        // d->widget is assigned, so focusFrame->widget() is null at that point.
        // Fall back to the focus frame's own parent chain: Qt constructs QFocusFrame(focusedWidget)
        // so at construction/initial-setWidget time the frame's parent IS the widget being focused.
        const QWidget* w = (focusFrame && focusFrame->widget()) ? focusFrame->widget() : widget;
        while (w) {
            if (qobject_cast<const QMdiSubWindow*>(w))
                return 0;
            // Return 1 for toolbar-embedded widgets (showFrameAboveWidget=true).
            // When QWidgetAction::releaseWidget calls setParent(null) during destruction,
            // QEvent::ParentChange triggers setWidget(null)/setWidget(w) where w->isWindow()==true
            // (Qt adds Qt::Window flag when parent is null), so setWidget takes the else-branch
            // and safely clears the frame — before updateSize can dereference null parentWidget().
            if (qobject_cast<const QToolBar*>(w))
                return 1;
            w = w->parentWidget();
        }
    }

    return QlementineStyle::styleHint(hint, option, widget, returnData);
}

///
/// \brief Background colour of a tab; the selected tab matches the canvas.
/// \param mouse Mouse interaction state.
/// \param selected Selection state.
/// \return Reference to the resolved tab background colour.
///
QColor const& QlementineAppStyle::tabBackgroundColor(MouseState mouse, SelectionState selected) const
{
    const bool darkMode = isDarkMode();
    const bool isSelected = selected == SelectionState::Selected;

    switch (mouse) {
        case MouseState::Pressed:
            return isSelected
                ? colorRef(darkMode ? Dark::kCanvas : Light::kCanvas)
                : colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return isSelected
                ? colorRef(darkMode ? Dark::kCanvas : Light::kCanvas)
                : colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
            return transparentRef(darkMode ? Dark::kChrome : Light::kChrome);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return isSelected
                ? colorRef(darkMode ? Dark::kCanvas : Light::kCanvas)
                : transparentRef(darkMode ? Dark::kChrome : Light::kChrome);
    }
}

///
/// \brief Background colour of the tab bar behind the tabs.
/// \param mouse Mouse interaction state.
/// \return Reference to the resolved tab-bar background colour.
///
QColor const& QlementineAppStyle::tabBarBackgroundColor(MouseState mouse) const
{
    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kChromeStrong : Light::kChromeStrong)
        : colorRef(isDarkMode() ? Dark::kChrome : Light::kChrome);
}

///
/// \brief Text colour of a tab label.
/// \param mouse Mouse interaction state.
/// \param selected Selection state (unused).
/// \return Reference to the resolved tab text colour.
///
QColor const& QlementineAppStyle::tabForegroundColor(MouseState mouse, SelectionState selected) const
{
    Q_UNUSED(selected)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief Background colour of a table header section.
/// \param mouse Mouse interaction state.
/// \param checked Sort/check state (unused).
/// \return Reference to the resolved header background colour.
///
QColor const& QlementineAppStyle::tableHeaderBgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kSurfaceDisabled : Light::kSurfaceDisabled);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kCanvas : Light::kCanvas);
    }
}

///
/// \brief Text colour of a table header section.
/// \param mouse Mouse interaction state.
/// \param checked Sort/check state (unused).
/// \return Reference to the resolved header text colour.
///
QColor const& QlementineAppStyle::tableHeaderFgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief Colour of the grid lines between table cells.
/// \return Reference to the resolved grid-line colour.
///
QColor const& QlementineAppStyle::tableLineColor() const
{
    return colorRef(isDarkMode() ? Dark::kBorderMuted : Light::kBorderMuted);
}

///
/// \brief Background colour of text input fields.
/// \param mouse Mouse interaction state.
/// \param status Validation status (unused).
/// \return Reference to the resolved field background colour.
///
QColor const& QlementineAppStyle::textFieldBackgroundColor(MouseState mouse, Status status) const
{
    Q_UNUSED(status)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kSurfaceDisabled : Light::kSurfaceDisabled)
        : colorRef(isDarkMode() ? Dark::kCanvas : Light::kCanvas);
}

///
/// \brief Border colour of text input fields; non-default statuses defer to the base style.
/// \param mouse Mouse interaction state.
/// \param focus Focus state.
/// \param status Validation status.
/// \return Reference to the resolved field border colour.
///
QColor const& QlementineAppStyle::textFieldBorderColor(MouseState mouse, FocusState focus, Status status) const
{
    if (status != Status::Default)
        return QlementineStyle::textFieldBorderColor(mouse, focus, status);

    const bool darkMode = isDarkMode();

    if (mouse == MouseState::Disabled)
        return colorRef(darkMode ? Dark::kBorderDisabled : Light::kBorderDisabled);
    if (focus == FocusState::Focused)
        return colorRef(darkMode ? Dark::kBlue : Light::kBlue);
    if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
        return colorRef(darkMode ? Dark::kBorderActive : Light::kBorderActive);

    return colorRef(darkMode ? Dark::kBorder : Light::kBorder);
}

///
/// \brief Background colour of toolbars.
/// \return Reference to the resolved toolbar background colour.
///
QColor const& QlementineAppStyle::toolBarBackgroundColor() const
{
    return colorRef(isDarkMode() ? Dark::kChrome : Light::kChrome);
}

///
/// \brief Border colour of toolbars.
/// \return Reference to the resolved toolbar border colour.
///
QColor const& QlementineAppStyle::toolBarBorderColor() const
{
    return colorRef(isDarkMode() ? Dark::kBorder : Light::kBorder);
}

///
/// \brief Background colour of tooltips.
/// \return Reference to the resolved tooltip background colour.
///
QColor const& QlementineAppStyle::toolTipBackgroundColor() const
{
    return colorRef(isDarkMode() ? Dark::kTooltipBase : Light::kTooltipBase);
}

///
/// \brief Border colour of tooltips.
/// \return Reference to the resolved tooltip border colour.
///
QColor const& QlementineAppStyle::toolTipBorderColor() const
{
    return colorRef(isDarkMode() ? Dark::kBorderActive : Light::kBorderActive);
}

///
/// \brief Text colour of tooltips.
/// \return Reference to the resolved tooltip text colour.
///
QColor const& QlementineAppStyle::toolTipForegroundColor() const
{
    return colorRef(isDarkMode() ? Dark::kTooltipText : Light::kTooltipText);
}

///
/// \brief Background colour for secondary tool buttons; primary roles defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Tool-button colour role.
/// \return Reference to the resolved background colour.
///
QColor const& QlementineAppStyle::toolButtonBackgroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::toolButtonBackgroundColor(mouse, role);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return transparentRef(darkMode ? Dark::kChrome : Light::kChrome);
    }
}

///
/// \brief Foreground colour for secondary tool buttons; primary roles defer to the base style.
/// \param mouse Mouse interaction state.
/// \param role Tool-button colour role.
/// \return Reference to the resolved foreground colour.
///
QColor const& QlementineAppStyle::toolButtonForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::toolButtonForegroundColor(mouse, role);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kTextEmphasis : Light::kTextEmphasis);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kDisabledText : Light::kDisabledText);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kText : Light::kText);
    }
}

#endif // HAVE_QLEMENTINE_APP_STYLE
