#pragma once

#include <QWidget>

namespace Ui {
class TrendPanelWidget;
}

class TrendPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrendPanelWidget(QWidget *parent = nullptr);
    ~TrendPanelWidget() override;

protected:
    void changeEvent(QEvent *event) override;

private:
    void configureToolbar();
    void applyThemeIcons();

    Ui::TrendPanelWidget *ui;
};
