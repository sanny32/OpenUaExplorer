#include <QAction>

#include "maintoolbutton.h"

namespace {
constexpr int mainToolButtonWidth = 78;
}

///
/// \brief MainToolButton::MainToolButton
/// \param action
/// \param parent
///
MainToolButton::MainToolButton(QAction *action, QWidget *parent)
    : ThemedToolButton(parent)
{
    setDefaultAction(action);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setMinimumWidth(mainToolButtonWidth);
    setMaximumWidth(mainToolButtonWidth);

    if (action) {
        setToolTip(action->text());
    }
}
