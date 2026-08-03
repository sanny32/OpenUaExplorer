// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file toolbuttonlayout.cpp
/// \brief Implements helpers for responsive tool-button layouts.
///

#include <QLayout>

#include "themedtoolbutton.h"
#include "toolbuttonlayout.h"

namespace {

///
/// \brief Applies one presentation style to a group of tool buttons.
/// \param buttons Buttons to update.
/// \param style Presentation style to apply.
///
void setButtonStyle(const QList<ThemedToolButton *> &buttons, Qt::ToolButtonStyle style)
{
    for (ThemedToolButton *button : buttons)
        button->setToolButtonStyle(style);
}

} // namespace

namespace ToolButtonLayout {

///
/// \brief Shows button text when the expanded layout fits, otherwise shows only icons.
/// \param layout Layout whose minimum width determines whether expanded buttons fit.
/// \param availableWidth Width currently available to the layout.
/// \param buttons Tool buttons that switch between expanded and compact styles.
///
void adaptToWidth(QLayout *layout, int availableWidth,
                  const QList<ThemedToolButton *> &buttons)
{
    for (ThemedToolButton *button : buttons) {
        button->setIconOnlyMinimumWidth(true);
        QSizePolicy policy = button->sizePolicy();
        policy.setHorizontalPolicy(QSizePolicy::Preferred);
        button->setSizePolicy(policy);
    }

    setButtonStyle(buttons, Qt::ToolButtonTextBesideIcon);
    layout->invalidate();

    int expandedMinimumWidth = layout->minimumSize().width();
    for (ThemedToolButton *button : buttons) {
        expandedMinimumWidth += button->sizeHint().width()
            - button->minimumSizeHint().width();
    }

    if (expandedMinimumWidth <= availableWidth)
        return;

    setButtonStyle(buttons, Qt::ToolButtonIconOnly);
    layout->invalidate();
}

} // namespace ToolButtonLayout
