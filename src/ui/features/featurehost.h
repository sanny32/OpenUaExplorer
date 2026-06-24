// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featurehost.h
/// \brief Declares the host facade passed to UI features.
///

#pragma once

#include <functional>

#include <QList>
#include <QMainWindow>
#include <QString>
#include <Qt>

class ConnectionController;
class FeatureManager;
class OpcUaClientService;
class ServiceModuleManager;
class QDockWidget;
class QMenu;
class QToolBar;
class SelectionContext;

///
/// \brief Provides UI features with shared services and contribution points.
///
class FeatureHost
{
public:
    ///
    /// \brief Constructs a facade over the main window composition root.
    /// \param mainWindow Main window receiving feature UI.
    /// \param viewMenu View menu receiving dock toggle actions.
    /// \param toolBar Main toolbar.
    /// \param clientService Shared OPC UA client service.
    /// \param connectionController Shared connection controller.
    /// \param dataModules Shared data-plugin registry.
    /// \param features Feature registry receiving contributions.
    /// \param selection Shared selected-node context.
    ///
    /// Features obtain built-in data plugins by type through dataModules(), so the
    /// host stays unaware of any concrete plugin.
    ///
    FeatureHost(QMainWindow *mainWindow,
                QMenu *viewMenu,
                QToolBar *toolBar,
                OpcUaClientService *clientService,
                ConnectionController *connectionController,
                ServiceModuleManager *dataModules,
                FeatureManager *features,
                SelectionContext *selection);

    ///
    /// \brief Returns the main window.
    /// \return Main window.
    ///
    QMainWindow *mainWindow() const;

    ///
    /// \brief Returns the View menu.
    /// \return View menu.
    ///
    QMenu *viewMenu() const;

    ///
    /// \brief Returns the main toolbar.
    /// \return Main toolbar.
    ///
    QToolBar *toolBar() const;

    ///
    /// \brief Returns the OPC UA client service.
    /// \return Client service.
    ///
    OpcUaClientService *clientService() const;

    ///
    /// \brief Returns the connection controller.
    /// \return Connection controller.
    ///
    ConnectionController *connectionController() const;

    ///
    /// \brief Returns the data-plugin registry.
    /// \return Data-plugin registry.
    ///
    ServiceModuleManager *dataModules() const;

    ///
    /// \brief Returns the feature registry.
    /// \return Feature registry.
    ///
    FeatureManager *features() const;

    ///
    /// \brief Returns the selected-node context.
    /// \return Selection context.
    ///
    SelectionContext *selection() const;

    ///
    /// \brief Adds a dock to the main window and View menu.
    /// \param area Default dock area.
    /// \param dock Dock widget.
    ///
    void addDock(Qt::DockWidgetArea area, QDockWidget *dock);

    ///
    /// \brief Records a default dock split.
    /// \param first Existing dock.
    /// \param second Dock to split from the first.
    /// \param orientation Split orientation.
    ///
    void splitDock(QDockWidget *first, QDockWidget *second, Qt::Orientation orientation);

    ///
    /// \brief Records a default dock resize.
    /// \param docks Docks to resize.
    /// \param sizes Target sizes.
    /// \param orientation Resize orientation.
    ///
    void resizeDocks(const QList<QDockWidget *> &docks,
                     const QList<int> &sizes,
                     Qt::Orientation orientation);

    ///
    /// \brief Returns a contributed dock by object name.
    /// \param objectName Dock object name.
    /// \return Matching dock, or nullptr.
    ///
    QDockWidget *dock(const QString &objectName) const;

    ///
    /// \brief Registers a feature command callable by MainWindow.
    /// \param id Stable command identifier.
    /// \param command Command callback.
    ///
    void registerCommand(const QString &id, std::function<void()> command);

private:
    QMainWindow *_mainWindow;
    QMenu *_viewMenu;
    QToolBar *_toolBar;
    OpcUaClientService *_clientService;
    ConnectionController *_connectionController;
    ServiceModuleManager *_dataModules;
    FeatureManager *_features;
    SelectionContext *_selection;
};
