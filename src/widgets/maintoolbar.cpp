#include "maintoolbar.h"
#include "connectionstatuswidget.h"
#include "endpointselectorwidget.h"
#include "maintoolbutton.h"
#include "securityselectorwidget.h"

#include <QAction>
#include <QIcon>
#include <QSize>
#include <QSizePolicy>
#include <QWidget>

MainToolBar::MainToolBar(QWidget *parent)
    : QToolBar(parent)
    , _endpointSelectorWidget(new EndpointSelectorWidget(this))
    , _securitySelectorWidget(new SecuritySelectorWidget(this))
    , _connectionStatusWidget(new ConnectionStatusWidget(this))
{
    setMovable(false);
    setIconSize(QSize(24, 24));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void MainToolBar::setupFromDesignerActions()
{
    const QList<QAction *> designerActions = actions();
    clear();

    for (QAction *action : designerActions) {
        if (action->isSeparator()) {
            addSeparator();
        } else {
            addMainButton(action);
        }
    }

    QWidget *toolbarSpacer = new QWidget(this);
    toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    addWidget(toolbarSpacer);
    addWidget(_endpointSelectorWidget);
    addWidget(fixedGap(18));
    addWidget(_securitySelectorWidget);
    addWidget(fixedGap(24));
    addSeparator();
    addWidget(fixedGap(18));
    addWidget(_connectionStatusWidget);
}

void MainToolBar::setConnectionIcon(const QIcon &icon)
{
    _connectionStatusWidget->setIcon(icon);
}

MainToolButton *MainToolBar::addMainButton(QAction *action)
{
    MainToolButton *button = new MainToolButton(action, this);
    addWidget(button);
    return button;
}

QWidget *MainToolBar::fixedGap(int width)
{
    QWidget *gap = new QWidget(this);
    gap->setFixedWidth(width);
    gap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    return gap;
}
