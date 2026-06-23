// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_plugins.cpp
/// \brief Tests the plugin framework registration and the plugin data API.
///

#include <QLoggingCategory>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QTest>

#include "addressspaceplugin.h"
#include "attributeplugin.h"
#include "dataaccessplugin.h"
#include "opcua/opcuaclientservice.h"
#include "plugincontext.h"
#include "pluginmanager.h"
#include "referenceplugin.h"
#include "serverplugin.h"

///
/// \brief Unit tests for the plugin manager and built-in plugins.
///
class TestPlugins : public QObject
{
    Q_OBJECT

private slots:
    void registersAllPlugins();
    void pluginsHaveDistinctNamesAndCategories();
    void subscribeApiReachesClientService();
};

namespace {

///
/// \brief Registers the five built-in plugins with a manager.
/// \param manager Manager to populate.
///
void registerBuiltInPlugins(PluginManager &manager)
{
    manager.registerPlugin(new ServerPlugin);
    manager.registerPlugin(new AddressSpacePlugin);
    manager.registerPlugin(new AttributePlugin);
    manager.registerPlugin(new ReferencePlugin);
    manager.registerPlugin(new DataAccessPlugin);
}

} // namespace

///
/// \brief Every registered plugin is owned by the manager.
///
void TestPlugins::registersAllPlugins()
{
    PluginManager manager;
    registerBuiltInPlugins(manager);
    QCOMPARE(manager.plugins().size(), 5);
}

///
/// \brief Each plugin exposes a distinct name and logging category.
///
void TestPlugins::pluginsHaveDistinctNamesAndCategories()
{
    PluginManager manager;
    registerBuiltInPlugins(manager);

    QSet<QString> names;
    QSet<QString> categories;
    for (Plugin *plugin : manager.plugins()) {
        names.insert(plugin->name());
        categories.insert(QString::fromLatin1(plugin->logCategory().categoryName()));
    }
    QCOMPARE(names.size(), 5);
    QCOMPARE(categories.size(), 5);
}

///
/// \brief The DataAccessPlugin subscribe API forwards to the client service.
///
void TestPlugins::subscribeApiReachesClientService()
{
    OpcUaClientService service;
    DataAccessPlugin plugin;
    PluginContext context(&service, nullptr);
    plugin.initialize(context);

    QSignalSpy spy(&plugin, &DataAccessPlugin::monitoringFinished);
    plugin.subscribe(QStringLiteral("ns=2;s=Temperature"), 500.0);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=2;s=Temperature"));
}

QTEST_MAIN(TestPlugins)

#include "test_plugins.moc"
