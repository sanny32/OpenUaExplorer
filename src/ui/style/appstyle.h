// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QProxyStyle>
#include <QSize>
#include <QString>

///
/// \brief Application proxy style that tightens standard Qt control metrics.
///
class AppStyle : public QProxyStyle
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the proxy style around the current application style.
    /// \param parent Parent object.
    ///
    explicit AppStyle(QObject *parent);

    ///
    /// \brief Constructs the proxy style around the named base style.
    /// \param baseStyleName Base style name to use, or an empty string to wrap the current style.
    ///
    explicit AppStyle(const QString &baseStyleName = QString());

    ///
    /// \brief Returns the geometry of a sub-element, adding padding to headers and item text.
    /// \param element Sub-element to locate.
    /// \param option Style option carrying the element geometry.
    /// \param widget Widget the element belongs to.
    /// \return Adjusted sub-element rectangle.
    ///
    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget = nullptr) const override;

    ///
    /// \brief Enforces minimum heights and widths for selected control types.
    /// \param type Contents type being measured.
    /// \param option Style option carrying the element state.
    /// \param contentsSize Size requested by the contents.
    /// \param widget Widget the element belongs to.
    /// \return Size clamped to the style's minimum dimensions.
    ///
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &contentsSize,
                           const QWidget *widget = nullptr) const override;

    ///
    /// \brief Draws a control element, forcing highlighted text colour on the attributes tree.
    /// \param element Control element to render.
    /// \param option Style option carrying the element state.
    /// \param painter Painter to draw with.
    /// \param widget Widget the element belongs to.
    ///
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget = nullptr) const override;

    /// \brief Minimum height used for compact controls.
    static constexpr int controlMinHeight = 30;
    /// \brief Minimum push-button width.
    static constexpr int pushButtonMinWidth = 80;
    /// \brief Minimum menu item height.
    static constexpr int menuItemMinHeight = 28;
    /// \brief Minimum menu-bar item height.
    static constexpr int menuBarItemMinHeight = 30;
    /// \brief Horizontal text padding added by the style.
    static constexpr int textHMargin = 6;
    /// \brief Vertical text padding added by the style.
    static constexpr int textVMargin = 1;

    ///
    /// \brief Resolves the innermost concrete base style.
    /// \return The innermost non-proxy base style of the application style stack.
    ///
    static const QStyle *baseStyle();

    ///
    /// \brief Reports whether the active base style is the Fusion style.
    /// \return True if the application base style is Fusion.
    ///
    static bool isFusionStyle();
};
