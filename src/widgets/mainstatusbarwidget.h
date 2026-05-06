#pragma once

#include <QWidget>

namespace Ui {
class MainStatusBarWidget;
}

///
/// \brief Status bar widget that displays connection state.
///
class MainStatusBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainStatusBarWidget(QWidget *parent = nullptr);
    ~MainStatusBarWidget() override;

private:
    Ui::MainStatusBarWidget *ui;
};
