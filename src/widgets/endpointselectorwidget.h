#pragma once

#include <QWidget>

namespace Ui {
class EndpointSelectorWidget;
}

class QString;

class EndpointSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EndpointSelectorWidget(QWidget *parent = nullptr);
    ~EndpointSelectorWidget() override;

    QString currentEndpoint() const;

private:
    Ui::EndpointSelectorWidget *ui;
};
