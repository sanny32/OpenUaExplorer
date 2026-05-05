#include <QAction>

#include "maintoolbutton.h"

namespace {
constexpr int mainToolButtonWidth = 78;
}

MainToolButton::MainToolButton(QAction *action, QWidget *parent)
    : QToolButton(parent)
{
    setDefaultAction(action);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setMinimumWidth(mainToolButtonWidth);
    setMaximumWidth(mainToolButtonWidth);

    if (action) {
        setToolTip(action->text());
    }
}
