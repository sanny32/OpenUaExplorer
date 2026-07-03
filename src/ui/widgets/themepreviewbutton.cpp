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

#include "appicons.h"

namespace {

constexpr int kPreviewIconSide = 44;

///
/// \brief Resolves the toolbar icon name for a mode.
/// \param mode Light, dark, or system.
/// \return Theme-action icon resource name shared with the toolbar.
///
QString previewIconName(const QString &mode)
{
    if (mode == QLatin1String("light"))
        return QStringLiteral("theme-light");
    if (mode == QLatin1String("dark"))
        return QStringLiteral("theme-dark");
    return QStringLiteral("theme-system");
}

///
/// \brief Renders the toolbar theme icon centred within the given bounds.
/// \param painter Painter targeting the card.
/// \param bounds Preview bounds.
/// \param mode Light, dark, or system.
///
void paintPreviewIcon(QPainter &painter, const QRectF &bounds, const QString &mode)
{
    QRect target(0, 0, kPreviewIconSide, kPreviewIconSide);
    target.moveCenter(bounds.toRect().center());
    AppIcons::themed(previewIconName(mode)).paint(&painter, target, Qt::AlignCenter);
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
    return QSize(150, 118);
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

    const QRectF previewRect = cardRect.adjusted(18.0, 12.0, -18.0, -34.0);
    paintPreviewIcon(painter, previewRect, _previewMode);

    QStyleOptionButton radio;
    radio.initFrom(this);
    radio.state |= isChecked() ? QStyle::State_On : QStyle::State_Off;
    radio.rect = QRect(14, height() - 26, 16, 16);
    style()->drawPrimitive(QStyle::PE_IndicatorRadioButton, &radio, &painter, this);

    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(QRect(36, height() - 30, width() - 46, 24),
                     Qt::AlignVCenter | Qt::AlignLeft, text());

    if (hasFocus()) {
        QStyleOptionFocusRect focus;
        focus.initFrom(this);
        focus.rect = rect().adjusted(3, 3, -3, -3);
        focus.backgroundColor = palette().color(QPalette::Base);
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focus, &painter, this);
    }
}
