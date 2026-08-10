// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedlabel.cpp
/// \brief Implements a label that elides its text to the width it is given.
///

#include <QPainter>
#include <QStyle>

#include "elidedlabel.h"

///
/// \brief Constructs an empty elided label.
/// \param parent Parent widget.
///
ElidedLabel::ElidedLabel(QWidget *parent)
    : QLabel(parent)
{
}

///
/// \brief Reports a minimum width of zero so the text never widens the window.
/// \return Minimum size with the label's natural height.
///
QSize ElidedLabel::minimumSizeHint() const
{
    return QSize(0, QLabel::minimumSizeHint().height());
}

///
/// \brief Paints the text elided to the width the layout granted.
/// \param event Paint event being handled.
///
void ElidedLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    const QRect content = contentsRect();
    const QString elided =
        fontMetrics().elidedText(text(), Qt::ElideRight, content.width());
    style()->drawItemText(&painter, content, alignment() | Qt::TextSingleLine, palette(),
                          isEnabled(), elided, foregroundRole());
}
