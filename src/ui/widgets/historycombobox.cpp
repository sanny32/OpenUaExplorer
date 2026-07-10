// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historycombobox.cpp
/// \brief Implements the combo box with removable popup entries.
///

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QImage>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyledItemDelegate>

#include "appcolors.h"
#include "historycombobox.h"

namespace {

/// \brief Side of the square remove button, in pixels.
constexpr int removeButtonSize = 16;

/// \brief Gap kept between the remove button and the entry's right edge.
constexpr int removeButtonMargin = 4;

/// \brief Inset of the cross inside the remove button.
constexpr qreal removeGlyphInset = 4.5;

/// \brief Opacity of a remove button that is visible but not hovered.
constexpr qreal idleGlyphOpacity = 0.55;

/// \brief Horizontal space an entry reserves for its remove button.
int reservedWidth()
{
    return removeButtonSize + removeButtonMargin * 2;
}

}

///
/// \brief Returns the remove button's rectangle inside a popup entry's rectangle.
/// \param itemRect Entry rectangle in popup viewport coordinates.
/// \return Rectangle the entry's remove button occupies.
///
QRect HistoryComboBox::removeButtonRect(const QRect &itemRect)
{
    QRect rect(0, 0, removeButtonSize, removeButtonSize);
    rect.moveCenter(QPoint(itemRect.right() - removeButtonMargin - removeButtonSize / 2,
                           itemRect.center().y()));
    return rect;
}

///
/// \brief Paints each entry with a remove button and reserves room for it.
///
class HistoryComboBoxDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    ///
    /// \brief Returns the row whose remove button is under the pointer.
    /// \return Row index, or -1 when no button is hovered.
    ///
    int hoveredRow() const
    {
        return _hoveredRow;
    }

    ///
    /// \brief Sets the row whose remove button is under the pointer.
    /// \param row Row index, or -1 when no button is hovered.
    ///
    void setHoveredRow(int row)
    {
        _hoveredRow = row;
    }

    ///
    /// \brief Adds the remove button's width to the entry's natural size.
    /// \param option Style options for the entry.
    /// \param index Entry being measured.
    /// \return Size hint including the reserved button column.
    ///
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setWidth(size.width() + reservedWidth());
        size.setHeight(qMax(size.height(), removeButtonSize + removeButtonMargin * 2));
        return size;
    }

    ///
    /// \brief Draws the entry with its text elided before the remove button.
    /// \param painter Painter to draw with.
    /// \param option Style options for the entry.
    /// \param index Entry being drawn.
    ///
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem viewItem = option;
        initStyleOption(&viewItem, index);

        const QWidget *widget = viewItem.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        const QRect textRect =
            style->subElementRect(QStyle::SE_ItemViewItemText, &viewItem, widget);
        viewItem.text = viewItem.fontMetrics.elidedText(
            viewItem.text, viewItem.textElideMode,
            qMax(0, textRect.width() - reservedWidth()));
        style->drawControl(QStyle::CE_ItemViewItem, &viewItem, painter, widget);

        if (!viewItem.state.testFlag(QStyle::State_Selected)
            && !viewItem.state.testFlag(QStyle::State_MouseOver))
            return;

        const QRect buttonRect = HistoryComboBox::removeButtonRect(viewItem.rect);
        const QColor color = AppColors::mostLegible(
            paintedBackground(viewItem, buttonRect.center(), style),
            viewItem.palette.color(QPalette::Text),
            viewItem.palette.color(QPalette::HighlightedText));

        drawRemoveButton(painter, buttonRect, color, index.row() == _hoveredRow);
    }

private:
    static QColor paintedBackground(const QStyleOptionViewItem &option, const QPoint &point,
                                    QStyle *style)
    {
        QStyleOptionViewItem probe = option;
        probe.text.clear();
        probe.icon = QIcon();
        probe.rect = option.rect.translated(-point);

        QImage sample(1, 1, QImage::Format_ARGB32);
        sample.fill(option.palette.color(QPalette::Base));

        QPainter samplePainter(&sample);
        style->drawControl(QStyle::CE_ItemViewItem, &probe, &samplePainter, option.widget);
        samplePainter.end();

        return sample.pixelColor(0, 0);
    }

    static void drawRemoveButton(QPainter *painter, const QRect &rect, QColor color, bool hovered)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        if (hovered) {
            QColor background = color;
            background.setAlphaF(0.18);
            painter->setPen(Qt::NoPen);
            painter->setBrush(background);
            painter->drawEllipse(rect);
        } else {
            color.setAlphaF(idleGlyphOpacity);
        }

        QPen pen(color, 1.5);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        const QRectF glyph = QRectF(rect).adjusted(removeGlyphInset, removeGlyphInset,
                                                   -removeGlyphInset, -removeGlyphInset);
        painter->drawLine(glyph.topLeft(), glyph.bottomRight());
        painter->drawLine(glyph.topRight(), glyph.bottomLeft());
        painter->restore();
    }

    int _hoveredRow = -1;
};

///
/// \brief Constructs the combo box and attaches the remove-button delegate.
/// \param parent Parent widget.
///
HistoryComboBox::HistoryComboBox(QWidget *parent)
    : QComboBox(parent)
    , _delegate(new HistoryComboBoxDelegate(this))
{
    attachToView();
}

///
/// \brief Widens the popup to fit the longest entry, then shows it.
///
void HistoryComboBox::showPopup()
{
    attachToView();
    setHoveredRemoveRow(-1);
    updatePopupWidth();
    QComboBox::showPopup();
}

///
/// \brief Consumes clicks on a remove button and tracks which button is hovered.
/// \param watched Object the event was sent to.
/// \param event Event being delivered.
/// \return True when the event was handled and must not reach the popup.
///
bool HistoryComboBox::eventFilter(QObject *watched, QEvent *event)
{
    if (_view && watched == _view && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        const QModelIndex current = _view->currentIndex();
        if (keyEvent->key() == Qt::Key_Delete && current.isValid()) {
            removeRow(current.row());
            return true;
        }
    }

    if (!_view || watched != _view->viewport())
        return QComboBox::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::MouseMove:
        setHoveredRemoveRow(rowUnderRemoveButton(
            static_cast<QMouseEvent *>(event)->position().toPoint()));
        break;
    case QEvent::Leave:
    case QEvent::Hide:
        setHoveredRemoveRow(-1);
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::LeftButton)
            break;

        const int row = rowUnderRemoveButton(mouseEvent->position().toPoint());
        if (row < 0)
            break;

        if (event->type() == QEvent::MouseButtonRelease)
            removeRow(row);
        return true;
    }
    default:
        break;
    }

    return QComboBox::eventFilter(watched, event);
}

///
/// \brief Binds the delegate and event filters to the current popup view.
///
/// The view is recreated whenever the style changes it, so this is repeated on every
/// popup rather than done once in the constructor.
///
void HistoryComboBox::attachToView()
{
    QAbstractItemView *itemView = view();
    if (_view == itemView)
        return;

    if (_view) {
        _view->removeEventFilter(this);
        _view->viewport()->removeEventFilter(this);
    }

    _view = itemView;
    _view->setItemDelegate(_delegate);
    _view->setMouseTracking(true);
    _view->installEventFilter(this);
    _view->viewport()->installEventFilter(this);
}

///
/// \brief Expands the popup to fit its longest entry, capped to the screen width.
///
void HistoryComboBox::updatePopupWidth()
{
    int contentWidth = width();
    const QFontMetrics metrics(_view->font());
    for (int index = 0; index < count(); ++index)
        contentWidth = qMax(contentWidth, metrics.horizontalAdvance(itemText(index)));

    contentWidth += style()->pixelMetric(QStyle::PM_ScrollBarExtent)
        + style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth) * 2
        + reservedWidth()
        + 32;

    if (const QScreen *currentScreen = screen())
        contentWidth = qMin(contentWidth, currentScreen->availableGeometry().width() - 40);

    _view->setMinimumWidth(contentWidth);
}

///
/// \brief Drops an entry, keeps the edit text sensible, and reports the removal.
/// \param row Row to remove.
///
void HistoryComboBox::removeRow(int row)
{
    if (row < 0 || row >= count())
        return;

    const QString text = itemText(row);
    const QString editText = currentText();
    setHoveredRemoveRow(-1);

    {
        const QSignalBlocker blocker(this);
        removeItem(row);
        if (isEditable() && editText != text)
            setEditText(editText);
    }

    if (count() == 0)
        hidePopup();
    else
        updatePopupWidth();

    emit itemRemoved(text);
}

///
/// \brief Maps a viewport position to the row whose remove button contains it.
/// \param viewportPosition Position in popup viewport coordinates.
/// \return Row index, or -1 when the position misses every button.
///
int HistoryComboBox::rowUnderRemoveButton(const QPoint &viewportPosition) const
{
    const QModelIndex index = _view->indexAt(viewportPosition);
    if (!index.isValid())
        return -1;
    if (!removeButtonRect(_view->visualRect(index)).contains(viewportPosition))
        return -1;
    return index.row();
}

///
/// \brief Highlights the hovered remove button, repainting the popup when it changes.
/// \param row Row index, or -1 when no button is hovered.
///
void HistoryComboBox::setHoveredRemoveRow(int row)
{
    if (_delegate->hoveredRow() == row)
        return;

    _delegate->setHoveredRow(row);
    if (_view)
        _view->viewport()->update();
}
