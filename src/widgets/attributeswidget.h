#pragma once

#include <QWidget>

namespace Ui {
class AttributesWidget;
}

class AttributesModel;

///
/// \brief Widget that displays attributes for the selected OPC UA node.
///
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
