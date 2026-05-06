#pragma once

#include "themedtoolbutton.h"

class QAction;

class MainToolButton : public ThemedToolButton
{
    Q_OBJECT

public:
    explicit MainToolButton(QAction *action, QWidget *parent = nullptr);
};
