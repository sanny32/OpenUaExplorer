#pragma once

#include <QWidget>

namespace Ui {
class SecuritySelectorWidget;
}

class QString;

///
/// \brief Widget for selecting the current OPC UA security policy.
///
class SecuritySelectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SecuritySelectorWidget(QWidget *parent = nullptr);
    ~SecuritySelectorWidget() override;

    QString currentSecurityPolicy() const;

private:
    Ui::SecuritySelectorWidget *ui;
};
