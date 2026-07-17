// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featuremanager.h
/// \brief Declares the UI feature registry and dock layout contribution store.
///

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

#include <QDockWidget>

class AppSettings;
class FeatureHost;
class FeatureModule;
class QMainWindow;
struct SessionData;

///
/// \brief Owns UI features and replays their dock layout contributions.
///
class FeatureManager : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Stored dock placement contribution.
    ///
    struct DockContribution {
        Qt::DockWidgetArea area;
        QDockWidget *dock;
    };

    ///
    /// \brief Stored dock split contribution.
    ///
    struct SplitContribution {
        QDockWidget *first;
        QDockWidget *second;
        Qt::Orientation orientation;
    };

    ///
    /// \brief Stored dock resize contribution.
    ///
    struct ResizeContribution {
        QList<QDockWidget *> docks;
        QList<int> sizes;
        Qt::Orientation orientation;
    };

    ///
    /// \brief Constructs an empty feature registry.
    /// \param parent Owning QObject.
    ///
    explicit FeatureManager(QObject *parent = nullptr);

    ///
    /// \brief Destroys the feature registry and owned feature modules.
    ///
    ~FeatureManager() override;

    ///
    /// \brief Takes ownership of a feature module.
    /// \param feature Feature to register.
    ///
    void registerFeature(FeatureModule *feature);

    ///
    /// \brief Initializes every feature in registration order.
    /// \param host Host services and contribution points.
    ///
    void initializeAll(FeatureHost &host);

    ///
    /// \brief Persists every feature's view state.
    /// \param settings Settings store to write to.
    ///
    void saveState(AppSettings &settings) const;

    ///
    /// \brief Restores every feature's view state.
    /// \param settings Settings store to read from.
    ///
    void restoreState(AppSettings &settings);

    ///
    /// \brief Clears runtime state in every feature.
    ///
    void clearRuntimeState();

    ///
    /// \brief Re-applies translated text owned by every feature after a language change.
    ///
    void retranslate();

    ///
    /// \brief Collects every feature's contribution to a saved working session.
    /// \param session Session payload to write to.
    ///
    void saveSession(SessionData &session) const;

    ///
    /// \brief Restores every feature's state from a loaded working session.
    /// \param session Session payload to read from.
    ///
    void restoreSession(const SessionData &session);

    ///
    /// \brief Records a dock placement contribution.
    /// \param area Default dock area.
    /// \param dock Dock widget.
    ///
    void addDock(Qt::DockWidgetArea area, QDockWidget *dock);

    ///
    /// \brief Records a dock split contribution.
    /// \param first Existing dock.
    /// \param second Dock to split from the first.
    /// \param orientation Split orientation.
    ///
    void splitDock(QDockWidget *first, QDockWidget *second, Qt::Orientation orientation);

    ///
    /// \brief Records a dock resize contribution.
    /// \param docks Docks to resize.
    /// \param sizes Target sizes.
    /// \param orientation Resize orientation.
    ///
    void resizeDocks(const QList<QDockWidget *> &docks,
                     const QList<int> &sizes,
                     Qt::Orientation orientation);

    ///
    /// \brief Replays the recorded default dock layout on a main window.
    /// \param window Main window receiving the layout.
    ///
    void resetDockLayout(QMainWindow &window) const;

    ///
    /// \brief Returns a contributed dock by object name.
    /// \param objectName Dock object name.
    /// \return Matching dock, or nullptr.
    ///
    QDockWidget *dock(const QString &objectName) const;

    ///
    /// \brief Registers a command callable by MainWindow.
    /// \param id Stable command identifier.
    /// \param command Command callback.
    ///
    void registerCommand(const QString &id, std::function<void()> command);

    ///
    /// \brief Runs a registered command.
    /// \param id Stable command identifier.
    /// \return True when a command was found and run.
    ///
    bool triggerCommand(const QString &id) const;

private:
    std::vector<std::unique_ptr<FeatureModule>> _features;
    QList<DockContribution> _dockContributions;
    QList<SplitContribution> _splitContributions;
    QList<ResizeContribution> _resizeContributions;
    QHash<QString, QDockWidget *> _docksByName;
    QHash<QString, std::function<void()>> _commands;
};
