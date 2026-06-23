// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logfeature.cpp
/// \brief Implements the activity-log UI feature.
///

#include "logfeature.h"

#include <QCoreApplication>
#include <QDockWidget>

#include "appsettings.h"
#include "featurehost.h"
#include "widgets/logwidget.h"

///
/// \brief Returns the human-readable feature name.
/// \return Feature name.
///
QString LogFeature::name() const
{
    return QCoreApplication::translate("LogFeature", "Log");
}

///
/// \brief Creates the feature UI and wires it to host services.
/// \param host Host services and contribution points.
///
void LogFeature::initialize(FeatureHost &host)
{
    _widget = new LogWidget;
    _dock = new QDockWidget(QCoreApplication::translate("LogFeature", "Log"),
                            host.mainWindow());
    _dock->setObjectName(QStringLiteral("logDock"));
    _dock->setMinimumSize(640, 190);
    _dock->setWidget(_widget);

    host.addDock(Qt::BottomDockWidgetArea, _dock);
    host.resizeDocks({_dock}, {245}, Qt::Vertical);
}

///
/// \brief Persists feature-owned view state.
/// \param settings Settings store to write to.
///
void LogFeature::saveState(AppSettings &settings) const
{
    if (_widget)
        _widget->saveViewState(settings);
}

///
/// \brief Restores feature-owned view state.
/// \param settings Settings store to read from.
///
void LogFeature::restoreState(AppSettings &settings)
{
    if (_widget)
        _widget->restoreViewState(settings);
}
