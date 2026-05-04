#pragma once

#include <QToolBar>

class QAction;
class ConnectionStatusWidget;
class EndpointSelectorWidget;
class QIcon;
class MainToolButton;
class SecuritySelectorWidget;

class MainToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit MainToolBar(QWidget *parent = nullptr);

    void setupFromDesignerActions();
    void setConnectionIcon(const QIcon &icon);

private:
    MainToolButton *addMainButton(QAction *action);
    QWidget *fixedGap(int width);

    EndpointSelectorWidget *_endpointSelectorWidget;
    SecuritySelectorWidget *_securitySelectorWidget;
    ConnectionStatusWidget *_connectionStatusWidget;
};
