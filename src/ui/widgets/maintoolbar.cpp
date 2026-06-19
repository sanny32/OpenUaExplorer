// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file maintoolbar.cpp
/// \brief Implements the main application toolbar.
///

#include <QAction>
#include <QSize>
#include <QSizePolicy>
#include <QWidget>

#include "maintoolbar.h"
#include "fixedgap.h"
#include "maintoolbutton.h"
#include "themedtoolbutton.h"

///
/// \brief Constructs the toolbar with its favourites button.
/// \param parent Parent widget.
///
MainToolBar::MainToolBar(QWidget *parent)
    : QToolBar(parent)
    , _favoritesButton(new ThemedToolButton(this))
{
    setMovable(false);
    setIconSize(QSize(24, 24));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    _favoritesButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    _favoritesButton->setText(tr("Favorites"));
    _favoritesButton->setToolTip(tr("Connect to a favourite server"));
    _favoritesButton->setIcon(QStringLiteral("star"));
}

///
/// \brief Rebuilds the toolbar from its Designer actions, appending the quick-connect button.
///
void MainToolBar::setupFromDesignerActions()
{
    const QList<QAction *> designerActions = actions();
    clear();

    for (QAction *action : designerActions) {
        if (action->isSeparator()) {
            addSeparator();
        } else if (action->isVisible()) {
            addMainButton(action);
        }
    }

    QWidget *toolbarSpacer = new QWidget(this);
    toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    addWidget(toolbarSpacer);
    addWidget(_favoritesButton);
    addWidget(new FixedGap(8, this));
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
