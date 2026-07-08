// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featurehost.cpp
/// \brief Implements the host facade passed to UI features.
///

#include "featurehost.h"

#include <utility>

#include <QAction>
#include <QDockWidget>
#include <QMenu>

#include "featuremanager.h"

///
/// \brief Constructs a facade over the main window composition root.
/// \param mainWindow Main window receiving feature UI.
/// \param viewMenu View menu receiving dock toggle actions.
/// \param toolBar Main toolbar.
/// \param clientService Shared OPC UA client service.
/// \param connectionController Shared connection controller.
/// \param dataModules Shared data-module registry.
/// \param features Feature registry receiving contributions.
/// \param selection Shared selected-node context.
///
FeatureHost::FeatureHost(QMainWindow *mainWindow,
                         QMenu *viewMenu,
                         QToolBar *toolBar,
                         OpcUaClientService *clientService,
                         ConnectionController *connectionController,
                         ServiceModuleManager *dataModules,
                         FeatureManager *features,
                         SelectionContext *selection)
    : _mainWindow(mainWindow)
    , _viewMenu(viewMenu)
    , _toolBar(toolBar)
    , _clientService(clientService)
    , _connectionController(connectionController)
    , _dataModules(dataModules)
    , _features(features)
    , _selection(selection)
{
}

///
/// \brief Returns the main window.
/// \return Main window.
///
QMainWindow *FeatureHost::mainWindow() const
{
    return _mainWindow;
}

///
/// \brief Returns the View menu.
/// \return View menu.
///
QMenu *FeatureHost::viewMenu() const
{
    return _viewMenu;
}

///
/// \brief Returns the main toolbar.
/// \return Main toolbar.
///
QToolBar *FeatureHost::toolBar() const
{
    return _toolBar;
}

///
/// \brief Returns the OPC UA client service.
/// \return Client service.
///
OpcUaClientService *FeatureHost::clientService() const
{
    return _clientService;
}

///
/// \brief Returns the connection controller.
/// \return Connection controller.
///
ConnectionController *FeatureHost::connectionController() const
{
    return _connectionController;
}

///
/// \brief Returns the data-module registry.
/// \return Data-module registry.
///
ServiceModuleManager *FeatureHost::dataModules() const
{
    return _dataModules;
}

///
/// \brief Returns the feature registry.
/// \return Feature registry.
///
FeatureManager *FeatureHost::features() const
{
    return _features;
}

///
/// \brief Returns the selected-node context.
/// \return Selection context.
///
SelectionContext *FeatureHost::selection() const
{
    return _selection;
}

///
/// \brief Adds a dock to the main window and View menu.
/// \param area Default dock area.
/// \param dock Dock widget.
///
void FeatureHost::addDock(Qt::DockWidgetArea area, QDockWidget *dock)
{
    if (!dock)
        return;
    if (!dock->parent())
        dock->setParent(_mainWindow);
    _mainWindow->addDockWidget(area, dock);
    _features->addDock(area, dock);
    if (_viewMenu)
        _viewMenu->insertAction(_viewMenu->actions().isEmpty() ? nullptr : _viewMenu->actions().constFirst(),
                                dock->toggleViewAction());
}

///
/// \brief Records a default dock split.
/// \param first Existing dock.
/// \param second Dock to split from the first.
/// \param orientation Split orientation.
///
void FeatureHost::splitDock(QDockWidget *first, QDockWidget *second,
                            Qt::Orientation orientation)
{
    _features->splitDock(first, second, orientation);
}

///
/// \brief Records a default dock resize.
/// \param docks Docks to resize.
/// \param sizes Target sizes.
/// \param orientation Resize orientation.
///
void FeatureHost::resizeDocks(const QList<QDockWidget *> &docks,
                              const QList<int> &sizes,
                              Qt::Orientation orientation)
{
    _features->resizeDocks(docks, sizes, orientation);
}

///
/// \brief Returns a contributed dock by object name.
/// \param objectName Dock object name.
/// \return Matching dock, or nullptr.
///
QDockWidget *FeatureHost::dock(const QString &objectName) const
{
    return _features->dock(objectName);
}

///
/// \brief Registers a feature command callable by MainWindow.
/// \param id Stable command identifier.
/// \param command Command callback.
///
void FeatureHost::registerCommand(const QString &id, std::function<void()> command)
{
    _features->registerCommand(id, std::move(command));
}
