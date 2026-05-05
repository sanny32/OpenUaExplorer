#pragma once

#include <QWidget>

namespace Ui {
class AttributesWidget;
}

class AttributesModel;

class AttributesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AttributesWidget(QWidget *parent = nullptr);
    ~AttributesWidget() override;

private:
    void setupAttributesView();

    Ui::AttributesWidget *ui;
    AttributesModel      *_model;
};
