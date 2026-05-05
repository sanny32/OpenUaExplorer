#pragma once

#include <QToolBar>

class QAction;
class EndpointSelectorWidget;
class MainToolButton;
class SecuritySelectorWidget;

class MainToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit MainToolBar(QWidget *parent = nullptr);

    void setupFromDesignerActions();

private:
    MainToolButton *addMainButton(QAction *action);
    QWidget *fixedGap(int width);

    EndpointSelectorWidget *_endpointSelectorWidget;
    SecuritySelectorWidget *_securitySelectorWidget;
};
