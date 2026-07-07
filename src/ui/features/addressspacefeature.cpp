// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacefeature.cpp
/// \brief Implements the address-space browser UI feature.
///

#include "addressspacefeature.h"

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
/// \brief Returns the human-readable feature name.
/// \return Feature name.
///
QString AddressSpaceFeature::name() const
{
    return QCoreApplication::translate("AddressSpaceFeature", "Address Space");
}

///
/// \brief Creates the feature UI and wires it to host services.
/// \param host Host services and contribution points.
///
void AddressSpaceFeature::initialize(FeatureHost &host)
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

    auto *addressSpacePlugin = host.dataModules()->module<AddressSpaceModule>();
    auto *referencePlugin = host.dataModules()->module<ReferenceModule>();
    auto *dataAccessPlugin = host.dataModules()->module<DataAccessModule>();

    QObject::connect(_widget, &AddressSpaceWidget::browseRequested,
                     addressSpacePlugin, &AddressSpaceModule::browse);
    QObject::connect(_widget, &AddressSpaceWidget::refreshRequested,
                     addressSpacePlugin, &AddressSpaceModule::refresh);
    QObject::connect(addressSpacePlugin, &AddressSpaceModule::childrenReady,
                     _widget, &AddressSpaceWidget::setBrowseChildren);
    QObject::connect(_widget, &AddressSpaceWidget::referencesRequested,
                     referencePlugin, &ReferenceModule::browseReferences);
    QObject::connect(referencePlugin, &ReferenceModule::referencesReady,
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
    if (dataAccessPlugin) {
        QObject::connect(dataAccessPlugin, &DataAccessModule::monitoringFinished, _widget,
                         [this](const QString &nodeId, bool subscribed, bool success, const QString &) {
            if (success)
                _widget->setNodeSubscribed(nodeId, subscribed);
        });
    }

    host.addDock(Qt::LeftDockWidgetArea, _addressDock);
    host.addDock(Qt::LeftDockWidgetArea, _nodeDetailsDock);
    host.splitDock(_addressDock, _nodeDetailsDock, Qt::Vertical);
    host.resizeDocks({_addressDock, _nodeDetailsDock}, {520, 360}, Qt::Vertical);

    if (QDockWidget *attributesDock = host.dock(QStringLiteral("attributesDock")))
        host.resizeDocks({_addressDock, attributesDock}, {300, 390}, Qt::Horizontal);

    host.registerCommand(QStringLiteral("addressSpace.browse"), [this] {
        browseAddressSpace();
    });
}

///
/// \brief Persists feature-owned view state.
/// \param settings Settings store to write to.
///
void AddressSpaceFeature::saveState(AppSettings &settings) const
{
    if (_widget)
        _widget->saveViewState(settings);
}

///
/// \brief Restores feature-owned view state.
/// \param settings Settings store to read from.
///
void AddressSpaceFeature::restoreState(AppSettings &settings)
{
    if (_widget)
        _widget->restoreViewState(settings);
}

///
/// \brief Clears runtime data when the OPC UA session is no longer usable.
///
void AddressSpaceFeature::clearRuntimeState()
{
    if (_widget)
        _widget->clear();
}

///
/// \brief Saves the expanded tree nodes and selected node into the session.
/// \param session Session payload to write to.
///
void AddressSpaceFeature::saveSession(SessionData &session) const
{
    if (!_widget)
        return;
    session.expandedNodes = _widget->expandedNodeIds();
    session.selectedNode = _widget->selectedNode().nodeId;
}

///
/// \brief Restores the expanded tree nodes and selected node from the session.
/// \param session Session payload to read from.
///
void AddressSpaceFeature::restoreSession(const SessionData &session)
{
    if (_widget)
        _widget->restoreExpansion(session.expandedNodes, session.selectedNode);
}

///
/// \brief Seeds the address-space tree with the Objects-folder root node.
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
