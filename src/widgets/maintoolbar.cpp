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
#include "endpointselectorwidget.h"
#include "fixedgap.h"
#include "maintoolbutton.h"
#include "securityselectorwidget.h"

///
/// \brief Constructs the toolbar with its endpoint and security selector widgets.
/// \param parent Parent widget.
///
MainToolBar::MainToolBar(QWidget *parent)
    : QToolBar(parent)
    , _endpointSelectorWidget(new EndpointSelectorWidget(this))
    , _securitySelectorWidget(new SecuritySelectorWidget(this))
{
    setMovable(false);
    setIconSize(QSize(24, 24));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

///
/// \brief Rebuilds the toolbar from its Designer actions, appending the selector widgets.
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
    addWidget(_endpointSelectorWidget);
    addWidget(new FixedGap(18, this));
    addWidget(_securitySelectorWidget);
    addWidget(new FixedGap(8, this));
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
