#include <QColor>
#include <QPalette>

#include "apptheme.h"

///
/// \brief AppTheme::lightPalette
/// \return
///
QPalette AppTheme::lightPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window,          QColor(0xf0, 0xf0, 0xf0));
    palette.setColor(QPalette::WindowText,      QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Base,            QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::AlternateBase,   QColor(0xf7, 0xf7, 0xf7));
    palette.setColor(QPalette::Text,            QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Button,          QColor(0xf8, 0xf8, 0xf8));
    palette.setColor(QPalette::ButtonText,      QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Highlight,       QColor(0x00, 0x78, 0xd7));
    palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::ToolTipBase,     QColor(0xff, 0xff, 0xe1));
    palette.setColor(QPalette::ToolTipText,     QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Link,            QColor(0x00, 0x00, 0xff));
    palette.setColor(QPalette::Midlight,        QColor(0xe3, 0xe3, 0xe3));
    palette.setColor(QPalette::Mid,             QColor(0xa0, 0xa0, 0xa0));
    palette.setColor(QPalette::Dark,            QColor(0xa0, 0xa0, 0xa0));
    return palette;
}

///
/// \brief AppTheme::darkPalette
/// \return
///
QPalette AppTheme::darkPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window,          QColor(0x2b, 0x2b, 0x2b));
    palette.setColor(QPalette::WindowText,      QColor(0xe2, 0xe8, 0xf0));
    palette.setColor(QPalette::Base,            QColor(0x1e, 0x1e, 0x1e));
    palette.setColor(QPalette::AlternateBase,   QColor(0x31, 0x31, 0x31));
    palette.setColor(QPalette::Text,            QColor(0xe2, 0xe8, 0xf0));
    palette.setColor(QPalette::PlaceholderText, QColor(0x8f, 0x98, 0xa3));
    palette.setColor(QPalette::Button,          QColor(0x3c, 0x3c, 0x3c));
    palette.setColor(QPalette::ButtonText,      QColor(0xe2, 0xe8, 0xf0));
    palette.setColor(QPalette::BrightText,      QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::Highlight,       QColor(0x26, 0x4f, 0x78));
    palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::ToolTipBase,     QColor(0x3c, 0x3c, 0x3c));
    palette.setColor(QPalette::ToolTipText,     QColor(0xe2, 0xe8, 0xf0));
    palette.setColor(QPalette::Link,            QColor(0x58, 0xa6, 0xff));
    palette.setColor(QPalette::Midlight,        QColor(0x44, 0x44, 0x44));
    palette.setColor(QPalette::Mid,             QColor(0x38, 0x38, 0x38));
    palette.setColor(QPalette::Dark,            QColor(0x25, 0x25, 0x25));
    return palette;
}

///
/// \brief AppTheme::systemPalette
/// \return
///
QPalette AppTheme::systemPalette()
{
    const QPalette system = QPalette();
    const bool isDark = system.color(QPalette::Window).lightness() < 128;
    return isDark ? darkPalette() : lightPalette();
}
