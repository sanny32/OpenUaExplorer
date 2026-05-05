#pragma once

#include <QDialog>

namespace Ui {
class ConnectionDialog;
}

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);
    ~ConnectionDialog() override;

protected:
    void changeEvent(QEvent *event) override;

private:
    void updateTheme();

    Ui::ConnectionDialog *ui;
};
