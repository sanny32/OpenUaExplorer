// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedaction.h
/// \brief Declares a theme-aware action.
///

#pragma once

#include <QAction>

///
/// \brief Action that refreshes its icon for the active application theme.
///
class ThemedAction : public QAction
{
    Q_OBJECT
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName)

public:
    ///
    /// \brief Constructs the themed action.
    /// \param parent Parent object.
    ///
    explicit ThemedAction(QObject *parent = nullptr);

    ///
    /// \brief Constructs the themed action with an icon and text.
    /// \param iconName Icon resource name.
    /// \param text Action text.
    /// \param parent Parent object.
    ///
    ThemedAction(const QString &iconName, const QString &text, QObject *parent = nullptr);

    ///
    /// \brief Returns the themed icon resource name.
    /// \return Icon resource name.
    ///
    QString iconName() const;

    ///
    /// \brief Sets the themed icon resource name.
    /// \param name Icon resource name.
    ///
    void setIconName(const QString &name);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
};
