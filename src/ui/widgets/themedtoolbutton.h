// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedtoolbutton.h
/// \brief Declares a theme-aware tool button.
///

#pragma once

#include <QColor>
#include <QIcon>
#include <QSize>
#include <QToolButton>

///
/// \brief Tool button that refreshes its icon for the active application theme.
///
/// Bordered application styles (see MacAppStyle) paint an outline around these
/// so toolbar commands read as buttons; flat styles render them unchanged.
///
class ThemedToolButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(bool squareIconOnly READ squareIconOnly WRITE setSquareIconOnly)
    Q_PROPERTY(bool linkStyle READ linkStyle WRITE setLinkStyle)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName)

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
    /// \brief Reports whether the button is painted as a link-style action.
    /// \return True when link-style rendering is enabled.
    ///
    bool linkStyle() const;

    ///
    /// \brief Returns the themed icon resource name.
    /// \return Icon resource name.
    ///
    QString iconName() const;

    ///
    /// \brief Enables or disables forcing a square shape for icon-only buttons.
    /// \param enabled True to force a square shape.
    ///
    void setSquareIconOnly(bool enabled);

    ///
    /// \brief Enables or disables link-style rendering.
    /// \param enabled True to paint as a link-style action.
    ///
    void setLinkStyle(bool enabled);

    ///
    /// \brief Sets the themed icon resource name.
    /// \param name Icon resource name.
    ///
    void setIconName(const QString &name);

    ///
    /// \brief Sets the themed icon by resource name.
    /// \param name Icon resource name.
    ///
    void setIcon(const QString &name);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void refreshIcon();
    QIcon tintedLinkIcon(const QColor &color) const;
    QColor linkColor() const;
    QSize squareSize(const QSize &size) const;

    bool _squareIconOnly = false;
    bool _linkStyle = false;
    QString _iconName;
};
