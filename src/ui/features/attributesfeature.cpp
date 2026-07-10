// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributesfeature.cpp
/// \brief Implements the selected-node attributes UI feature.
///

#include "attributesfeature.h"

#include <QCoreApplication>
#include <QDockWidget>

#include "application.h"
#include "appsettings.h"
#include "attributemodule.h"
#include "featurehost.h"
#include "servicemodulemanager.h"
#include "selectioncontext.h"
#include "widgets/attributeswidget.h"

///
/// \brief Returns the human-readable feature name.
/// \return Feature name.
///
QString AttributesFeature::name() const
{
    return QCoreApplication::translate("AttributesFeature", "Attributes");
}

///
/// \brief Creates the feature UI and wires it to host services.
/// \param host Host services and contribution points.
///
void AttributesFeature::initialize(FeatureHost &host)
{
    _widget = new AttributesWidget;
    _dock = new QDockWidget(QCoreApplication::translate("AttributesFeature", "Attributes"),
                            host.mainWindow());
    _dock->setObjectName(QStringLiteral("attributesDock"));
    _dock->setMinimumSize(390, 220);
    _dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    _dock->setWidget(_widget);

    auto *attributeModule = host.dataModules()->module<AttributeModule>();
    SelectionContext *selection = host.selection();

    QObject::connect(_widget, &AttributesWidget::writeRequested,
                     attributeModule, &AttributeModule::write);
    QObject::connect(selection, &SelectionContext::detailsReady,
                     _widget, &AttributesWidget::setNodeDetails);

    QObject::connect(selection, &SelectionContext::nodeSelected,
                     attributeModule, [attributeModule](const OpcUaNodeInfo &node) {
                         if (!node.nodeId.isEmpty())
                             attributeModule->read(node.nodeId);
                     });
    QObject::connect(attributeModule, &AttributeModule::attributesReady,
                     selection, &SelectionContext::setDetails);
    QObject::connect(theApp(), &Application::timestampModeChanged,
                     _widget, &AttributesWidget::setTimestampMode);

    host.addDock(Qt::RightDockWidgetArea, _dock);
}

///
/// \brief Persists feature-owned view state.
/// \param settings Settings store to write to.
///
void AttributesFeature::saveState(AppSettings &settings) const
{
    if (_widget)
        _widget->saveViewState(settings);
}

///
/// \brief Restores feature-owned view state.
/// \param settings Settings store to read from.
///
void AttributesFeature::restoreState(AppSettings &settings)
{
    if (_widget)
        _widget->restoreViewState(settings);
}

///
/// \brief Clears runtime data when the OPC UA session is no longer usable.
///
void AttributesFeature::clearRuntimeState()
{
    if (_widget)
        _widget->clear();
}
