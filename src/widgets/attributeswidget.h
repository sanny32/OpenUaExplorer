#pragma once

#include <QWidget>

namespace Ui {
class AttributesWidget;
}

class AttributesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AttributesWidget(QWidget *parent = nullptr);
    ~AttributesWidget() override;

private:
    void populateAttributes();

    Ui::AttributesWidget *ui;
};
