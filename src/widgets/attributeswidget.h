// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.h
/// \brief Declares the selected node attributes widget.
///

#pragma once

#include <QWidget>

#include "itestdatapopulatable.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class AttributesWidget;
}

class AttributesModel;

///
/// \brief Widget that displays attributes for the selected OPC UA node.
///
class AttributesWidget : public QWidget, public ITestDataPopulatable
{
    Q_OBJECT

public:
    explicit AttributesWidget(QWidget *parent = nullptr);
    ~AttributesWidget() override;

    void populateWithTestData() override;
    void setNodeDetails(const OpcUaNodeDetails &details);
    void clear();

private:
    void setupAttributesView();

    Ui::AttributesWidget *ui;
    AttributesModel      *_model;
};
