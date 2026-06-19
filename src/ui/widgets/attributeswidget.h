// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.h
/// \brief Declares the selected node attributes widget.
///

#pragma once

#include <QWidget>

#include "opcua/opcuatypes.h"

namespace Ui {
class AttributesWidget;
}

class AppSettings;
class AttributesModel;

///
/// \brief Widget that displays attributes for the selected OPC UA node.
///
class AttributesWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the attributes widget and its tree view.
    /// \param parent Parent widget.
    ///
    explicit AttributesWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~AttributesWidget() override;

    ///
    /// \brief Shows the attributes of the selected node, expanding the top level.
    /// \param details Selected node details.
    ///
    void setNodeDetails(const OpcUaNodeDetails &details);

    ///
    /// \brief Clears the attributes view.
    ///
    void clear();

    ///
    /// \brief Persists the attributes tree header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the attributes tree header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

private:
    void setupAttributesView();

    Ui::AttributesWidget *ui;
    AttributesModel      *_model;
};
