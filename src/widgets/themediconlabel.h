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
    explicit ThemedIconLabel(QWidget *parent = nullptr);

    void setIcon(const QString &name, QSize size = QSize());

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
    QSize   _size;
};
