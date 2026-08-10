// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file formatbadge.h
/// \brief Declares the tile that names a value's encoding.
///

#pragma once

#include <QString>
#include <QWidget>

///
/// \brief Rounded tile carrying a short format name such as "PNG" or "GIF".
///
/// A header needs something to look at before the text is read, and an encoding name is
/// short enough to be that something. The tile paints itself rather than styling a label,
/// so it keeps its shape under every application style.
///
class FormatBadge : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)

public:
    ///
    /// \brief Constructs an empty badge.
    /// \param parent Parent widget.
    ///
    explicit FormatBadge(QWidget *parent = nullptr);

    ///
    /// \brief Returns the format name shown on the badge.
    ///
    QString text() const;

    ///
    /// \brief Sets the format name shown on the badge.
    /// \param text Short format name; the badge stays blank when it is empty.
    ///
    void setText(const QString &text);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString _text;
};
