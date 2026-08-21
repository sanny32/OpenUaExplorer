// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodel.h
/// \brief Declares the OPC UA data access table model.
///

#pragma once

#include <memory>
#include <vector>

#include <QAbstractItemModel>
#include <QStringList>
#include <QVector>

#include "appsettings.h"
#include "columnalignmentstore.h"
#include "dataaccessitem.h"
#include "opcua/opcuatypes.h"

class QMimeData;

namespace OpcUaFormat {
struct ValueElement;
}

///
/// \brief Tree model for OPC UA data access monitored items.
///
/// Every monitored node is a top-level row. A row whose value is composite expands into one
/// child row per array element or structure field, built the first time the row is expanded
/// and refreshed in place while the value keeps arriving.
///
class DataAccessModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty data-access model.
    /// \param parent Owning QObject.
    ///
    explicit DataAccessModel(QObject *parent = nullptr);

    ///
    /// \brief Replaces all rows.
    /// \param items New data-access rows.
    ///
    void setItems(const QVector<DataAccessItem> &items);

    ///
    /// \brief Updates the row matching the node, or appends a new row when absent.
    /// \param details Node details to add or update.
    ///
    void addOrUpdate(const OpcUaNodeDetails &details);

    ///
    /// \brief Applies a fresh attribute read to a listed row, leaving unlisted nodes out.
    /// \param details Node details to apply.
    /// \return True when a row for the node was found and updated.
    ///
    bool refreshItem(const OpcUaNodeDetails &details);

    ///
    /// \brief Appends a placeholder row for a node whose attributes are still being read.
    /// \param node Browsed node to show.
    ///
    void addPending(const OpcUaNodeInfo &node);

    ///
    /// \brief Clears the pending mark of a row once its request chain has finished.
    /// \param nodeId Node to update.
    ///
    void clearPending(const QString &nodeId);

    ///
    /// \brief Reports whether a row is still waiting for its attributes or subscription.
    /// \param nodeId Node to query.
    /// \return True while the row is pending.
    ///
    bool isPending(const QString &nodeId) const;

    ///
    /// \brief Refreshes the value, status, and timestamps of rows matching the read results.
    /// \param values Read results.
    ///
    void updateValues(const QVector<OpcUaDataValue> &values);

    ///
    /// \brief Records the publishing interval the server granted for a monitored node.
    /// \param nodeId Affected node.
    /// \param publishingInterval Granted interval in milliseconds; 0 clears the shown value.
    ///
    void setRevisedInterval(const QString &nodeId, double publishingInterval);

    ///
    /// \brief Removes the rows referenced by the given indexes, highest row first.
    /// \param rows Selected model rows.
    ///
    void removeRows(const QModelIndexList &rows);

    ///
    /// \brief Moves the given rows, kept as one block, in front of a destination row.
    /// \param rows Rows to move; indexes of this model, not of a proxy.
    /// \param destinationRow Row the block is inserted before; the row count appends.
    /// \return True when the row order changed.
    ///
    bool moveRows(const QModelIndexList &rows, int destinationRow);

    ///
    /// \brief Returns the MIME type carrying rows dragged inside the data-access table.
    /// \return MIME type string.
    ///
    static QString rowMimeType();

    ///
    /// \brief Collects the NodeIds of the given rows, or of every row when none are given.
    /// \param rows Optional selected rows.
    /// \return NodeIds for selected rows or all rows.
    ///
    QStringList nodeIds(const QModelIndexList &rows = {}) const;

    ///
    /// \brief Returns the item at a row.
    /// \param row Model row.
    /// \return Data item or an empty item.
    ///
    DataAccessItem itemAt(int row) const;

    ///
    /// \brief Exports the data-access rows as CSV text.
    /// \return CSV document with a header row.
    ///
    QString toCsv() const;

    ///
    /// \brief Removes all rows.
    ///
    void clear();

    ///
    /// \brief Marks the listed values as belonging to a connection that is gone.
    /// \param offline True while the server connection is gone.
    ///
    void setOffline(bool offline);

    ///
    /// \brief Reports whether the rows show values of a lost connection.
    /// \return True while the rows are offline.
    ///
    bool isOffline() const;

    ///
    /// \brief Returns the index of a row, either a monitored node or an element of one.
    /// \param row Row within the parent.
    /// \param column Model column.
    /// \param parent Parent index; an invalid parent addresses the monitored nodes.
    /// \return Model index, or an invalid index when the row does not exist.
    ///
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the parent index of an element row.
    /// \param child Child index.
    /// \return Parent index, or an invalid index for monitored nodes.
    ///
    QModelIndex parent(const QModelIndex &child) const override;

    ///
    /// \brief Reports whether a row expands, without building its elements.
    /// \param parent Row to query.
    /// \return True when the row has child rows.
    ///
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the number of rows, building the elements of an expanded row on demand.
    /// \param parent Parent index.
    /// \return Item count for the root, or the element count of a composite value.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index.
    /// \return Column count.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text, alignment, and status/subscription colours for a row.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Writes the edited subscription name into the Subscription column.
    /// \param index Cell being edited.
    /// \param value New subscription name.
    /// \param role Only Qt::EditRole is accepted.
    /// \return True when the value was applied.
    ///
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    ///
    /// \brief Returns the column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Re-emits the header titles after a UI language change.
    ///
    void retranslate();

    ///
    /// \brief Marks the Subscription column editable and every row draggable.
    /// \param index Cell to query.
    /// \return Item flags for the cell.
    ///
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    ///
    /// \brief Reports the drop actions rows dragged inside the table may use.
    /// \return Qt::MoveAction.
    ///
    Qt::DropActions supportedDropActions() const override;

    ///
    /// \brief Returns the MIME types the table produces and accepts.
    /// \return Single-entry list holding rowMimeType().
    ///
    QStringList mimeTypes() const override;

    ///
    /// \brief Encodes the dragged rows by NodeId, in row order.
    /// \param indexes Dragged cells; their rows are used.
    /// \return MIME data owned by the caller, or nullptr when nothing is draggable.
    ///
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    ///
    /// \brief Accepts a drop of the table's own rows between two rows.
    /// \param data Dragged MIME data.
    /// \param action Proposed drop action.
    /// \param row Row the data would be inserted before, or -1 to append.
    /// \param column Unused; rows move as a whole.
    /// \param parent Drop parent; only the root accepts rows.
    /// \return True when the drop would reorder rows.
    ///
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                         const QModelIndex &parent) const override;

    ///
    /// \brief Reorders the dragged rows in front of the drop row.
    /// \param data Dragged MIME data.
    /// \param action Drop action; only Qt::MoveAction reorders.
    /// \param row Row the data is inserted before, or -1 to append.
    /// \param column Unused; rows move as a whole.
    /// \param parent Drop parent; only the root accepts rows.
    /// \return True when the drop was handled.
    ///
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent) override;

    ///
    /// \brief Returns the text alignment for a column.
    /// \param column Column index.
    /// \return Column alignment.
    ///
    Qt::Alignment columnAlignment(int column) const;

    ///
    /// \brief Sets the text alignment for a column.
    /// \param column Column index.
    /// \param alignment Alignment to apply.
    ///
    void setColumnAlignment(int column, Qt::Alignment alignment);

    ///
    /// \brief Overrides the change-highlight preference of the given rows.
    /// \param rows Rows to change; indexes of this model, not of a proxy.
    /// \param mode Preference to store.
    ///
    void setHighlightMode(const QModelIndexList &rows, HighlightMode mode);

    ///
    /// \brief Returns the stored change-highlight preference of a row.
    /// \param row Model row.
    /// \return Stored preference, or FollowDefault for an out-of-range row.
    ///
    HighlightMode highlightMode(int row) const;

    ///
    /// \brief Reports the change-highlight preference a row resolves to.
    /// \param row Model row.
    /// \return True when changes of that row should be highlighted.
    ///
    bool highlightsChanges(int row) const;

public slots:
    ///
    /// \brief Sets the timestamp display mode and repaints the timestamp column.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

    ///
    /// \brief Sets the change-highlight preference rows follow unless overridden.
    /// \param enabled True to highlight value changes by default.
    ///
    void setDefaultHighlightChanges(bool enabled);

    ///
    /// \brief Sets how many array elements a value cell spells out and repaints the column.
    /// \param elements Longest array still spelled out element by element.
    ///
    void setInlineArrayElements(int elements);

public:

    ///
    /// \brief Columns exposed by the data-access table.
    ///
    enum Column {
        ColNumber       = 0,
        ColNodeId       = 1,
        ColDisplayName  = 2,
        ColValue        = 3,
        ColDataType     = 4,
        ColTimestamp    = 5,
        ColStatus       = 6,
        ColSubscription = 7,
        ColActualInterval = 8,
        ColCount        = 9
    };

    ///
    /// \brief Largest number of element rows a composite value expands into.
    ///
    /// Beyond this the elements outnumber what anyone reads, and a live subscription would
    /// spend every notification laying out rows nobody looks at; the rest is summarised
    /// by a single trailing row.
    ///
    static constexpr int MaxExpandedElements = 1000;

    ///
    /// \brief Row facts the value delegate paints with; the model itself stays theme-agnostic.
    ///
    enum Role {
        /// \brief Time of the last value change in milliseconds since the epoch; 0 when never changed.
        ValueChangedAtRole = Qt::UserRole + 1,
        /// \brief OpcUaFormat::StatusSeverity of the row's status code, as an int.
        StatusSeverityRole,
        /// \brief Publishing interval in milliseconds, or 0 when the row is not monitored.
        ExpectedIntervalRole,
        /// \brief Resolved change-highlight preference of the row, as a bool.
        HighlightChangesRole,
        /// \brief Named values a writable enumeration cell offers, as OpcUaEnumEntries.
        ///
        /// Empty for every cell whose value is not one the user may pick a name for, which
        /// is what tells the delegate to leave the cell to the write dialog.
        EnumEntriesRole
    };

private:
    ///
    /// \brief One element row of a composite value.
    ///
    struct ValueNode
    {
        /// \brief Row label: "[0]" for an array element, the field name for a structure.
        QString label;
        /// \brief Formatted element value.
        QString text;
        /// \brief Element type name.
        QString typeName;
        /// \brief Raw element value, kept to expand the element further.
        QVariant value;
        /// \brief Time of the last change of this element, in milliseconds since the epoch.
        qint64 changedAt = 0;
        /// \brief Row among its siblings.
        int row = 0;
        /// \brief Top-level row of the tree; tracked by the root only, so moves stay cheap.
        int topRow = 0;
        /// \brief Owning element, or null for the per-item root.
        ValueNode *parent = nullptr;
        /// \brief Element rows, filled once the row is expanded.
        std::vector<std::unique_ptr<ValueNode>> children;
        /// \brief True once the elements were built.
        bool childrenBuilt = false;
        /// \brief True when the element expands into elements of its own.
        bool expandable = false;
        /// \brief True for the trailing row standing in for the elements beyond the limit.
        bool placeholder = false;
    };

    ///
    /// \brief Resolves a row's highlight preference against the application-wide default.
    /// \param item Row to resolve.
    /// \return True when changes of that row should be highlighted.
    ///
    bool resolveHighlight(const DataAccessItem &item) const;

    ValueNode *nodeForIndex(const QModelIndex &index) const;
    ValueNode *rootNode(int topRow) const;
    static int topRowOf(const ValueNode *node);
    DataAccessItem itemForNode(const ValueNode *node) const;
    static QString elementText(const OpcUaFormat::ValueElement &element,
                               const ValueNode *parent, const DataAccessItem &item);
    void buildChildren(ValueNode *node, const DataAccessItem &item) const;
    void refreshChildren(int topRow, qint64 changedAt);
    void updateNode(ValueNode *node, const QModelIndex &nodeIndex, const QVariant &value,
                    const DataAccessItem &item, qint64 changedAt);
    QVariant nodeData(const ValueNode *node, int column, int role) const;

    QVector<DataAccessItem> _items;
    /// \brief Per-item element trees, indexed like _items; entries stay null until expanded.
    mutable std::vector<std::unique_ptr<ValueNode>> _roots;
    ColumnAlignmentStore _columnAlignments;
    AppSettings::TimestampMode _timestampMode;
    int _inlineArrayElements;
    bool _offline = false;
    bool _defaultHighlightChanges = false;
};
