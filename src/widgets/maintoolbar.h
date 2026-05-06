#pragma once

#include <QToolBar>

class QAction;
class EndpointSelectorWidget;
class MainToolButton;
class SecuritySelectorWidget;

///
/// \brief Main application toolbar with themed actions and connection selectors.
///
class MainToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit MainToolBar(QWidget *parent = nullptr);

    void setupFromDesignerActions();

private:
    MainToolButton *addMainButton(QAction *action);

    EndpointSelectorWidget *_endpointSelectorWidget;
    SecuritySelectorWidget *_securitySelectorWidget;
};
