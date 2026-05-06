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

#include "appicons.h"
#include "coloredpushbutton.h"

namespace {

///
/// \brief Checks whether a widget is rendered through the Fusion style.
/// \param widget
/// \return
///
bool isFusionStyle(const QWidget *widget)
{
    const QStyle *currentStyle = widget != nullptr ? widget->style() : QApplication::style();

    while (const auto *proxyStyle = qobject_cast<const QProxyStyle *>(currentStyle)) {
        const QStyle *baseStyle = proxyStyle->baseStyle();
        if (baseStyle == nullptr || baseStyle == currentStyle)
            break;

        currentStyle = baseStyle;
    }

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
    const QStyle *currentStyle = widget != nullptr ? widget->style() : QApplication::style();

    while (const auto *proxyStyle = qobject_cast<const QProxyStyle *>(currentStyle)) {
        const QStyle *baseStyle = proxyStyle->baseStyle();
        if (baseStyle == nullptr || baseStyle == currentStyle)
            break;

        currentStyle = baseStyle;
    }

    return currentStyle != nullptr
           && currentStyle->objectName().contains("windows", Qt::CaseInsensitive);
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
    const bool fusionStyle = isFusionStyle(this);
    const bool compactPaintRect = isWindowsStyle(this);

    QColor bg = _colors.base;
    if (down)
        bg = _colors.pressed;
    else if (hovered)
        bg = _colors.hover;
    else if (!enabled && fusionStyle)
        bg = palette().color(QPalette::Disabled, QPalette::Button);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF buttonRect = compactPaintRect
                                  ? rect().adjusted(1, 2, -1, -2)
                                  : rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(buttonRect, 4, 4);

    if (fusionStyle) {
        QLinearGradient gradient(buttonRect.topLeft(), buttonRect.bottomLeft());
        if (down) {
            gradient.setColorAt(0.0, bg.darker(112));
            gradient.setColorAt(1.0, bg.lighter(112));
        } else {
            gradient.setColorAt(0.0, bg.lighter(enabled ? 128 : 108));
            gradient.setColorAt(0.45, bg.lighter(enabled ? 112 : 104));
            gradient.setColorAt(1.0, bg.darker(enabled ? 108 : 100));
        }

        const QColor border = enabled ? bg.darker(135) : palette().color(QPalette::Disabled, QPalette::Mid);

        p.fillPath(path, gradient);
        p.setPen(QPen(border, 1));
        p.drawPath(path);
    } else {
        p.fillPath(path, bg);
    }

    if (hasFocus()) {
        if (fusionStyle) {
            QPainterPath focusPath;
            focusPath.addRoundedRect(buttonRect.adjusted(2, 2, -2, -2), 3, 3);
            p.setPen(QPen(bg.lighter(170), 1));
            p.drawPath(focusPath);
        } else {
            p.setPen(QPen(bg.lighter(160), 1.5));
            p.drawPath(path);
        }
    }

    p.setPen(enabled || !fusionStyle ? _colors.text : palette().color(QPalette::Disabled, QPalette::ButtonText));
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
