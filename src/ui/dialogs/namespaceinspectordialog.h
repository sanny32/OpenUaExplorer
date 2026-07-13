// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file namespaceinspectordialog.h
/// \brief Declares the namespace inspector dialog.
///

#pragma once

#include "dialogs/appbasedialog.h"

#include <QElapsedTimer>
#include <QString>
#include <QStringList>

#include "opcua/opcuatypes.h"

namespace Ui {
class NamespaceInspectorDialog;
}

class OpcUaBackend;
class QStandardItemModel;

///
/// \brief In-memory cache of one server's namespace table and node counts.
///
/// Outlives individual dialog instances so the address-space crawl runs only on
/// the first open or an explicit Refresh. The owner clears it when the connection
/// drops so a new session recomputes the counts.
///
struct NamespaceInspectorCache
{
    /// \brief Whether the cache holds a completed crawl result.
    bool valid = false;
    /// \brief Namespace URIs indexed by namespace index.
    QStringList uris;
    /// \brief Node counts keyed by namespace index.
    OpcUaNamespaceNodeCounts counts;
};

///
/// \brief Shows the server namespace table with per-namespace node counts.
///
/// The URI list is read from the server NamespaceArray; the node counts come from
/// a full address-space crawl that runs while the dialog is open.
///
class NamespaceInspectorDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and wires it to the backend.
    /// \param service OPC UA backend used to read namespaces and crawl node counts.
    /// \param cache Per-connection cache reused across dialog instances, or nullptr to disable caching.
    /// \param parent Parent widget.
    ///
    explicit NamespaceInspectorDialog(OpcUaBackend *service,
                                      NamespaceInspectorCache *cache,
                                      QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~NamespaceInspectorDialog() override;

protected:
    void showEvent(QShowEvent *event) override;
    void done(int result) override;

private slots:
    void refresh();
    void exportNamespaces();
    void updateSelectedPanel();

private:
    void handleNamespaces(const QStringList &namespaces, const QString &error);
    void handleStatisticsProgress(int visitedNodes);
    void handleStatistics(const OpcUaNamespaceNodeCounts &nodeCounts, const QString &error);
    void loadFromCache();
    void startCrawl();
    void rebuildTable();
    void setControlsBusy(bool busy);
    int selectedNamespaceIndex() const;
    static bool isSystemNamespace(int index, const QString &uri);

    Ui::NamespaceInspectorDialog *ui;
    OpcUaBackend *_service;
    NamespaceInspectorCache *_cache;
    QStandardItemModel *_model;
    QStringList _uris;
    OpcUaNamespaceNodeCounts _counts;
    QElapsedTimer _progressClock;
    int _selectedNamespaceIndex = -1;
    bool _loadedOnce = false;
    bool _countsReady = false;
    bool _crawling = false;
    bool _crawlAborted = false;
};
