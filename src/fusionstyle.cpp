// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file fusionstyle.cpp
/// \brief Implements Fusion style palette helpers.
///

#include <QColor>
#include <QPalette>

#include "fusionstyle.h"

///
/// \brief FusionStyle::palette
/// \param darkAppearance True to create a dark appearance palette.
/// \return Palette configured for the Fusion style.
///
QPalette FusionStyle::palette(bool darkAppearance)
{
    const QColor windowText = darkAppearance ? QColor(240, 240, 240) : Qt::black;
    const QColor backGround = darkAppearance ? QColor(50, 50, 50) : QColor(239, 239, 239);
    const QColor light = backGround.lighter(150);
    const QColor mid = darkAppearance ? backGround.lighter(150) : backGround.darker(130);
    const QColor midLight = mid.lighter(110);
    const QColor base = darkAppearance ? backGround.darker(140) : Qt::white;
    const QColor disabledBase(backGround);
    const QColor dark = darkAppearance ? backGround.lighter(175) : backGround.darker(150);
    const QColor darkDisabled = QColor(209, 209, 209).darker(110);
    const QColor text = darkAppearance ? windowText : Qt::black;
    const QColor highlight = QColor(48, 140, 198);
    const QColor highlightedText = darkAppearance ? windowText : Qt::white;
    const QColor disabledText = darkAppearance ? QColor(130, 130, 130) : QColor(190, 190, 190);
    const QColor button = backGround;
    const QColor shadow = dark.darker(135);
    const QColor disabledShadow = shadow.lighter(150);
    const QColor disabledHighlight(145, 145, 145);
    QColor placeholder = text;
    placeholder.setAlpha(128);

    QPalette p(windowText, backGround, light, dark, mid, text, base);
    p.setBrush(QPalette::Midlight,        midLight);
    p.setBrush(QPalette::Button,          button);
    p.setBrush(QPalette::Shadow,          shadow);
    p.setBrush(QPalette::HighlightedText, highlightedText);
    p.setBrush(QPalette::Disabled, QPalette::Text,       disabledText);
    p.setBrush(QPalette::Disabled, QPalette::WindowText, disabledText);
    p.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledText);
    p.setBrush(QPalette::Disabled, QPalette::Base,       disabledBase);
    p.setBrush(QPalette::Disabled, QPalette::Dark,       darkDisabled);
    p.setBrush(QPalette::Disabled, QPalette::Shadow,     disabledShadow);
    p.setBrush(QPalette::Active,   QPalette::Highlight,  highlight);
    p.setBrush(QPalette::Inactive, QPalette::Highlight,  highlight);
    p.setBrush(QPalette::Disabled, QPalette::Highlight,  disabledHighlight);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    p.setBrush(QPalette::Active,   QPalette::Accent,     highlight);
    p.setBrush(QPalette::Inactive, QPalette::Accent,     highlight);
    p.setBrush(QPalette::Disabled, QPalette::Accent,     disabledHighlight);
#endif
    p.setBrush(QPalette::PlaceholderText, placeholder);
    if (darkAppearance) p.setBrush(QPalette::Link, highlight);

    return p;
}
