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
/// \brief Tells whether the text last painted did not fit the width the layout granted.
/// \return True when the painted text was cut short.
///
bool ElidedLabel::isElided() const
{
    return _elided;
}

///
/// \brief Reports room for the elide mark alone, so the text never widens the window.
/// \return Minimum size with the label's natural height.
///
/// Keeping the mark's width means a field squeezed by its neighbours still shows that it has
/// a value to reveal, instead of collapsing to nothing.
///
QSize ElidedLabel::minimumSizeHint() const
{
    const QSize hint = QLabel::minimumSizeHint();
    const int mark = text().isEmpty()
        ? 0
        : fontMetrics().horizontalAdvance(QStringLiteral("…"));
    return QSize(qMin(mark, hint.width()), hint.height());
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

    if (_elided != (elided != text())) {
        _elided = elided != text();
        emit elisionChanged(_elided);
    }
}
