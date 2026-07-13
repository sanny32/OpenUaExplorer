// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modules.cpp
/// \brief Tests the module framework registration and the module data API.
///

#include <QLoggingCategory>
#include <QDateTime>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QTimeZone>

#include "addressspacemodule.h"
#include "attributemodule.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "opcua/opcuabackend.h"
#include "opcua/qtopcuabackend.h"
#include "servicecontext.h"
#include "servicemodulemanager.h"
#include "referencemodule.h"
#include "servermodule.h"

///
/// \brief Unit tests for the module manager and built-in modules.
///
class TestModules : public QObject
{
    Q_OBJECT

private slots:
    void registersAllModules();
    void modulesHaveDistinctNamesAndCategories();
    void subscribeApiReachesBackend();
    void eventSubscribeApiReachesBackend();
    void eventHistoryApiReachesBackend();
};

namespace {

///
/// \brief Registers the six built-in modules with a manager.
/// \param manager Manager to populate.
///
void registerBuiltInModules(ServiceModuleManager &manager)
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
void TestModules::registersAllModules()
{
    ServiceModuleManager manager;
    registerBuiltInModules(manager);
    QCOMPARE(manager.modules().size(), 6);
}

///
/// \brief Each module exposes a distinct name and logging category.
///
void TestModules::modulesHaveDistinctNamesAndCategories()
{
    ServiceModuleManager manager;
    registerBuiltInModules(manager);

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
/// \brief The DataAccessModule subscribe API forwards to the backend.
///
void TestModules::subscribeApiReachesBackend()
{
    QtOpcUaBackend service;
    DataAccessModule module;
    ServiceContext context(&service, nullptr);
    module.initialize(context);

    QSignalSpy spy(&module, &DataAccessModule::monitoringFinished);
    module.subscribe(QStringLiteral("ns=2;s=Temperature"), 500.0);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=2;s=Temperature"));
}

///
/// \brief The EventsModule subscribe API forwards to the backend.
///
void TestModules::eventSubscribeApiReachesBackend()
{
    QtOpcUaBackend service;
    EventsModule module;
    ServiceContext context(&service, nullptr);
    module.initialize(context);

    QSignalSpy spy(&module, &EventsModule::eventMonitoringFinished);
    module.subscribeEvents(QStringLiteral("ns=0;i=2253"), 500.0);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=0;i=2253"));
}

///
/// \brief The EventsModule event history API forwards to the backend.
///
void TestModules::eventHistoryApiReachesBackend()
{
    QtOpcUaBackend service;
    EventsModule module;
    ServiceContext context(&service, nullptr);
    module.initialize(context);

    QSignalSpy spy(&module, &EventsModule::eventsHistoryReady);
    module.readHistory(QStringLiteral("ns=0;i=2253"),
                       QDateTime(QDate(2026, 6, 25), QTime(12, 0), QTimeZone::UTC),
                       QDateTime(QDate(2026, 6, 25), QTime(13, 0), QTimeZone::UTC),
                       1000);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=0;i=2253"));
}

QTEST_MAIN(TestModules)

#include "test_modules.moc"
