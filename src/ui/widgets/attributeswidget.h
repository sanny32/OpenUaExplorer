// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.h
/// \brief Declares the selected node attributes widget.
///

#pragma once

#include <QWidget>

#include "appsettings.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class AttributesWidget;
}

class AttributesModel;
class ElidedTextDelegate;
class QEvent;
class QModelIndex;
class QPoint;
class ThemedAction;

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
    /// \brief Keeps the last read attributes visible but inactive while the connection is gone.
    /// \param offline True while the server connection is gone.
    ///
    void setOffline(bool offline);

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

public slots:
    ///
    /// \brief Applies the OPC UA timestamp display mode to the attributes tree.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Emitted when the user writes a value to the selected node.
    /// \param nodeId Node to write.
    /// \param value Converted value.
    /// \param valueType Selected QOpcUa::Types numeric value.
    ///
    void writeRequested(const QString &nodeId, const QVariant &value, int valueType);

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupAttributesView();
    void copySelectedAttributeCell();
    void copyAttributeTree();
    void showAttributesContextMenu(const QPoint &pos);
    void showAttributeValue(const QModelIndex &index);
    void setupWriteEditor(int valueType, const QString &dataTypeId, const QVariant &value);
    void updateValueEditor();
    void clearWriteEditor();
    void writeCurrentValue();

    Ui::AttributesWidget *ui;
    AttributesModel      *_model;
    ThemedAction         *_copyCellAction = nullptr;
    ThemedAction         *_copyTreeAction = nullptr;
    ElidedTextDelegate   *_valueDelegate = nullptr;
    QString               _nodeId;
    QString               _nodeName;
    bool                  _offline = false;
};
