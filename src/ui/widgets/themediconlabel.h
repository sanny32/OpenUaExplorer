// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themediconlabel.h
/// \brief Declares a theme-aware icon label.
///

#pragma once

#include <QLabel>
#include <QSize>

///
/// \brief Label that displays a theme-aware icon pixmap.
///
class ThemedIconLabel : public QLabel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty themed icon label.
    /// \param parent Parent widget.
    ///
    explicit ThemedIconLabel(QWidget *parent = nullptr);

    ///
    /// \brief Sets the themed icon to display.
    /// \param name Icon resource name.
    /// \param size Desired pixmap size; invalid uses the icon's first available size.
    ///
    void setIcon(const QString &name, QSize size = QSize());

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
    QSize   _size;
};
