// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedtextdelegate.cpp
/// \brief Implements the delegate that offers a viewer button for truncated cells.
///

#include <QAbstractItemView>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

#include "appicons.h"
#include "elidedtextdelegate.h"
#include "models/valueroles.h"

namespace {

/// \brief Edge length of the square button, in pixels.
constexpr int ButtonSize = 20;
/// \brief Edge length of the glyph drawn inside the button, in pixels.
constexpr int IconSize = 12;
/// \brief Gap kept between the elided text and the button, in pixels.
constexpr int TextGap = 6;
/// \brief Gap kept between the button and the trailing edge of the cell, in pixels.
constexpr int EdgeGap = 2;
/// \brief Corner radius of the button, in pixels.
constexpr int ButtonRadius = 4;

///
/// \brief Returns the palette group a cell's state calls for.
/// \param state Style state of the cell.
/// \return Colour group to read text colours from.
///
QPalette::ColorGroup colorGroup(QStyle::State state)
{
    if (!state.testFlag(QStyle::State_Enabled))
        return QPalette::Disabled;
    return state.testFlag(QStyle::State_Active) ? QPalette::Normal : QPalette::Inactive;
}

///
/// \brief Shifts a colour away from the background of the active theme.
/// \param color Colour to shift.
/// \param steps How far to shift it.
/// \return Darker colour on a light theme, lighter colour on a dark one.
///
QColor shade(const QColor &color, int steps)
{
    const int amount = 100 + steps * 10;
    return color.lightnessF() < 0.5 ? color.lighter(amount) : color.darker(amount);
}

} // namespace

///
/// \brief Constructs the delegate and starts tracking the view's viewport.
/// \param view View the delegate paints into.
///
ElidedTextDelegate::ElidedTextDelegate(QAbstractItemView *view)
    : QStyledItemDelegate(view)
    , _view(view)
{
    if (!_view)
        return;
    // The view reports hovered rows, not hovered points, so the button needs the
    // pointer position of its own to light up only under the cursor.
    _view->viewport()->setMouseTracking(true);
    _view->viewport()->installEventFilter(this);
}

///
/// \brief Paints the cell and, when it cannot show its value in full, the viewer button.
/// \param painter Painter to draw with.
/// \param option Style options for the cell.
/// \param index Model index being painted.
///
void ElidedTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    const QStyleOptionViewItem opt = cellOption(option, index);
    if (!needsButton(opt, index)) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    // The style still owns the background, the selection, and the focus ring, so the
    // cell is drawn whole with its text stripped out; only the shortened text and the
    // button are ours to place.
    QStyleOptionViewItem background(opt);
    background.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &background, painter, background.widget);

    const QRect button = buttonRect(opt.rect);
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    textRect.setRight(button.left() - TextGap);

    if (textRect.width() > 0) {
        const QPalette::ColorRole role = opt.state.testFlag(QStyle::State_Selected)
                                             ? QPalette::HighlightedText
                                             : QPalette::Text;
        painter->save();
        painter->setPen(opt.palette.color(colorGroup(opt.state), role));
        painter->drawText(textRect, static_cast<int>(opt.displayAlignment),
                          opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, textRect.width()));
        painter->restore();
    }

    QColor fill = opt.palette.color(colorGroup(opt.state), QPalette::Button);
    if (_pressed == index)
        fill = shade(fill, 2);
    else if (_hovered == index)
        fill = shade(fill, 1);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(fill);
    painter->drawRoundedRect(button, ButtonRadius, ButtonRadius);
    painter->restore();

    const QIcon::Mode mode = opt.state.testFlag(QStyle::State_Enabled) ? QIcon::Normal
                                                                      : QIcon::Disabled;
    const QRect iconRect(button.center().x() - IconSize / 2 + 1,
                         button.center().y() - IconSize / 2 + 1, IconSize, IconSize);
    const QString icon = hasViewer(index) ? QStringLiteral("image") : QStringLiteral("expand");
    AppIcons::themed(icon).paint(painter, iconRect, Qt::AlignCenter, mode);
}

///
/// \brief Reports whether a viewport point lands on the viewer button of a cell.
/// \param pos Point in the viewport's coordinates.
/// \param index Model index being inspected.
/// \return True when the point is over the button this delegate draws for the cell.
///
bool ElidedTextDelegate::coversButton(const QPoint &pos, const QModelIndex &index) const
{
    if (!_view || !index.isValid() || _view->itemDelegateForIndex(index) != this)
        return false;
    return buttonRect(_view->visualRect(index)).contains(pos);
}

///
/// \brief Routes clicks that land on the viewer button to viewRequested().
/// \param event Event delivered to the cell.
/// \param model Model behind the view.
/// \param option Style options for the cell.
/// \param index Model index the event belongs to.
/// \return True when the event was consumed by the button.
///
bool ElidedTextDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                     const QStyleOptionViewItem &option, const QModelIndex &index)
{
    const QEvent::Type type = event->type();
    if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonRelease) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        const QStyleOptionViewItem opt = cellOption(option, index);
        const bool onButton = mouse->button() == Qt::LeftButton && needsButton(opt, index)
                              && buttonRect(opt.rect).contains(mouse->position().toPoint());

        // The press is left to the view so the row still selects; only its visual
        // state is taken here, and the click itself is reported on release.
        const QPersistentModelIndex pressed = _pressed;
        _pressed = (type == QEvent::MouseButtonPress && onButton) ? QPersistentModelIndex(index)
                                                                  : QPersistentModelIndex();
        if (_pressed != pressed && _view)
            _view->viewport()->update();

        if (type == QEvent::MouseButtonRelease && onButton) {
            emit viewRequested(index);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

///
/// \brief Tracks the pointer over the viewport to give the button a hover state.
/// \param watched Object the event was sent to.
/// \param event Event being delivered.
/// \return False, so the view keeps handling the event.
///
bool ElidedTextDelegate::eventFilter(QObject *watched, QEvent *event)
{
    if (_view && watched == _view->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            const QPoint pos = static_cast<QMouseEvent *>(event)->position().toPoint();
            const QModelIndex index = _view->indexAt(pos);
            setHoveredIndex(coversButton(pos, index) ? index : QModelIndex());
        } else if (event->type() == QEvent::Leave) {
            setHoveredIndex(QModelIndex());
        }
    }
    return QStyledItemDelegate::eventFilter(watched, event);
}

///
/// \brief Returns the button area inside a cell.
/// \param cell Rectangle of the whole cell.
/// \return Square button rectangle at the trailing edge of the cell.
///
QRect ElidedTextDelegate::buttonRect(const QRect &cell) const
{
    const int size = qMin(ButtonSize, cell.height());
    const int right = cell.right() - EdgeGap - scrollBarOverlap();
    return QRect(right - size, cell.top() + (cell.height() - size) / 2, size, size);
}

///
/// \brief Returns the width an overlay scroll bar covers at the trailing edge.
/// \return Width to keep free, in pixels, or zero when nothing overlaps the cells.
///
/// A transient scroll bar floats above the viewport instead of taking a strip of
/// its own, so a button drawn flush with the cell edge would sit underneath it.
///
int ElidedTextDelegate::scrollBarOverlap() const
{
    if (!_view)
        return 0;
    const QScrollBar *bar = _view->verticalScrollBar();
    if (!bar || bar->minimum() >= bar->maximum())
        return 0;
    QStyle *style = _view->style();
    if (!style->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, bar))
        return 0;
    return style->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, bar);
}

///
/// \brief Reports whether a cell's text is too wide to be shown in full.
/// \param option Style options with the text already filled in.
/// \return True when the text does not fit the cell.
///
bool ElidedTextDelegate::isTruncated(const QStyleOptionViewItem &option) const
{
    if (option.text.isEmpty())
        return false;
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option,
                                                 option.widget);
    return option.fontMetrics.horizontalAdvance(option.text) > textRect.width();
}

///
/// \brief Reports whether a cell carries a value a viewer can show better than the cell.
/// \param index Model index being inspected.
/// \return True when the cell holds an encoded picture.
///
bool ElidedTextDelegate::hasViewer(const QModelIndex &index) const
{
    return !index.data(ValueRoles::ImageDataRole).toByteArray().isEmpty();
}

///
/// \brief Reports whether a cell should carry the viewer button.
/// \param option Style options with the text already filled in.
/// \param index Model index being inspected.
/// \return True when the cell needs a viewer button.
///
/// A picture earns the button whatever the column width, because no width would make the
/// cell show it; text only earns it once the column has to cut it short.
///
bool ElidedTextDelegate::needsButton(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    return hasViewer(index) || isTruncated(option);
}

///
/// \brief Builds a fully initialized style option for a cell.
/// \param option Style options handed in by the view.
/// \param index Model index being inspected.
/// \return Style option with text, palette, and state applied.
///
QStyleOptionViewItem ElidedTextDelegate::cellOption(const QStyleOptionViewItem &option,
                                                    const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    if (!opt.widget)
        opt.widget = _view;
    return opt;
}

///
/// \brief Remembers which button the pointer is over and repaints on a change.
/// \param index Cell whose button is hovered, or an invalid index for none.
///
void ElidedTextDelegate::setHoveredIndex(const QModelIndex &index)
{
    if (_hovered == index)
        return;
    _hovered = index;
    _view->viewport()->update();
}
