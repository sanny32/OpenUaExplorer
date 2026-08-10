// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file imagecanvas.cpp
/// \brief Implements the widget that shows a picture over a transparency chequerboard.
///

#include <QPainter>

#include "appcolors.h"
#include "imagecanvas.h"

namespace {

/// \brief Edge length of one chequerboard square, in pixels.
constexpr int CheckerSize = 10;

///
/// \brief Builds the two-by-two tile the chequerboard is brushed with.
/// \return Tile in the colours of the active theme.
///
QPixmap checkerTile()
{
    QPixmap tile(2 * CheckerSize, 2 * CheckerSize);
    tile.fill(AppColors::transparencyCheckerLight());

    QPainter painter(&tile);
    const QColor shade = AppColors::transparencyCheckerDark();
    painter.fillRect(0, 0, CheckerSize, CheckerSize, shade);
    painter.fillRect(CheckerSize, CheckerSize, CheckerSize, CheckerSize, shade);
    return tile;
}

} // namespace

///
/// \brief Constructs an empty canvas.
/// \param parent Parent widget.
///
ImageCanvas::ImageCanvas(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

///
/// \brief Returns the picture currently drawn.
///
QPixmap ImageCanvas::pixmap() const
{
    return _pixmap;
}

///
/// \brief Sets the picture to draw.
/// \param pixmap Picture at the size it should appear; a null pixmap clears the canvas.
///
void ImageCanvas::setPixmap(const QPixmap &pixmap)
{
    _pixmap = pixmap;
    updateGeometry();
    update();
}

///
/// \brief Returns the message shown while there is no picture.
///
QString ImageCanvas::placeholderText() const
{
    return _placeholder;
}

///
/// \brief Sets the message shown while there is no picture.
/// \param text Message to show.
///
void ImageCanvas::setPlaceholderText(const QString &text)
{
    if (_placeholder == text)
        return;
    _placeholder = text;
    update();
}

///
/// \brief Returns the size the picture asks the scroll area for.
///
QSize ImageCanvas::sizeHint() const
{
    return _pixmap.isNull() ? QSize(0, 0) : _pixmap.size();
}

///
/// \brief Returns the size below which the scroll area has to start scrolling.
///
QSize ImageCanvas::minimumSizeHint() const
{
    return sizeHint();
}

///
/// \brief Paints the chequerboard and the picture centred on it.
/// \param event Paint event.
///
void ImageCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    if (_pixmap.isNull()) {
        painter.setPen(AppColors::hint());
        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, _placeholder);
        return;
    }

    painter.fillRect(rect(), QBrush(checkerTile()));
    const QPoint origin((width() - _pixmap.width()) / 2, (height() - _pixmap.height()) / 2);
    painter.drawPixmap(origin, _pixmap);
}
