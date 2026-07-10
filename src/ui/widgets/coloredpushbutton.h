// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file coloredpushbutton.h
/// \brief Declares a color-configurable push button.
///

#pragma once

#include <QColor>
#include <QPushButton>

///
/// \brief Push button that can render explicit colors without stylesheets.
///
class ColoredPushButton : public QPushButton
{
    Q_OBJECT

public:
    ///
    /// \brief Color set used to paint button states.
    ///
    struct Colors {
        /// \brief Normal-state background color.
        QColor base;
        /// \brief Hover-state background color.
        QColor hover;
        /// \brief Pressed-state background color.
        QColor pressed;
        /// \brief Text color.
        QColor text = Qt::white;
    };

    ///
    /// \brief Constructs an uncoloured button that paints like a standard push button.
    /// \param parent Parent widget.
    ///
    explicit ColoredPushButton(QWidget *parent = nullptr);

    ///
    /// \brief Enables custom colouring, deriving the dark variant from the given light colours.
    /// \param colors Light-theme colour set.
    ///
    void setColors(const Colors &colors);

    ///
    /// \brief Disables custom colouring and restores the application palette.
    ///
    void clearColors();

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    static Colors darkColorsFromLight(const Colors &lightColors);

    void applyColors(const Colors &colors);
    void refreshColors();

    Colors _colors;
    Colors _lightColors;
    Colors _darkColors;
    bool _colored = false;
};
