// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themepreviewbutton.cpp
/// \brief Implements the theme-selection preview card.
///

#include "themepreviewbutton.h"

#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionFocusRect>

namespace {

///
/// \brief Paints one half of the miniature application window.
/// \param painter Painter targeting the card.
/// \param bounds Half-window bounds.
/// \param dark Whether to use the dark preview palette.
///
void paintPreviewHalf(QPainter &painter, const QRectF &bounds, bool dark)
{
    const QColor window = dark ? QColor(38, 38, 38) : QColor(250, 250, 250);
    const QColor panel = dark ? QColor(48, 48, 48) : QColor(239, 239, 239);
    const QColor control = dark ? QColor(67, 67, 67) : QColor(218, 218, 218);
    const qreal sidebarWidth = bounds.width() * 0.38;

    painter.fillRect(bounds, window);
    painter.fillRect(QRectF(bounds.left(), bounds.top(), sidebarWidth, bounds.height()), panel);

    const qreal margin = qMax<qreal>(5.0, bounds.height() * 0.08);
    const qreal rowHeight = qMax<qreal>(7.0, bounds.height() * 0.14);
    for (int row = 0; row < 3; ++row) {
        const qreal y = bounds.top() + margin + row * (rowHeight + margin);
        painter.fillRect(QRectF(bounds.left() + margin, y,
                                qMax<qreal>(4.0, sidebarWidth - 2 * margin), rowHeight), control);
        painter.fillRect(QRectF(bounds.left() + sidebarWidth + margin, y,
                                qMax<qreal>(4.0, bounds.width() - sidebarWidth - 2 * margin),
                                rowHeight), control);
    }
}

}

///
/// \brief Constructs a theme preview card.
/// \param parent Parent widget.
///
ThemePreviewButton::ThemePreviewButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setAutoExclusive(true);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

///
/// \brief Returns the preview mode name.
/// \return Light, dark, or system.
///
QString ThemePreviewButton::previewMode() const
{
    return _previewMode;
}

///
/// \brief Selects which colour scheme the card previews.
/// \param mode Light, dark, or system.
///
void ThemePreviewButton::setPreviewMode(const QString &mode)
{
    const QString normalized = mode.toLower();
    if (_previewMode == normalized)
        return;

    _previewMode = normalized;
    update();
}

///
/// \brief Returns the card's preferred size.
/// \return Size suitable for the preview and caption.
///
QSize ThemePreviewButton::sizeHint() const
{
    return QSize(190, 150);
}

///
/// \brief Paints the card, miniature window, and radio-style caption.
/// \param event Paint event.
///
void ThemePreviewButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF cardRect = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    const QColor border = isChecked() ? palette().color(QPalette::Highlight)
                                      : palette().color(QPalette::Mid);
    painter.setPen(QPen(border, isChecked() ? 2.0 : 1.0));
    painter.setBrush(palette().color(QPalette::Base));
    painter.drawRoundedRect(cardRect, 5.0, 5.0);

    const QRectF previewRect = cardRect.adjusted(22.0, 14.0, -22.0, -42.0);
    painter.save();
    painter.setClipRect(previewRect);
    if (_previewMode == QLatin1String("system")) {
        QRectF lightHalf = previewRect;
        lightHalf.setWidth(previewRect.width() / 2.0);
        QRectF darkHalf = previewRect;
        darkHalf.setLeft(lightHalf.right());
        paintPreviewHalf(painter, lightHalf, false);
        paintPreviewHalf(painter, darkHalf, true);
    } else {
        paintPreviewHalf(painter, previewRect, _previewMode == QLatin1String("dark"));
    }
    painter.restore();
    painter.setPen(palette().color(QPalette::Mid));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(previewRect, 3.0, 3.0);

    QStyleOptionButton radio;
    radio.initFrom(this);
    radio.state |= isChecked() ? QStyle::State_On : QStyle::State_Off;
    radio.rect = QRect(16, height() - 30, 18, 18);
    style()->drawPrimitive(QStyle::PE_IndicatorRadioButton, &radio, &painter, this);

    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(QRect(42, height() - 34, width() - 52, 26),
                     Qt::AlignVCenter | Qt::AlignLeft, text());

    if (hasFocus()) {
        QStyleOptionFocusRect focus;
        focus.initFrom(this);
        focus.rect = rect().adjusted(3, 3, -3, -3);
        focus.backgroundColor = palette().color(QPalette::Base);
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focus, &painter, this);
    }
}
