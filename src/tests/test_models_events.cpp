// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_events.cpp
/// \brief Tests EventsModel behavior.
///

#include <QAbstractItemModelTester>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QMimeData>
#include <QPalette>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>
#include <QTimeZone>

#include "appsettings.h"
#include "testdata.h"
#include "testimages.h"
#include "formatters/attributeformatter.h"
#include "opcua/opcuatypes.h"
#include "models/attributesmodel.h"
#include "models/csvexporter.h"
#include "models/dataaccessmodel.h"
#include "models/eventsmodel.h"
#include "models/historymodel.h"
#include "models/logmodel.h"
#include "models/nodeinfomodel.h"
#include "models/referencesmodel.h"
#include "models/subscriptionsmodel.h"
#include "models/valueroles.h"

using TestImages::encodedPng;

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestModelsEvents : public QObject
{
    Q_OBJECT

private slots:
    void eventsModelAddEventsAppendsAndCaps();
    void eventsModelExportsCsv();
    void eventsModelDisplaysKnownEventTypeNames();
    void eventsModelHeaderRolesAndMutators();
};

///
/// \brief EventsModel::addEvents appends rows and caps the table size.
///
void TestModelsEvents::eventsModelAddEventsAppendsAndCaps()
{
    EventsModel model;
    model.addEvents({{QStringLiteral("12:00"), QStringLiteral("100"),
                      QStringLiteral("Server"), QStringLiteral("First"),
                      QStringLiteral("BaseEventType")}});
    QCOMPARE(model.rowCount(), 1);

    model.addEvents({{QStringLiteral("12:01"), QStringLiteral("200"),
                      QStringLiteral("Server"), QStringLiteral("Second"),
                      QStringLiteral("BaseEventType")}});
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(1, EventsModel::ColMessage)).toString(),
             QStringLiteral("Second"));

    QVector<EventItem> many;
    for (int i = 0; i < 1100; ++i)
        many.append({QStringLiteral("t"), QStringLiteral("1"),
                     QStringLiteral("Server"), QStringLiteral("E%1").arg(i),
                     QStringLiteral("BaseEventType")});
    model.addEvents(many);
    QCOMPARE(model.rowCount(), 1000);
}

///
/// \brief EventsModel exports displayed event rows as escaped CSV.
///
void TestModelsEvents::eventsModelExportsCsv()
{
    EventsModel model;
    model.setItems({{QStringLiteral("2026-06-26 11:05:20.335+03:00"),
                     QStringLiteral("500"),
                     QStringLiteral("MyLevel"),
                     QStringLiteral("Level, \"exceeded\"\nagain"),
                     QStringLiteral("ns=0;i=9482")}});

    QCOMPARE(model.toCsv(),
             QStringLiteral("Time,Severity,Source,Message,Event Type\n"
                            "2026-06-26 11:05:20.335+03:00,500,MyLevel,"
                            "\"Level, \"\"exceeded\"\"\nagain\",ExclusiveLevelAlarmType\n"));
}

///
/// \brief EventsModel shows known namespace-0 EventType NodeIds as BrowseNames.
///
void TestModelsEvents::eventsModelDisplaysKnownEventTypeNames()
{
    EventsModel model;
    model.setItems({{QStringLiteral("12:00"), QStringLiteral("500"),
                     QStringLiteral("MyLevel"), QStringLiteral("Level exceeded"),
                     QStringLiteral("ns=0;i=9482")},
                    {QStringLiteral("12:01"), QStringLiteral("500"),
                     QStringLiteral("Custom"), QStringLiteral("Custom event"),
                     QStringLiteral("ns=2;s=CustomAlarmType")}});

    QCOMPARE(model.data(model.index(0, EventsModel::ColEventType)).toString(),
             QStringLiteral("ExclusiveLevelAlarmType"));
    QCOMPARE(model.data(model.index(1, EventsModel::ColEventType)).toString(),
             QStringLiteral("ns=2;s=CustomAlarmType"));
}

///
/// \brief EventsModel: headerData, the message column, alignment role and mutator.
///
void TestModelsEvents::eventsModelHeaderRolesAndMutators()
{
    EventsModel model;
    model.setItems({{QStringLiteral("12:00"), QStringLiteral("100"),
                     QStringLiteral("Server"), QStringLiteral("Started"),
                     QStringLiteral("SystemStatusChangeEventType")},
                    {QStringLiteral("12:01"), QStringLiteral("100"),
                     QStringLiteral("Server"), QStringLiteral("Stopped"),
                     QStringLiteral("SystemStatusChangeEventType")}});

    QCOMPARE(model.headerData(EventsModel::ColTime, Qt::Horizontal).toString(),
             QStringLiteral("Time"));
    QCOMPARE(model.headerData(EventsModel::ColMessage, Qt::Horizontal).toString(),
             QStringLiteral("Message"));
    QCOMPARE(model.headerData(EventsModel::ColSeverity, Qt::Horizontal).toString(),
             QStringLiteral("Severity"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(EventsModel::ColTime, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, EventsModel::ColMessage)).toString(),
             QStringLiteral("Started"));
    QCOMPARE(model.data(model.index(0, EventsModel::ColSource)).toString(),
             QStringLiteral("Server"));
    QVERIFY(model.data(model.index(0, EventsModel::ColTime),
                       Qt::TextAlignmentRole).isValid());

    model.setColumnAlignment(EventsModel::ColMessage,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(model.index(0, EventsModel::ColMessage),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));
}


QTEST_MAIN(TestModelsEvents)

#include "test_models_events.moc"
