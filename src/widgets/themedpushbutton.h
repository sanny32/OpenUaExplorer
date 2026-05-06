// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedpushbutton.h
/// \brief Declares a theme-aware push button.
///

#pragma once

#include <QPushButton>

///
/// \brief Push button that refreshes its icon for the active application theme.
///
class ThemedPushButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ThemedPushButton(QWidget *parent = nullptr);

    void setIcon(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
};
