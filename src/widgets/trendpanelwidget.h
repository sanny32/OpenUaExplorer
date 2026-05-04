#pragma once

#include <QIcon>
#include <QString>
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

private:
    void configureToolbar();
    QIcon themedIcon(const QString &name) const;

    Ui::TrendPanelWidget *ui;
};
