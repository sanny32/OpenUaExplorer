#pragma once

#include <QToolButton>

class QAction;

class MainToolButton : public QToolButton
{
    Q_OBJECT

public:
    explicit MainToolButton(QAction *action, QWidget *parent = nullptr);
};
