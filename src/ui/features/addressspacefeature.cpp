// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacefeature.cpp
/// \brief Implements the address-space browser UI feature.
///

#include "addressspacefeature.h"
#include "featurecommands.h"

#include <QCoreApplication>
#include <QDockWidget>

#include "addressspacemodule.h"
#include "appsettings.h"
#include "dataaccessmodule.h"
#include "featurehost.h"
#include "session/sessiondata.h"
#include "opcua/standardnodeid.h"
#include "servicemodulemanager.h"
#include "referencemodule.h"
#include "selectioncontext.h"
#include "widgets/addressspacewidget.h"

///
/// \brief Label used by feature registration and translated dock commands.
///
QString AddressSpaceFeature::name() const
{
    return QCoreApplication::translate("AddressSpaceFeature", "Address Space");
}

///
/// \brief Builds the feature as a sequence of UI creation, wiring, layout, and commands.
///
void AddressSpaceFeature::initialize(FeatureHost &host)
{
    createDocks(host);
    wireModules(host);
    contributeLayout(host);
    registerCommands(host);
}

///
/// \brief Creates both docks around a single AddressSpaceWidget-owned details panel.
///
void AddressSpaceFeature::createDocks(FeatureHost &host)
{
    _widget = new AddressSpaceWidget;
    _addressDock = new QDockWidget(
        QCoreApplication::translate("AddressSpaceFeature", "Address Space"),
        host.mainWindow());
    _addressDock->setObjectName(QStringLiteral("addressSpaceDock"));
    _addressDock->setMinimumSize(300, 220);
    _addressDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    _addressDock->setWidget(_widget);

    _nodeDetailsDock = new QDockWidget(
        QCoreApplication::translate("AddressSpaceFeature", "Node Details"),
        host.mainWindow());
    _nodeDetailsDock->setObjectName(QStringLiteral("nodeDetailsDock"));
    _nodeDetailsDock->setMinimumSize(300, 180);
    _nodeDetailsDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    _nodeDetailsDock->setWidget(_widget->takeNodeDetailsPanel());
}

///
/// \brief Routes widget intents through modules and the shared selection context.
///
void AddressSpaceFeature::wireModules(FeatureHost &host)
{
    auto *addressSpaceModule = host.dataModules()->module<AddressSpaceModule>();
    auto *referenceModule = host.dataModules()->module<ReferenceModule>();
    auto *dataAccessModule = host.dataModules()->module<DataAccessModule>();

    QObject::connect(_widget, &AddressSpaceWidget::browseRequested,
                     addressSpaceModule, &AddressSpaceModule::browse);
    QObject::connect(_widget, &AddressSpaceWidget::refreshRequested,
                     addressSpaceModule, &AddressSpaceModule::refresh);
    QObject::connect(addressSpaceModule, &AddressSpaceModule::childrenReady,
                     _widget, &AddressSpaceWidget::setBrowseChildren);
    QObject::connect(_widget, &AddressSpaceWidget::searchRequested,
                     addressSpaceModule, &AddressSpaceModule::search);
    QObject::connect(_widget, &AddressSpaceWidget::searchCancelRequested,
                     addressSpaceModule, &AddressSpaceModule::cancelSearch);
    QObject::connect(addressSpaceModule, &AddressSpaceModule::searchProgress,
                     _widget, &AddressSpaceWidget::setSearchProgress);
    QObject::connect(addressSpaceModule, &AddressSpaceModule::searchFinished,
                     _widget, &AddressSpaceWidget::setSearchResult);
    QObject::connect(_widget, &AddressSpaceWidget::referencesRequested,
                     referenceModule, &ReferenceModule::browseReferences);
    QObject::connect(referenceModule, &ReferenceModule::referencesReady,
                     _widget, &AddressSpaceWidget::setBrowseReferences);
    QObject::connect(_widget, &AddressSpaceWidget::nodeSelected,
                     host.selection(), &SelectionContext::selectNode);
    QObject::connect(_widget, &AddressSpaceWidget::readHistoryRequested,
                     host.selection(), &SelectionContext::requestHistory);
    QObject::connect(_widget, &AddressSpaceWidget::monitorEventsRequested,
                     host.selection(), &SelectionContext::requestEventMonitor);
    QObject::connect(_widget, &AddressSpaceWidget::readEventsHistoryRequested,
                     host.selection(), &SelectionContext::requestEventsHistory);
    QObject::connect(_widget, &AddressSpaceWidget::addToTrendRequested,
                     host.selection(), &SelectionContext::requestAddToTrend);
    QObject::connect(_widget, &AddressSpaceWidget::monitorNodeRequested,
                     host.selection(), &SelectionContext::requestMonitorNode);
    QObject::connect(_widget, &AddressSpaceWidget::subscribeRequested,
                     host.selection(), &SelectionContext::requestSubscribe);
    QObject::connect(_widget, &AddressSpaceWidget::unsubscribeRequested,
                     host.selection(), &SelectionContext::requestUnsubscribe);
    QObject::connect(_widget, &AddressSpaceWidget::callMethodRequested,
                     host.selection(), &SelectionContext::requestCallMethod);
    QObject::connect(host.selection(), &SelectionContext::detailsReady,
                     _widget, &AddressSpaceWidget::setNodeDetails);
    if (dataAccessModule) {
        QObject::connect(dataAccessModule, &DataAccessModule::monitoringFinished, _widget,
                         [this](const QString &nodeId, bool subscribed, bool success, const QString &) {
            if (success)
                _widget->setNodeSubscribed(nodeId, subscribed);
        });
    }
}

///
/// \brief Contributes the default dock placement without persisting user layout state.
///
void AddressSpaceFeature::contributeLayout(FeatureHost &host)
{
    host.addDock(Qt::LeftDockWidgetArea, _addressDock);
    host.addDock(Qt::LeftDockWidgetArea, _nodeDetailsDock);
    host.splitDock(_addressDock, _nodeDetailsDock, Qt::Vertical);
    host.resizeDocks({_addressDock, _nodeDetailsDock}, {520, 360}, Qt::Vertical);

    if (QDockWidget *attributesDock = host.dock(QStringLiteral("attributesDock")))
        host.resizeDocks({_addressDock, attributesDock}, {300, 390}, Qt::Horizontal);
}

///
/// \brief Exposes address-space browsing to MainWindow actions without coupling to MainWindow.
///
void AddressSpaceFeature::registerCommands(FeatureHost &host)
{
    host.registerCommand(FeatureCommandIds::addressSpaceBrowse(), [this] {
        browseAddressSpace();
    });
}

///
/// \brief Stores tree/view presentation state, leaving live server data out of settings.
///
void AddressSpaceFeature::saveState(AppSettings &settings) const
{
    if (_widget)
        _widget->saveViewState(settings);
}

///
/// \brief Restores only presentation state saved by saveState().
///
void AddressSpaceFeature::restoreState(AppSettings &settings)
{
    if (_widget)
        _widget->restoreViewState(settings);
}

///
/// \brief Clears node data that belongs to the active OPC UA session.
///
void AddressSpaceFeature::clearRuntimeState()
{
    if (_widget)
        _widget->clear();
}

///
/// \brief Adds address-space navigation state to an already collected session payload.
///
void AddressSpaceFeature::saveSession(SessionData &session) const
{
    if (!_widget)
        return;
    session.expandedNodes = _widget->expandedNodeIds();
    session.selectedNode = _widget->selectedNode().nodeId;
}

///
/// \brief Replays saved tree expansion and selection after the session workspace is loaded.
///
void AddressSpaceFeature::restoreSession(const SessionData &session)
{
    if (_widget)
        _widget->restoreExpansion(session.expandedNodes, session.selectedNode);
}

///
/// \brief Seeds browsing from the standard Objects folder.
///
void AddressSpaceFeature::browseAddressSpace()
{
    if (!_widget)
        return;

    OpcUaNodeInfo root;
    root.nodeId = QString::fromLatin1(StandardNodeId::ObjectsFolder);
    root.browseName = QCoreApplication::translate("AddressSpaceFeature", "Root");
    root.displayName = QCoreApplication::translate("AddressSpaceFeature", "Root");
    root.nodeClass = 1;
    root.hasChildren = true;
    _widget->setRootNode(root);
}
