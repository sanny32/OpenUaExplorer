// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file coloredpushbutton.cpp
/// \brief Implements a color-configurable push button.
///

#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>

#include "appicons.h"
#include "coloredpushbutton.h"

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

    const bool down = isDown() || isChecked();
    const bool hovered = opt.state & QStyle::State_MouseOver;

    QColor bg = _colors.base;
    if (down)
        bg = _colors.pressed;
    else if (hovered)
        bg = _colors.hover;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);

    p.fillPath(path, bg);

    if (hasFocus()) {
        p.setPen(QPen(bg.lighter(160), 1.5));
        p.drawPath(path);
    }

    p.setPen(_colors.text);
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
