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
    ///
    /// \brief Constructs the themed push button.
    /// \param parent Parent widget.
    ///
    explicit ThemedPushButton(QWidget *parent = nullptr);

    ///
    /// \brief Sets the themed icon by resource name.
    /// \param name Icon resource name.
    ///
    void setIcon(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
};
