// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file separatoritemdelegate.h
/// \brief Declares the item delegate that draws combo box separator entries.
///

#pragma once

#include <QStyledItemDelegate>

///
/// \brief Item delegate that paints separator entries as a legible rule.
///
/// The separator drawn by QComboBoxDelegate follows the style's toolbar-separator
/// primitive, which several styles render too faintly to see on a dark popup. This
/// delegate derives the rule from QPalette::Text instead, so it holds up in both
/// colour schemes. Attach it to any combo box popup that uses insertSeparator();
/// non-separator entries fall through to the base implementation.
///
class SeparatorItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    ///
    /// \brief Installs the delegate on a combo box popup, replacing the style's own.
    /// \param comboBox Combo box whose popup should draw legible separators.
    ///
    /// The popup view is recreated whenever the style changes it, so call this from
    /// showPopup() rather than once at construction.
    ///
    static void attachTo(class QComboBox *comboBox);

    ///
    /// \brief Reports whether an entry is a separator line rather than a choice.
    /// \param index Entry to test.
    /// \return True for separator entries.
    ///
    static bool isSeparator(const QModelIndex &index);

    ///
    /// \brief Draws a separator entry as a full-width rule.
    /// \param painter Painter to draw with.
    /// \param option Style options for the entry.
    ///
    static void drawSeparator(QPainter *painter, const QStyleOptionViewItem &option);

    ///
    /// \brief Returns the height a separator entry occupies.
    /// \return Separator size; the width is left to the view.
    ///
    static QSize separatorSizeHint();

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
