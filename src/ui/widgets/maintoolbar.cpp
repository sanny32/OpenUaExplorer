// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file maintoolbar.cpp
/// \brief Implements the main application toolbar.
///

#include <QAction>
#include <QEvent>
#include <QSize>
#include <QSizePolicy>
#include <QWidget>

#include <utility>

#include "maintoolbar.h"
#include "fixedgap.h"
#include "maintoolbutton.h"
#include "themedtoolbutton.h"

///
/// \brief Constructs the toolbar.
/// \param parent Parent widget.
///
MainToolBar::MainToolBar(QWidget *parent)
    : QToolBar(parent)
{
    setMovable(false);
    setIconSize(QSize(24, 24));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

///
/// \brief Rebuilds the toolbar from its Designer actions.
///
/// Actions become buttons in declaration order, except the one named
/// "actionFavorites", which is pulled out and right-aligned after the spacer.
///
void MainToolBar::setupFromDesignerActions()
{
    const QList<QAction *> designerActions = actions();
    clear();

    _equalWidthButtons.clear();
    QAction *favoritesAction = nullptr;
    for (QAction *action : designerActions) {
        if (action->objectName() == "actionFavorites") {
            favoritesAction = action;
        } else if (action->isSeparator()) {
            addSeparator();
        } else if (action->isVisible()) {
            _equalWidthButtons.append(addMainButton(action));
        }
    }

    QWidget *toolbarSpacer = new QWidget(this);
    toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    addWidget(toolbarSpacer);

    if (favoritesAction) {
        _favoritesButton = addMainButton(favoritesAction);
        _equalWidthButtons.append(_favoritesButton);
    }

    equalizeButtonWidths();
}

///
/// \brief Sizes every tracked button to the widest label, keeping them compact.
///
/// The width is driven by the actual label advance (not QToolButton::sizeHint,
/// which pads text-under-icon buttons generously and would look too wide) plus a
/// small margin, floored at MainToolButton::minWidth. So the row stays tight for
/// the current labels yet grows just enough to never clip a longer translation.
///
void MainToolBar::equalizeButtonWidths()
{
    // Breathing room added around the widest label before it becomes the shared
    // button width. Tune this alone to make the whole row tighter or roomier.
    constexpr int labelMargin = 6;

    int width = MainToolButton::minWidth;
    for (ThemedToolButton *button : std::as_const(_equalWidthButtons)) {
        const int labelWidth = button->fontMetrics().horizontalAdvance(button->text());
        width = qMax(width, labelWidth + labelMargin);
    }

    for (ThemedToolButton *button : std::as_const(_equalWidthButtons)) {
        button->setMinimumWidth(width);
        button->setMaximumWidth(width);
    }
}

///
/// \brief Re-equalises button widths when the language, font, or style changes.
/// \param event Change event being handled.
///
void MainToolBar::changeEvent(QEvent *event)
{
    QToolBar::changeEvent(event);

    switch (event->type()) {
    case QEvent::LanguageChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:
        equalizeButtonWidths();
        break;
    default:
        break;
    }
}

///
/// \brief Gives access to the favourites button.
/// \return The favourites button.
///
ThemedToolButton *MainToolBar::favoritesButton() const
{
    return _favoritesButton;
}

///
/// \brief Adds a main toolbar button bound to an action.
/// \param action Action the button triggers.
/// \return The created button.
///
MainToolButton *MainToolBar::addMainButton(QAction *action)
{
    MainToolButton *button = new MainToolButton(action, this);
    addWidget(button);
    return button;
}
