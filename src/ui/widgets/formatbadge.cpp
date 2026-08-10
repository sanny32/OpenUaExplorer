// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file formatbadge.cpp
/// \brief Implements the tile that names a value's encoding.
///

#include <QFontMetricsF>
#include <QPainter>

#include "appcolors.h"
#include "formatbadge.h"

namespace {

/// \brief Edge length of the tile, in pixels.
constexpr int BadgeSize = 40;
/// \brief Corner radius of the tile, in pixels.
constexpr qreal BadgeRadius = 10.0;
/// \brief Space kept between the text and the tile edge, in pixels.
constexpr int TextPadding = 5;

} // namespace

///
/// \brief Constructs an empty badge.
/// \param parent Parent widget.
///
FormatBadge::FormatBadge(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

///
/// \brief Returns the format name shown on the badge.
///
QString FormatBadge::text() const
{
    return _text;
}

///
/// \brief Sets the format name shown on the badge.
/// \param text Short format name; the badge stays blank when it is empty.
///
void FormatBadge::setText(const QString &text)
{
    if (_text == text)
        return;
    _text = text;
    update();
}

///
/// \brief Returns the preferred size of the badge.
///
QSize FormatBadge::sizeHint() const
{
    return QSize(BadgeSize, BadgeSize);
}

///
/// \brief Returns the smallest useful size of the badge.
///
QSize FormatBadge::minimumSizeHint() const
{
    return sizeHint();
}

///
/// \brief Paints the tile and the format name centred on it.
/// \param event Paint event.
///
/// A four-letter name such as "JPEG" is wider than the tile at the interface font size,
/// so the text is shrunk to the tile instead of the tile grown to the text.
///
void FormatBadge::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(AppColors::formatBadgeBackground());
    painter.drawRoundedRect(QRectF(rect()), BadgeRadius, BadgeRadius);

    if (_text.isEmpty())
        return;

    QFont badgeFont = font();
    badgeFont.setBold(true);
    const qreal available = width() - 2 * TextPadding;
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (QFontMetricsF(badgeFont).horizontalAdvance(_text) <= available)
            break;
        if (badgeFont.pointSizeF() > 0.0)
            badgeFont.setPointSizeF(badgeFont.pointSizeF() - 0.5);
        else
            badgeFont.setPixelSize(badgeFont.pixelSize() - 1);
    }

    painter.setFont(badgeFont);
    painter.setPen(AppColors::formatBadgeText());
    painter.drawText(rect(), Qt::AlignCenter, _text);
}
