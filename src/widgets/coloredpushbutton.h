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
        QColor base;
        QColor hover;
        QColor pressed;
        QColor text = Qt::white;
    };

    explicit ColoredPushButton(QWidget *parent = nullptr);

    void setColors(const Colors &colors);
    void clearColors();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Colors m_colors;
    bool m_colored = false;
};
