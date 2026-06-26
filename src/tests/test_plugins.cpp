// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_plugins.cpp
/// \brief Tests the module framework registration and the module data API.
///

#include <QLoggingCategory>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QTest>

#include "addressspacemodule.h"
#include "attributemodule.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "opcua/opcuaclientservice.h"
#include "servicecontext.h"
#include "servicemodulemanager.h"
#include "referencemodule.h"
#include "servermodule.h"

///
/// \brief Unit tests for the module manager and built-in modules.
///
class TestPlugins : public QObject
{
    Q_OBJECT

private slots:
    void registersAllPlugins();
    void pluginsHaveDistinctNamesAndCategories();
    void subscribeApiReachesClientService();
    void eventSubscribeApiReachesClientService();
};

namespace {

///
/// \brief Registers the six built-in modules with a manager.
/// \param manager Manager to populate.
///
void registerBuiltInPlugins(ServiceModuleManager &manager)
{
    manager.registerModule(new ServerModule);
    manager.registerModule(new AddressSpaceModule);
    manager.registerModule(new AttributeModule);
    manager.registerModule(new ReferenceModule);
    manager.registerModule(new DataAccessModule);
    manager.registerModule(new EventsModule);
}

} // namespace

///
/// \brief Every registered module is owned by the manager.
///
void TestPlugins::registersAllPlugins()
{
    ServiceModuleManager manager;
    registerBuiltInPlugins(manager);
    QCOMPARE(manager.modules().size(), 6);
}

///
/// \brief Each module exposes a distinct name and logging category.
///
void TestPlugins::pluginsHaveDistinctNamesAndCategories()
{
    ServiceModuleManager manager;
    registerBuiltInPlugins(manager);

    QSet<QString> names;
    QSet<QString> categories;
    for (ServiceModule *module : manager.modules()) {
        names.insert(module->name());
        categories.insert(QString::fromLatin1(module->logCategory().categoryName()));
    }
    QCOMPARE(names.size(), 6);
    QCOMPARE(categories.size(), 6);
}

///
/// \brief The DataAccessModule subscribe API forwards to the client service.
///
void TestPlugins::subscribeApiReachesClientService()
{
    OpcUaClientService service;
    DataAccessModule module;
    ServiceContext context(&service, nullptr);
    module.initialize(context);

    QSignalSpy spy(&module, &DataAccessModule::monitoringFinished);
    module.subscribe(QStringLiteral("ns=2;s=Temperature"), 500.0);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=2;s=Temperature"));
}

///
/// \brief The EventsModule subscribe API forwards to the client service.
///
void TestPlugins::eventSubscribeApiReachesClientService()
{
    OpcUaClientService service;
    EventsModule module;
    ServiceContext context(&service, nullptr);
    module.initialize(context);

    QSignalSpy spy(&module, &EventsModule::eventMonitoringFinished);
    module.subscribeEvents(QStringLiteral("ns=0;i=2253"), 500.0);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=0;i=2253"));
}

QTEST_MAIN(TestPlugins)

#include "test_plugins.moc"
