// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacefeature.cpp
/// \brief Implements the address-space browser UI feature.
///

#include "addressspacefeature.h"

#include <QCoreApplication>
#include <QDockWidget>

#include "addressspaceplugin.h"
#include "appsettings.h"
#include "featurehost.h"
#include "opcua/standardnodeid.h"
#include "pluginmanager.h"
#include "referenceplugin.h"
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

    auto *addressSpacePlugin = host.dataPlugins()->plugin<AddressSpacePlugin>();
    auto *referencePlugin = host.dataPlugins()->plugin<ReferencePlugin>();

    QObject::connect(_widget, &AddressSpaceWidget::browseRequested,
                     addressSpacePlugin, &AddressSpacePlugin::browse);
    QObject::connect(_widget, &AddressSpaceWidget::refreshRequested,
                     addressSpacePlugin, &AddressSpacePlugin::refresh);
    QObject::connect(addressSpacePlugin, &AddressSpacePlugin::childrenReady,
                     _widget, &AddressSpaceWidget::setBrowseChildren);
    QObject::connect(_widget, &AddressSpaceWidget::referencesRequested,
                     referencePlugin, &ReferencePlugin::browseReferences);
    QObject::connect(referencePlugin, &ReferencePlugin::referencesReady,
                     _widget, &AddressSpaceWidget::setBrowseReferences);
    QObject::connect(_widget, &AddressSpaceWidget::nodeSelected,
                     host.selection(), &SelectionContext::selectNode);
    QObject::connect(host.selection(), &SelectionContext::detailsReady,
                     _widget, &AddressSpaceWidget::setNodeDetails);

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
