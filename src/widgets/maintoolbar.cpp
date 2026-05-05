#include "maintoolbar.h"
#include "endpointselectorwidget.h"
#include "maintoolbutton.h"
#include "securityselectorwidget.h"

#include <QAction>
#include <QSize>
#include <QSizePolicy>
#include <QWidget>

MainToolBar::MainToolBar(QWidget *parent)
    : QToolBar(parent)
    , _endpointSelectorWidget(new EndpointSelectorWidget(this))
    , _securitySelectorWidget(new SecuritySelectorWidget(this))
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
    addWidget(fixedGap(8));
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
