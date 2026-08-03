// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file separatoritemdelegate.cpp
/// \brief Implements the item delegate that draws combo box separator entries.
///

#include <QAbstractItemView>
#include <QComboBox>
#include <QPainter>

#include "separatoritemdelegate.h"

namespace {

/// \brief Total height of a separator entry, leaving room around the rule.
constexpr int separatorHeight = 7;

/// \brief Horizontal inset of the rule inside the entry.
constexpr int separatorInset = 4;

/// \brief Opacity the rule is mixed into the popup background with.
constexpr qreal separatorOpacity = 0.28;

}

///
/// \brief Installs the delegate on a combo box popup, replacing the style's own.
/// \param comboBox Combo box whose popup should draw legible separators.
///
void SeparatorItemDelegate::attachTo(QComboBox *comboBox)
{
    QAbstractItemView *view = comboBox->view();
    if (view->findChild<SeparatorItemDelegate *>(QString(), Qt::FindDirectChildrenOnly))
        return;
    view->setItemDelegate(new SeparatorItemDelegate(view));
}

///
/// \brief Reports whether an entry is a separator line rather than a choice.
/// \param index Entry to test.
/// \return True for separator entries.
///
bool SeparatorItemDelegate::isSeparator(const QModelIndex &index)
{
    return index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
}

///
/// \brief Draws a separator entry as a full-width rule.
/// \param painter Painter to draw with.
/// \param option Style options for the entry.
///
void SeparatorItemDelegate::drawSeparator(QPainter *painter, const QStyleOptionViewItem &option)
{
    QRect rect = option.rect;
    if (const auto *view = qobject_cast<const QAbstractItemView *>(option.widget))
        rect.setWidth(view->viewport()->width());

    QColor color = option.palette.color(QPalette::Text);
    color.setAlphaF(separatorOpacity);

    painter->fillRect(QRect(rect.left() + separatorInset, rect.center().y(),
                            qMax(0, rect.width() - separatorInset * 2), 1),
                      color);
}

///
/// \brief Returns the height a separator entry occupies.
/// \return Separator size; the width is left to the view.
///
QSize SeparatorItemDelegate::separatorSizeHint()
{
    return QSize(0, separatorHeight);
}

///
/// \brief Paints separator entries as a rule and everything else as usual.
/// \param painter Painter to draw with.
/// \param option Style options for the entry.
/// \param index Entry being drawn.
///
void SeparatorItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    if (isSeparator(index)) {
        drawSeparator(painter, option);
        return;
    }
    QStyledItemDelegate::paint(painter, option, index);
}

///
/// \brief Collapses separator entries to the rule's height.
/// \param option Style options for the entry.
/// \param index Entry being measured.
/// \return Size hint for the entry.
///
QSize SeparatorItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    if (isSeparator(index))
        return separatorSizeHint();
    return QStyledItemDelegate::sizeHint(option, index);
}
