// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featuremanager.cpp
/// \brief Implements the UI feature registry and dock layout contribution store.
///

#include "featuremanager.h"

#include <utility>

#include <QMainWindow>

#include "featureplugin.h"

///
/// \brief Constructs an empty feature registry.
/// \param parent Owning QObject.
///
FeatureManager::FeatureManager(QObject *parent)
    : QObject(parent)
{
}

///
/// \brief Destroys the feature registry and owned feature plugins.
///
FeatureManager::~FeatureManager() = default;

///
/// \brief Takes ownership of a feature plugin.
/// \param feature Feature to register.
///
void FeatureManager::registerFeature(FeaturePlugin *feature)
{
    if (feature)
        _features.push_back(std::unique_ptr<FeaturePlugin>(feature));
}

///
/// \brief Initializes every feature in registration order.
/// \param host Host services and contribution points.
///
void FeatureManager::initializeAll(FeatureHost &host)
{
    for (const std::unique_ptr<FeaturePlugin> &feature : _features)
        feature->initialize(host);
}

///
/// \brief Persists every feature's view state.
/// \param settings Settings store to write to.
///
void FeatureManager::saveState(AppSettings &settings) const
{
    for (const std::unique_ptr<FeaturePlugin> &feature : _features)
        feature->saveState(settings);
}

///
/// \brief Restores every feature's view state.
/// \param settings Settings store to read from.
///
void FeatureManager::restoreState(AppSettings &settings)
{
    for (const std::unique_ptr<FeaturePlugin> &feature : _features)
        feature->restoreState(settings);
}

///
/// \brief Clears runtime state in every feature.
///
void FeatureManager::clearRuntimeState()
{
    for (const std::unique_ptr<FeaturePlugin> &feature : _features)
        feature->clearRuntimeState();
}

///
/// \brief Records a dock placement contribution.
/// \param area Default dock area.
/// \param dock Dock widget.
///
void FeatureManager::addDock(Qt::DockWidgetArea area, QDockWidget *dock)
{
    if (!dock)
        return;
    _dockContributions.append({area, dock});
    _docksByName.insert(dock->objectName(), dock);
}

///
/// \brief Records a dock split contribution.
/// \param first Existing dock.
/// \param second Dock to split from the first.
/// \param orientation Split orientation.
///
void FeatureManager::splitDock(QDockWidget *first, QDockWidget *second,
                               Qt::Orientation orientation)
{
    if (first && second)
        _splitContributions.append({first, second, orientation});
}

///
/// \brief Records a dock resize contribution.
/// \param docks Docks to resize.
/// \param sizes Target sizes.
/// \param orientation Resize orientation.
///
void FeatureManager::resizeDocks(const QList<QDockWidget *> &docks,
                                 const QList<int> &sizes,
                                 Qt::Orientation orientation)
{
    if (!docks.isEmpty() && docks.size() == sizes.size())
        _resizeContributions.append({docks, sizes, orientation});
}

///
/// \brief Replays the recorded default dock layout on a main window.
/// \param window Main window receiving the layout.
///
void FeatureManager::resetDockLayout(QMainWindow &window) const
{
    for (const DockContribution &contribution : _dockContributions) {
        contribution.dock->show();
        window.addDockWidget(contribution.area, contribution.dock);
    }
    for (const SplitContribution &contribution : _splitContributions)
        window.splitDockWidget(contribution.first, contribution.second, contribution.orientation);
    for (const ResizeContribution &contribution : _resizeContributions)
        window.resizeDocks(contribution.docks, contribution.sizes, contribution.orientation);
}

///
/// \brief Returns a contributed dock by object name.
/// \param objectName Dock object name.
/// \return Matching dock, or nullptr.
///
QDockWidget *FeatureManager::dock(const QString &objectName) const
{
    return _docksByName.value(objectName, nullptr);
}

///
/// \brief Registers a command callable by MainWindow.
/// \param id Stable command identifier.
/// \param command Command callback.
///
void FeatureManager::registerCommand(const QString &id, std::function<void()> command)
{
    if (!id.isEmpty() && command)
        _commands.insert(id, std::move(command));
}

///
/// \brief Runs a registered command.
/// \param id Stable command identifier.
/// \return True when a command was found and run.
///
bool FeatureManager::triggerCommand(const QString &id) const
{
    const auto it = _commands.constFind(id);
    if (it == _commands.constEnd())
        return false;
    it.value()();
    return true;
}
