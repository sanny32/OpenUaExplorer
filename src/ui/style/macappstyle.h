// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file macappstyle.h
/// \brief Declares the macOS-flavoured application style.
///

#ifndef MACAPPSTYLE_H
#define MACAPPSTYLE_H

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "qlementineappstyle.h"

#include <oclero/qlementine/style/Theme.hpp>

///
/// \brief The MacAppStyle class
///
class MacAppStyle : public QlementineAppStyle
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the macOS style with its light/dark themes and tracks color-scheme changes.
    /// \param parent Owning QObject.
    ///
    explicit MacAppStyle(QObject* parent = nullptr);

    ///
    /// \brief Custom-draws macOS dock-widget titles; everything else defers to the base style.
    /// \param element Control element to render.
    /// \param option Style option carrying the element state.
    /// \param painter Painter to draw with.
    /// \param widget Widget the element belongs to.
    ///
    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget = nullptr) const override;

    ///
    /// \brief Draws an outlined bezel behind opted-in tool buttons; everything else defers to the base style.
    /// \param element Primitive element to render.
    /// \param option Style option carrying the element state.
    /// \param painter Painter to draw with.
    /// \param widget Widget the element belongs to.
    ///
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                       QPainter* painter, const QWidget* widget = nullptr) const override;

    ///
    /// \brief Runs base polishing, then keeps item-view icons in their original colours.
    /// \param widget Widget being polished.
    ///
    void polish(QWidget* widget) override;

    ///
    /// \brief Lays dialog buttons out without the Mac policy's gap around destructive buttons.
    /// \param hint Style hint being queried.
    /// \param option Style option carrying the hint context.
    /// \param widget Widget the hint applies to.
    /// \param returnData Optional extra return data.
    /// \return Resolved style hint value.
    ///
    int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr,
                  QStyleHintReturn* returnData = nullptr) const override;

    ///
    /// \brief Background colour for secondary buttons; primary roles defer to the base style.
    /// \param mouse Mouse interaction state.
    /// \param role Button colour role.
    /// \param widget Button widget (unused).
    /// \return Reference to the resolved background colour.
    ///
    QColor const& buttonBackgroundColor(oclero::qlementine::MouseState mouse,
                                        oclero::qlementine::ColorRole role,
                                        const QWidget* widget = nullptr) const override;

    ///
    /// \brief Foreground colour for secondary buttons; primary roles defer to the base style.
    /// \param mouse Mouse interaction state.
    /// \param role Button colour role.
    /// \param widget Button widget (unused).
    /// \return Reference to the resolved foreground colour.
    ///
    QColor const& buttonForegroundColor(oclero::qlementine::MouseState mouse,
                                        oclero::qlementine::ColorRole role,
                                        const QWidget* widget = nullptr) const override;

    ///
    /// \brief Icon tint for secondary roles; primary roles defer to the base style.
    /// \param mouse Mouse interaction state.
    /// \param role Icon colour role.
    /// \return Reference to the resolved icon colour.
    ///
    QColor const& iconForegroundColor(oclero::qlementine::MouseState mouse,
                                      oclero::qlementine::ColorRole role) const override;

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
    QColor listItemBackgroundColor(oclero::qlementine::MouseState mouse,
                                   oclero::qlementine::SelectionState selected,
                                   oclero::qlementine::FocusState focus,
                                   oclero::qlementine::ActiveState active,
                                   const QModelIndex& index,
                                   const QWidget* widget = nullptr) const override;

    ///
    /// \brief Text colour for a list/tree item.
    /// \param mouse Mouse interaction state.
    /// \param selected Selection state (unused).
    /// \param focus Focus state (unused).
    /// \param active Active state (unused).
    /// \return Reference to the resolved item text colour.
    ///
    QColor const& listItemForegroundColor(oclero::qlementine::MouseState mouse,
                                          oclero::qlementine::SelectionState selected,
                                          oclero::qlementine::FocusState focus,
                                          oclero::qlementine::ActiveState active) const override;

    ///
    /// \brief Colour of splitter handles, brightening on hover/press.
    /// \param mouse Mouse interaction state.
    /// \return Reference to the resolved splitter colour.
    ///
    QColor const& splitterColor(oclero::qlementine::MouseState mouse) const override;

    ///
    /// \brief Background colour of a tab; the selected tab matches the canvas.
    /// \param mouse Mouse interaction state.
    /// \param selected Selection state.
    /// \return Reference to the resolved tab background colour.
    ///
    QColor const& tabBackgroundColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::SelectionState selected) const override;

    ///
    /// \brief Background colour of the tab bar behind the tabs.
    /// \param mouse Mouse interaction state.
    /// \return Reference to the resolved tab-bar background colour.
    ///
    QColor const& tabBarBackgroundColor(oclero::qlementine::MouseState mouse) const override;

    ///
    /// \brief Text colour of a tab label.
    /// \param mouse Mouse interaction state.
    /// \param selected Selection state (unused).
    /// \return Reference to the resolved tab text colour.
    ///
    QColor const& tabForegroundColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::SelectionState selected) const override;

    ///
    /// \brief Background colour of a table header section.
    /// \param mouse Mouse interaction state.
    /// \param checked Sort/check state (unused).
    /// \return Reference to the resolved header background colour.
    ///
    QColor const& tableHeaderBgColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::CheckState checked) const override;

    ///
    /// \brief Text colour of a table header section.
    /// \param mouse Mouse interaction state.
    /// \param checked Sort/check state (unused).
    /// \return Reference to the resolved header text colour.
    ///
    QColor const& tableHeaderFgColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::CheckState checked) const override;

    ///
    /// \brief Colour of the grid lines between table cells.
    /// \return Reference to the resolved grid-line colour.
    ///
    QColor const& tableLineColor() const override;

    ///
    /// \brief Background colour of text input fields.
    /// \param mouse Mouse interaction state.
    /// \param status Validation status (unused).
    /// \return Reference to the resolved field background colour.
    ///
    QColor const& textFieldBackgroundColor(oclero::qlementine::MouseState mouse,
                                           oclero::qlementine::Status status) const override;

    ///
    /// \brief Border colour of text input fields; non-default statuses defer to the base style.
    /// \param mouse Mouse interaction state.
    /// \param focus Focus state.
    /// \param status Validation status.
    /// \return Reference to the resolved field border colour.
    ///
    QColor const& textFieldBorderColor(oclero::qlementine::MouseState mouse,
                                       oclero::qlementine::FocusState focus,
                                       oclero::qlementine::Status status) const override;

    ///
    /// \brief Background colour of toolbars.
    /// \return Reference to the resolved toolbar background colour.
    ///
    QColor const& toolBarBackgroundColor() const override;

    ///
    /// \brief Border colour of toolbars.
    /// \return Reference to the resolved toolbar border colour.
    ///
    QColor const& toolBarBorderColor() const override;

    ///
    /// \brief Text colour of tooltips.
    /// \return Reference to the resolved tooltip text colour.
    ///
    QColor const& toolTipForegroundColor() const override;

    ///
    /// \brief Background colour for secondary tool buttons; primary roles defer to the base style.
    /// \param mouse Mouse interaction state.
    /// \param role Tool-button colour role.
    /// \return Reference to the resolved background colour.
    ///
    QColor const& toolButtonBackgroundColor(oclero::qlementine::MouseState mouse,
                                            oclero::qlementine::ColorRole role) const override;

    ///
    /// \brief Foreground colour for secondary tool buttons; primary roles defer to the base style.
    /// \param mouse Mouse interaction state.
    /// \param role Tool-button colour role.
    /// \return Reference to the resolved foreground colour.
    ///
    QColor const& toolButtonForegroundColor(oclero::qlementine::MouseState mouse,
                                            oclero::qlementine::ColorRole role) const override;

private:
    void updateTheme();
    bool isDarkMode() const;

    ///
    /// \brief Paints the macOS bezel (fill + border) behind an outlined tool button.
    /// \param option Style option carrying the tool-button state.
    /// \param painter Painter to draw with.
    ///
    void drawOutlinedToolButton(const QStyleOption* option, QPainter* painter) const;

    oclero::qlementine::Theme _lightTheme;
    oclero::qlementine::Theme _darkTheme;
};

#endif // HAVE_QLEMENTINE_APP_STYLE
#endif // MACAPPSTYLE_H
