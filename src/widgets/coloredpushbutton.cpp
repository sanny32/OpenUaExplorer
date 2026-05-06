// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file coloredpushbutton.cpp
/// \brief Implements a color-configurable push button.
///

#include <QApplication>
#include <QEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QStyleOption>
#include <qdrawutil.h>

#include "appicons.h"
#include "coloredpushbutton.h"

namespace {

///
/// \brief Style-specific button drawing path.
///
enum class ButtonStyle {
    Fusion,
    Windows11,
    Windows,
    Generic,
};

///
/// \brief Returns the unwrapped application style for a widget.
/// \param widget
/// \return
///
const QStyle *baseStyle(const QWidget *widget)
{
    const QStyle *currentStyle = widget != nullptr ? widget->style() : QApplication::style();

    while (const auto *proxyStyle = qobject_cast<const QProxyStyle *>(currentStyle)) {
        const QStyle *baseStyle = proxyStyle->baseStyle();
        if (baseStyle == nullptr || baseStyle == currentStyle)
            break;

        currentStyle = baseStyle;
    }

    return currentStyle;
}

///
/// \brief Checks whether a widget is rendered through the Fusion style.
/// \param widget
/// \return
///
bool isFusionStyle(const QWidget *widget)
{
    const QStyle *currentStyle = baseStyle(widget);
    return currentStyle != nullptr
           && currentStyle->objectName().compare("fusion", Qt::CaseInsensitive) == 0;
}

///
/// \brief Checks whether a widget is rendered through a Windows style.
/// \param widget
/// \return
///
bool isWindowsStyle(const QWidget *widget)
{
    const QStyle *currentStyle = baseStyle(widget);
    return currentStyle != nullptr
           && currentStyle->objectName().contains("windows", Qt::CaseInsensitive);
}

///
/// \brief Checks whether a widget is rendered through the Windows 11 style.
/// \param widget
/// \return
///
bool isWindows11Style(const QWidget *widget)
{
    const QStyle *currentStyle = baseStyle(widget);
    return currentStyle != nullptr
           && currentStyle->objectName().compare("windows11", Qt::CaseInsensitive) == 0;
}

///
/// \brief Resolves the style-specific drawing path for a widget.
/// \param widget
/// \return
///
ButtonStyle buttonStyle(const QWidget *widget)
{
    if (isFusionStyle(widget))
        return ButtonStyle::Fusion;
    if (isWindows11Style(widget))
        return ButtonStyle::Windows11;
    if (isWindowsStyle(widget))
        return ButtonStyle::Windows;

    return ButtonStyle::Generic;
}

namespace Fusion {

///
/// \brief Blends two colors using the same weighting convention as Qt styles.
/// \param colorA
/// \param colorB
/// \param factor
/// \return
///
QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor)
{
    constexpr int maxFactor = 100;
    QColor color = colorA;
    color.setRed((colorA.red() * factor) / maxFactor
                 + (colorB.red() * (maxFactor - factor)) / maxFactor);
    color.setGreen((colorA.green() * factor) / maxFactor
                   + (colorB.green() * (maxFactor - factor)) / maxFactor);
    color.setBlue((colorA.blue() * factor) / maxFactor
                  + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return color;
}

///
/// \brief Creates a Fusion-style vertical gradient from a base color.
/// \param rect
/// \param baseColor
/// \return
///
QLinearGradient fusionGradient(const QRectF &rect, const QColor &baseColor)
{
    QLinearGradient gradient(rect.center().x(), rect.top(), rect.center().x(), rect.bottom());
    gradient.setColorAt(0.0, baseColor.lighter(124));
    gradient.setColorAt(1.0, baseColor.lighter(102));
    return gradient;
}

///
/// \brief Returns the Fusion-style highlighted outline color.
/// \param palette
/// \return
///
QColor fusionHighlightedOutline(const QPalette &palette)
{
    QColor outline = palette.color(QPalette::Highlight).darker(125);
    if (outline.value() > 160)
        outline.setHsl(outline.hue(), outline.saturation(), 160);

    return outline;
}

///
/// \brief Draws a colored push button using Fusion button geometry.
/// \param painter
/// \param button
/// \param option
/// \param bg
/// \param enabled
/// \param down
/// \param hovered
///
void drawButton(QPainter *painter, const QPushButton *button, const QStyleOption &option,
                const QColor &bg, bool enabled, bool down, bool hovered)
{
    const bool defaultButton = button->isDefault() && enabled;
    const bool focusOutline = button->hasFocus()
                              && (option.state & QStyle::State_KeyboardFocusChange);
    const QRectF fusionRect = QRectF(button->rect().adjusted(0, 1, -1, 0));
    QColor buttonColor = bg;
    const QColor highlightedOutline = fusionHighlightedOutline(button->palette());
    const QColor outline = focusOutline || defaultButton
                               ? highlightedOutline
                               : button->palette().color(QPalette::Window).darker(140);

    if (defaultButton)
        buttonColor = mergedColors(buttonColor, highlightedOutline.lighter(130), 90);

    painter->save();
    painter->translate(0.5, -0.5);
    painter->setPen(Qt::transparent);
    painter->setBrush(down ? QBrush(buttonColor.darker(110))
                           : QBrush(fusionGradient(fusionRect, hovered ? buttonColor
                                                                       : buttonColor.darker(104))));
    painter->drawRoundedRect(fusionRect, 2.0, 2.0);

    painter->setBrush(Qt::NoBrush);
    painter->setPen(enabled ? QPen(outline) : QPen(outline.lighter(115)));
    painter->drawRoundedRect(fusionRect, 2.0, 2.0);

    painter->setPen(QColor(255, 255, 255, 30));
    painter->drawRoundedRect(fusionRect.adjusted(1, 1, -1, -1), 2.0, 2.0);
    painter->restore();
}

}

///
/// \brief Selects the background color for the current button state.
/// \param colors
/// \param palette
/// \param enabled
/// \param down
/// \param hovered
/// \param style
/// \return
///
QColor stateBackgroundColor(const ColoredPushButton::Colors &colors, const QPalette &palette,
                            bool enabled, bool down, bool hovered, ButtonStyle style)
{
    QColor bg = colors.base;
    if (down)
        bg = colors.pressed;
    else if (hovered)
        bg = colors.hover;
    else if (!enabled && style == ButtonStyle::Fusion)
        bg = palette.color(QPalette::Disabled, QPalette::Button);

    return bg;
}

namespace Generic {

///
/// \brief Draws a colored push button using the generic flat fill.
/// \param painter
/// \param button
/// \param bg
///
void drawButton(QPainter *painter, const QPushButton *button, const QColor &bg)
{
    const QRectF buttonRect = button->rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(buttonRect, 4, 4);

    painter->fillPath(path, bg);

    if (button->hasFocus()) {
        painter->setPen(QPen(bg.lighter(160), 1.5));
        painter->drawPath(path);
    }
}

}

namespace Windows {

///
/// \brief Draws a colored push button using Qt's Windows button frame helper.
/// \param painter
/// \param button
/// \param bg
/// \param down
///
void drawButton(QPainter *painter, const QPushButton *button, const QColor &bg, bool down)
{
    const QBrush fill(bg);
    qDrawWinButton(painter, button->rect(), button->palette(), down, &fill);
}

}

namespace Windows11 {

///
/// \brief Draws a colored push button using Windows 11 button geometry.
/// \param painter
/// \param button
/// \param bg
/// \param enabled
/// \param down
/// \param hovered
///
void drawButton(QPainter *painter, const QPushButton *button, const QColor &bg,
                bool enabled, bool down, bool hovered)
{
    constexpr int radius = 4;
    const QRectF fillRect = QRectF(button->rect().marginsRemoved(QMargins(2, 2, 2, 2)));
    QColor fillColor = bg;
    if (down)
        fillColor = bg.darker(108);
    else if (hovered)
        fillColor = bg.lighter(108);

    painter->setPen(Qt::NoPen);
    painter->setBrush(fillColor);
    painter->drawRoundedRect(fillRect, radius, radius);

    QRectF borderRect = fillRect.adjusted(0.5, 0.5, -0.5, -0.5);
    const QColor borderColor = enabled ? bg.darker(125)
                                       : button->palette().color(QPalette::Disabled, QPalette::Mid);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(borderColor);
    painter->drawRoundedRect(borderRect, radius, radius);
}

}

}

///
/// \brief ColoredPushButton::ColoredPushButton
/// \param parent
///
ColoredPushButton::ColoredPushButton(QWidget *parent)
    : QPushButton(parent)
{
}

///
/// \brief ColoredPushButton::setColors
/// \param colors
///
void ColoredPushButton::setColors(const Colors &colors)
{
    _lightColors = colors;
    _darkColors  = darkColorsFromLight(colors);
    _colored     = true;

    refreshColors();
}

///
/// \brief ColoredPushButton::clearColors
///
void ColoredPushButton::clearColors()
{
    _colored = false;
    setPalette(QApplication::palette());
    update();
}

///
/// \brief ColoredPushButton::changeEvent
/// \param event
///
void ColoredPushButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    if (_colored
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshColors();
    }
}

///
/// \brief ColoredPushButton::paintEvent
/// \param event
///
void ColoredPushButton::paintEvent(QPaintEvent *event)
{
    if (!_colored) {
        QPushButton::paintEvent(event);
        return;
    }

    QStyleOption opt;
    opt.initFrom(this);

    const bool enabled = opt.state & QStyle::State_Enabled;
    const bool down = enabled && (isDown() || isChecked());
    const bool hovered = enabled && (opt.state & QStyle::State_MouseOver);
    const ButtonStyle style = buttonStyle(this);

    const QColor bg = stateBackgroundColor(_colors, palette(), enabled, down, hovered, style);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    switch (style) {
    case ButtonStyle::Fusion:
        Fusion::drawButton(&p, this, opt, bg, enabled, down, hovered);
        break;
    case ButtonStyle::Windows11:
        Windows11::drawButton(&p, this, bg, enabled, down, hovered);
        break;
    case ButtonStyle::Windows:
        Windows::drawButton(&p, this, bg, down);
        break;
    case ButtonStyle::Generic:
        Generic::drawButton(&p, this, bg);
        break;
    }

    p.setPen(enabled || style != ButtonStyle::Fusion
                 ? _colors.text
                 : palette().color(QPalette::Disabled, QPalette::ButtonText));
    p.drawText(rect(), Qt::AlignCenter, text());
}

///
/// \brief ColoredPushButton::darkColorsFromLight
/// \param lightColors
/// \return
///
ColoredPushButton::Colors ColoredPushButton::darkColorsFromLight(const Colors &lightColors)
{
    return {
        lightColors.base.lighter(125),
        lightColors.hover.lighter(120),
        lightColors.pressed.lighter(125),
        lightColors.text,
    };
}

///
/// \brief ColoredPushButton::applyColors
/// \param colors
///
void ColoredPushButton::applyColors(const Colors &colors)
{
    _colors = colors;
    _colored = true;

    QPalette pal = palette();
    pal.setColor(QPalette::Button, colors.base);
    pal.setColor(QPalette::ButtonText, colors.text);
    setPalette(pal);

    update();
}

///
/// \brief ColoredPushButton::refreshColors
///
void ColoredPushButton::refreshColors()
{
    applyColors(AppIcons::isDarkTheme() ? _darkColors : _lightColors);
}
