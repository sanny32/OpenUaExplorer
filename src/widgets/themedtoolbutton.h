// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedtoolbutton.h
/// \brief Declares a theme-aware tool button.
///

#pragma once

#include <QSize>
#include <QToolButton>

///
/// \brief Tool button that refreshes its icon for the active application theme.
///
class ThemedToolButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(bool squareIconOnly READ squareIconOnly WRITE setSquareIconOnly)

public:
    ///
    /// \brief Constructs the themed tool button.
    /// \param parent Parent widget.
    ///
    explicit ThemedToolButton(QWidget *parent = nullptr);

    ///
    /// \brief Returns the minimum size hint, squared when in icon-only mode.
    /// \return Minimum size hint.
    ///
    QSize minimumSizeHint() const override;

    ///
    /// \brief Returns the preferred size, squared when in icon-only mode.
    /// \return Preferred size hint.
    ///
    QSize sizeHint() const override;

    ///
    /// \brief Reports whether the button is forced square when it shows only an icon.
    /// \return True when square icon-only mode is enabled.
    ///
    bool squareIconOnly() const;

    ///
    /// \brief Enables or disables forcing a square shape for icon-only buttons.
    /// \param enabled True to force a square shape.
    ///
    void setSquareIconOnly(bool enabled);

    ///
    /// \brief Sets the themed icon by resource name.
    /// \param name Icon resource name.
    ///
    void setIcon(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();
    QSize squareSize(const QSize &size) const;

    bool _squareIconOnly = false;
    QString _iconName;
};
