// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_log.cpp
/// \brief Tests LogModel filtering and retention behavior.
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
class TestModelsLog : public QObject
{
    Q_OBJECT

private slots:
    void logFilterByLevel();
    void logSearchFilterIsCaseInsensitive();
    void logLevelAndSearchCombine();
    void logModelDropsTheOldestBeyondItsDepth();
    void logModelDepthCountsHiddenRowsToo();
    void logModelLoweringTheDepthTrimsImmediately();
    void logModelColumnsRolesAndFilters();
};

///
/// \brief setFilterLevel keeps only matching rows; clearFilterLevel restores all.
///
void TestModelsLog::logFilterByLevel()
{
    LogModel model;
    new QAbstractItemModelTester(&model, &model);
    for (const TestData::LogEntry &entry : TestData::logItems())
        model.addItem({QString(), entry.level, entry.source, entry.message});

    const int total = TestData::logItems().size();
    int errorCount = 0;
    for (const TestData::LogEntry &entry : TestData::logItems())
        if (entry.level == LogItem::Level::Error)
            ++errorCount;

    QCOMPARE(model.rowCount(), total);
    model.setFilterLevel(LogItem::Level::Error);
    QCOMPARE(model.rowCount(), errorCount);
    model.clearFilterLevel();
    QCOMPARE(model.rowCount(), total);
}

///
/// \brief setSearchFilter matches the message substring case-insensitively.
///
void TestModelsLog::logSearchFilterIsCaseInsensitive()
{
    LogModel model;
    model.addItem({QString(), LogItem::Level::Info, "Client", "Connected to server"});
    model.addItem({QString(), LogItem::Level::Info, "Client", "Browse completed"});
    model.addItem({QString(), LogItem::Level::Error, "Client", "Read failed"});

    model.setSearchFilter(QStringLiteral("connect")); // lowercase vs "Connected"
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage)).toString(),
             QStringLiteral("Connected to server"));

    model.setSearchFilter(QString());
    QCOMPARE(model.rowCount(), 3);
}

///
/// \brief Level and search filters are combined with logical AND.
///
void TestModelsLog::logLevelAndSearchCombine()
{
    LogModel model;
    model.addItem({QString(), LogItem::Level::Warning, "Client", "timeout on read"});
    model.addItem({QString(), LogItem::Level::Error, "Client", "timeout on write"});
    model.addItem({QString(), LogItem::Level::Error, "Client", "bad node id"});

    model.setFilterLevel(LogItem::Level::Error);
    model.setSearchFilter(QStringLiteral("timeout"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage)).toString(),
             QStringLiteral("timeout on write"));
}

///
/// \brief The log keeps only its most recent entries once the depth is reached.
///
void TestModelsLog::logModelDropsTheOldestBeyondItsDepth()
{
    LogModel model;
    model.setMaxRows(3);
    QCOMPARE(model.maxRows(), 3);

    for (int index = 0; index < 5; ++index) {
        model.addItem({QStringLiteral("t%1").arg(index), LogItem::Level::Info,
                       QStringLiteral("Client"), QStringLiteral("m%1").arg(index)});
    }

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage)).toString(),
             QStringLiteral("m2"));
    QCOMPARE(model.data(model.index(2, LogModel::ColMessage)).toString(),
             QStringLiteral("m4"));
}

///
/// \brief The depth caps everything held, so a filtered view cannot hide unbounded growth.
///
void TestModelsLog::logModelDepthCountsHiddenRowsToo()
{
    LogModel model;
    model.setMaxRows(2);
    model.setFilterLevel(LogItem::Level::Error);

    model.addItem({QStringLiteral("t0"), LogItem::Level::Error,
                   QStringLiteral("Client"), QStringLiteral("boom")});
    // Two entries the filter hides; they still count against the depth and push the error out.
    model.addItem({QStringLiteral("t1"), LogItem::Level::Info,
                   QStringLiteral("Client"), QStringLiteral("chatter")});
    model.addItem({QStringLiteral("t2"), LogItem::Level::Info,
                   QStringLiteral("Client"), QStringLiteral("chatter")});

    QCOMPARE(model.rowCount(), 0);

    model.clearFilterLevel();
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, LogModel::ColTimestamp)).toString(),
             QStringLiteral("t1"));
}

///
/// \brief Lowering the depth trims what is already there instead of waiting for new entries.
///
void TestModelsLog::logModelLoweringTheDepthTrimsImmediately()
{
    LogModel model;
    for (int index = 0; index < 6; ++index) {
        model.addItem({QStringLiteral("t%1").arg(index), LogItem::Level::Info,
                       QStringLiteral("Client"), QStringLiteral("m%1").arg(index)});
    }
    QCOMPARE(model.rowCount(), 6);

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    model.setMaxRows(2);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(removedSpy.size(), 1);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage)).toString(),
             QStringLiteral("m4"));

    // A depth below one is meaningless; the model keeps at least the newest entry.
    model.setMaxRows(0);
    QCOMPARE(model.maxRows(), 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage)).toString(),
             QStringLiteral("m5"));
}

///
/// \brief LogModel: every column, the level colour, the filtered-add path and getters.
///
void TestModelsLog::logModelColumnsRolesAndFilters()
{
    LogModel model;
    model.addItem({QStringLiteral("t1"), LogItem::Level::Info,
                   QStringLiteral("Client"), QStringLiteral("Connected")});
    model.addItem({QStringLiteral("t2"), LogItem::Level::Warning,
                   QStringLiteral("Client"), QStringLiteral("Slow")});
    model.addItem({QStringLiteral("t3"), LogItem::Level::Error,
                   QStringLiteral("Client"), QStringLiteral("Failed")});

    QCOMPARE(model.headerData(LogModel::ColTimestamp, Qt::Horizontal).toString(),
             QStringLiteral("Time"));
    QCOMPARE(model.headerData(LogModel::ColLevel, Qt::Horizontal).toString(),
             QStringLiteral("Level"));
    QCOMPARE(model.headerData(LogModel::ColSource, Qt::Horizontal).toString(),
             QStringLiteral("Source"));
    QCOMPARE(model.headerData(LogModel::ColMessage, Qt::Horizontal).toString(),
             QStringLiteral("Message"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(LogModel::ColLevel, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, LogModel::ColTimestamp)).toString(),
             QStringLiteral("t1"));
    QCOMPARE(model.data(model.index(0, LogModel::ColLevel)).toString(),
             QStringLiteral("INFO"));
    QCOMPARE(model.data(model.index(1, LogModel::ColLevel)).toString(),
             QStringLiteral("WARN"));
    QCOMPARE(model.data(model.index(2, LogModel::ColLevel)).toString(),
             QStringLiteral("ERROR"));
    QCOMPARE(model.data(model.index(0, LogModel::ColSource)).toString(),
             QStringLiteral("Client"));
    QCOMPARE(model.data(model.index(2, LogModel::ColLevel), Qt::ForegroundRole)
                 .value<QColor>(), QColor(200, 40, 40));
    QVERIFY(model.data(model.index(0, LogModel::ColTimestamp),
                       Qt::TextAlignmentRole).isValid());

    // The getter reflects the active level filter.
    model.setFilterLevel(LogItem::Level::Error);
    QCOMPARE(model.filterLevel(), LogItem::Level::Error);

    // Adding a row that fails the active filter stores it but keeps it hidden.
    const int hiddenBefore = model.rowCount();
    model.addItem({QStringLiteral("t4"), LogItem::Level::Info,
                   QStringLiteral("Client"), QStringLiteral("Hidden")});
    QCOMPARE(model.rowCount(), hiddenBefore);
    model.clearFilterLevel();
    QVERIFY(model.rowCount() > hiddenBefore);

    model.setColumnAlignment(LogModel::ColMessage,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}


QTEST_MAIN(TestModelsLog)

#include "test_models_log.moc"
