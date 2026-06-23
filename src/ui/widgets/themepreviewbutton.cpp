// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themepreviewbutton.cpp
/// \brief Implements the theme-selection preview card.
///

#include "themepreviewbutton.h"

#include <QIcon>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionFocusRect>

namespace {

///
/// \brief Resolves the preview artwork for a mode.
/// \param mode Light, dark, or system.
/// \return Resource path of the matching window mockup.
///
QString previewResource(const QString &mode)
{
    if (mode == QLatin1String("light"))
        return QStringLiteral(":/icons/theme-preview-light.svg");
    if (mode == QLatin1String("dark"))
        return QStringLiteral(":/icons/theme-preview-dark.svg");
    return QStringLiteral(":/icons/theme-preview-system.svg");
}

///
/// \brief Renders the preview window mockup into the given bounds.
/// \param painter Painter targeting the card.
/// \param bounds Window bounds.
/// \param mode Light, dark, or system.
///
void paintPreviewWindow(QPainter &painter, const QRectF &bounds, const QString &mode)
{
    const qreal dpr = painter.device()->devicePixelRatioF();
    const QPixmap pixmap = QIcon(previewResource(mode)).pixmap((bounds.size() * dpr).toSize());
    painter.drawPixmap(bounds, pixmap, QRectF(pixmap.rect()));
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
    paintPreviewWindow(painter, previewRect, _previewMode);
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
