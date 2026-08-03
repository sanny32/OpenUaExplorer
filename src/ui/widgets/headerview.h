// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file headerview.h
/// \brief Declares the custom wrapped header view.
///

#pragma once

#include <QHash>
#include <QHeaderView>

///
/// \brief Header view with wrapped labels and per-section alignment.
///
class HeaderView : public QHeaderView
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the header and clamps sections to their wrapped minimum width.
    /// \param orientation Header orientation.
    /// \param parent Parent widget.
    ///
    explicit HeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

    ///
    /// \brief Returns the alignment used for sections without an explicit override.
    /// \return Default header alignment.
    ///
    Qt::Alignment defaultAlignment() const;

    ///
    /// \brief Sets the alignment used for sections without an explicit override.
    /// \param alignment Default header alignment.
    ///
    void setDefaultAlignment(Qt::Alignment alignment);

    ///
    /// \brief Returns the alignment for a section, falling back to the default.
    /// \param logicalIndex Section index.
    /// \return Section alignment.
    ///
    Qt::Alignment sectionAlignment(int logicalIndex) const;

    ///
    /// \brief Overrides the alignment for a single section.
    /// \param logicalIndex Section index.
    /// \param alignment Alignment to apply.
    ///
    void setSectionAlignment(int logicalIndex, Qt::Alignment alignment);

    ///
    /// \brief Serialises the base header state together with the per-section alignments.
    /// \return Opaque state blob suitable for restoreLayout().
    ///
    QByteArray saveLayout() const;

    ///
    /// \brief Restores the base header state and per-section alignments from a blob.
    /// \param state Blob produced by saveLayout().
    ///
    void restoreLayout(const QByteArray &state);

signals:
    ///
    /// \brief Emitted when a section's alignment changes.
    /// \param logicalIndex Section index.
    /// \param alignment New alignment.
    ///
    void sectionAlignmentChanged(int logicalIndex, Qt::Alignment alignment);

public:
    ///
    /// \brief Connects header-data changes so the header re-computes its wrapped height.
    /// \param model Model to display.
    ///
    void setModel(QAbstractItemModel *model) override;

protected:
    QSize sizeHint() const override;
    void showEvent(QShowEvent *event) override;
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;

private:
    int minimumWrappedSectionWidth(int logicalIndex) const;
    void enforceMinimumWidths();

    Qt::Alignment _alignment = Qt::AlignLeft | Qt::AlignVCenter;
    QHash<int, Qt::Alignment> _sectionAlignments;
};
