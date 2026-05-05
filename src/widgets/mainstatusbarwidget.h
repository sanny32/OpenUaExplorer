#pragma once

#include <QWidget>

namespace Ui {
class MainStatusBarWidget;
}

class QIcon;

class MainStatusBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainStatusBarWidget(QWidget *parent = nullptr);
    ~MainStatusBarWidget() override;

    void setConnectionIcon(const QIcon &icon);

private:
    Ui::MainStatusBarWidget *ui;
};
