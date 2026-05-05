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
/// \brief MainToolBar::MainToolBar
/// \param parent
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
/// \brief MainToolBar::setupFromDesignerActions
///
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
    addWidget(new FixedGap(18, this));
    addWidget(_securitySelectorWidget);
    addWidget(new FixedGap(8, this));
}

///
/// \brief MainToolBar::addMainButton
/// \param action
/// \return
///
MainToolButton *MainToolBar::addMainButton(QAction *action)
{
    MainToolButton *button = new MainToolButton(action, this);
    addWidget(button);
    return button;
}
