// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_data_access_presentation.cpp
/// \brief Tests DataAccessModel formatting, state, arrays, and highlighting.
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
class TestModelsDataAccessPresentation : public QObject
{
    Q_OBJECT

private slots:
    void dataAccessTracksValueChanges();
    void dataAccessResolvesHighlightPreference();
    void dataAccessExposesQualityAndValueFont();
    void dataAccessFormatsTypedValues();
    void dataAccessElementRowsFollowTheirArray();
    void dataAccessFlashesOnlyTheElementsThatChanged();
    void dataAccessCapsTheElementRowsOfHugeArrays();
    void dataAccessValueCellFollowsTheInlineArrayPreference();
    void dataAccessTimestampModeReformats();
    void dataAccessSummarisesImagesAndOffersTheirBytes();
    void dataAccessOfflineGreysRowsAndLocksEditing();
};

///
/// \brief Only a value that actually differs stamps the change time.
///
void TestModelsDataAccessPresentation::dataAccessTracksValueChanges()
{
    DataAccessModel model;
    model.setItems(TestData::dataAccessItems());
    const QModelIndex valueIndex = model.index(0, DataAccessModel::ColValue);
    QCOMPARE(valueIndex.data(DataAccessModel::ValueChangedAtRole).toLongLong(), 0);

    OpcUaDataValue value;
    value.nodeId = TestData::dataAccessItems().first().nodeId;
    value.value = 42.0;
    value.status = QStringLiteral("Good");
    model.updateValues({value});

    const qint64 firstChange = valueIndex.data(DataAccessModel::ValueChangedAtRole).toLongLong();
    QVERIFY(firstChange > 0);

    // A notification repeating the value must not restart the highlight.
    QTest::qWait(5);
    model.updateValues({value});
    QCOMPARE(valueIndex.data(DataAccessModel::ValueChangedAtRole).toLongLong(), firstChange);

    QTest::qWait(5);
    value.value = 43.0;
    model.updateValues({value});
    QVERIFY(valueIndex.data(DataAccessModel::ValueChangedAtRole).toLongLong() > firstChange);

    // The attribute-read path stamps changes the same way.
    OpcUaNodeDetails details;
    details.nodeId = value.nodeId;
    details.value = 44.0;
    const qint64 beforeRead = valueIndex.data(DataAccessModel::ValueChangedAtRole).toLongLong();
    QTest::qWait(5);
    model.addOrUpdate(details);
    QVERIFY(valueIndex.data(DataAccessModel::ValueChangedAtRole).toLongLong() > beforeRead);
}

///
/// \brief Rows follow the default highlight preference until overridden individually.
///
void TestModelsDataAccessPresentation::dataAccessResolvesHighlightPreference()
{
    DataAccessModel model;
    model.setItems(TestData::dataAccessItems());
    const QModelIndex first = model.index(0, DataAccessModel::ColValue);
    const QModelIndex second = model.index(1, DataAccessModel::ColValue);

    // Highlighting is opt-in, so an untouched model leaves every row alone.
    QVERIFY(!first.data(DataAccessModel::HighlightChangesRole).toBool());

    model.setDefaultHighlightChanges(true);
    QVERIFY(first.data(DataAccessModel::HighlightChangesRole).toBool());
    QCOMPARE(model.highlightMode(0), HighlightMode::FollowDefault);

    QSignalSpy changeSpy(&model, &QAbstractItemModel::dataChanged);
    model.setHighlightMode({first}, HighlightMode::Disabled);
    QCOMPARE(changeSpy.size(), 1);
    QVERIFY(!first.data(DataAccessModel::HighlightChangesRole).toBool());
    QVERIFY(second.data(DataAccessModel::HighlightChangesRole).toBool());
    QVERIFY(!model.highlightsChanges(0));
    QVERIFY(model.highlightsChanges(1));

    // Re-applying the same override changes nothing and stays silent.
    model.setHighlightMode({first}, HighlightMode::Disabled);
    QCOMPARE(changeSpy.size(), 1);

    // The default reaches rows that follow it, and leaves the overridden one alone.
    model.setDefaultHighlightChanges(false);
    QVERIFY(!second.data(DataAccessModel::HighlightChangesRole).toBool());
    model.setHighlightMode({second}, HighlightMode::Enabled);
    QVERIFY(second.data(DataAccessModel::HighlightChangesRole).toBool());
    QVERIFY(!first.data(DataAccessModel::HighlightChangesRole).toBool());

    // Out-of-range rows fall back to the default instead of asserting.
    model.setHighlightMode({model.index(99, 0)}, HighlightMode::Enabled);
    QCOMPARE(model.highlightMode(99), HighlightMode::FollowDefault);
    QVERIFY(!model.highlightsChanges(99));
}

///
/// \brief The quality role and the value font carry what the value delegate paints with.
///
void TestModelsDataAccessPresentation::dataAccessExposesQualityAndValueFont()
{
    DataAccessModel model;
    model.setItems(TestData::dataAccessItems());

    const QModelIndex monitored = model.index(0, DataAccessModel::ColValue);
    QCOMPARE(monitored.data(DataAccessModel::StatusSeverityRole).toInt(),
             int(OpcUaFormat::StatusSeverity::Good));

    // The interval bounds the change wash, and only counts once the server granted one.
    QCOMPARE(monitored.data(DataAccessModel::ExpectedIntervalRole).toDouble(), 0.0);
    model.setRevisedInterval(TestData::dataAccessItems().first().nodeId, 250.0);
    QCOMPARE(monitored.data(DataAccessModel::ExpectedIntervalRole).toDouble(), 250.0);

    // An unmonitored row reports no interval even when one lingers on the item.
    const int unmonitoredRow = 3;
    QVERIFY(model.itemAt(unmonitoredRow).subscriptionName.isEmpty());
    model.setRevisedInterval(model.itemAt(unmonitoredRow).nodeId, 250.0);
    QCOMPARE(model.index(unmonitoredRow, DataAccessModel::ColValue)
                 .data(DataAccessModel::ExpectedIntervalRole).toDouble(), 0.0);

    OpcUaDataValue uncertain;
    uncertain.nodeId = TestData::dataAccessItems().first().nodeId;
    uncertain.value = 1.0;
    uncertain.status = QStringLiteral("UncertainLastUsableValue");
    model.updateValues({uncertain});
    QCOMPARE(monitored.data(DataAccessModel::StatusSeverityRole).toInt(),
             int(OpcUaFormat::StatusSeverity::Uncertain));

    OpcUaDataValue bad = uncertain;
    bad.value = 2.0;
    bad.status = QStringLiteral("BadNodeIdUnknown");
    model.updateValues({bad});
    QCOMPARE(monitored.data(DataAccessModel::StatusSeverityRole).toInt(),
             int(OpcUaFormat::StatusSeverity::Bad));

    // Values use the interface font; only pending rows carry a font override.
    QVERIFY(!monitored.data(Qt::FontRole).isValid());
    QVERIFY(!model.index(0, DataAccessModel::ColDisplayName).data(Qt::FontRole).isValid());

    OpcUaNodeInfo pending;
    pending.nodeId = QStringLiteral("ns=2;s=Pending");
    pending.displayName = QStringLiteral("Pending");
    model.addPending(pending);
    const QModelIndex pendingValue = model.index(model.rowCount() - 1, DataAccessModel::ColValue);
    QVERIFY(pendingValue.data(Qt::FontRole).value<QFont>().italic());
}

///
/// \brief DataAccessModel formats byte-sized numbers and compound values consistently.
///
void TestModelsDataAccessPresentation::dataAccessFormatsTypedValues()
{
    DataAccessModel model;

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Byte");
    details.value = QVariant::fromValue<quint8>(122);
    model.addOrUpdate(details);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue)).toString(),
             QStringLiteral("122"));

    details.nodeId = QStringLiteral("ns=2;s=SByte");
    details.value = QVariant::fromValue<qint8>(-6);
    model.addOrUpdate(details);
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColValue)).toString(),
             QStringLiteral("-6"));

    details.nodeId = QStringLiteral("ns=2;s=ByteString");
    details.value = QByteArray::fromHex("01ab");
    model.addOrUpdate(details);
    QCOMPARE(model.data(model.index(2, DataAccessModel::ColValue)).toString(),
             QStringLiteral("01 ab"));

    OpcUaDataValue update;
    update.nodeId = details.nodeId;
    update.value = QVariantList{QVariant::fromValue<quint8>(122),
                                QVariant::fromValue<qint8>(-6)};
    model.updateValues({update});
    // A short array still fits in the cell, and spells its elements out in its child rows too.
    const QModelIndex arrayIndex = model.index(2, DataAccessModel::ColValue);
    QCOMPARE(model.data(arrayIndex).toString(), QStringLiteral("[122, -6]"));
    const QModelIndex arrayRow = model.index(2, 0);
    QCOMPARE(model.rowCount(arrayRow), 2);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColNodeId, arrayRow)).toString(),
             QStringLiteral("[0]"));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue, arrayRow)).toString(),
             QStringLiteral("122"));
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColValue, arrayRow)).toString(),
             QStringLiteral("-6"));
}

///
/// \brief An array row expands into one labelled child row per element.
///
void TestModelsDataAccessPresentation::dataAccessElementRowsFollowTheirArray()
{
    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Values");
    details.dataTypeId = QStringLiteral("ns=0;i=4");
    details.value = QVariantList{QVariant::fromValue<qint16>(1), QVariant::fromValue<qint16>(2),
                                 QVariantList{QVariant::fromValue<qint16>(3)}};
    model.addOrUpdate(details);

    const QModelIndex row = model.index(0, 0);
    QVERIFY(model.hasChildren(row));
    QCOMPARE(model.rowCount(row), 3);
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColNodeId, row)).toString(),
             QStringLiteral("[1]"));
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColValue, row)).toString(),
             QStringLiteral("2"));
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColDataType, row)).toString(),
             QStringLiteral("Int16"));

    // A nested array keeps expanding, and its own cell names it.
    const QModelIndex nested = model.index(2, 0, row);
    QVERIFY(model.hasChildren(nested));
    QCOMPARE(model.rowCount(nested), 1);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue, nested)).toString(),
             QStringLiteral("3"));
    QCOMPARE(model.parent(nested), row);

    // A scalar row stays a leaf.
    details.nodeId = QStringLiteral("ns=2;s=Scalar");
    details.value = 7;
    model.addOrUpdate(details);
    QVERIFY(!model.hasChildren(model.index(1, 0)));
    QCOMPARE(model.rowCount(model.index(1, 0)), 0);
}

///
/// \brief An update stamps the change time only on the elements whose value moved.
///
void TestModelsDataAccessPresentation::dataAccessFlashesOnlyTheElementsThatChanged()
{
    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Values");
    details.dataTypeId = QStringLiteral("ns=0;i=4");
    details.value = QVariantList{1, 2};
    model.addOrUpdate(details);

    const QModelIndex row = model.index(0, 0);
    QCOMPARE(model.rowCount(row), 2);
    QCOMPARE(model.index(0, DataAccessModel::ColValue, row)
                 .data(DataAccessModel::ValueChangedAtRole).toLongLong(), 0);

    OpcUaDataValue update;
    update.nodeId = details.nodeId;
    update.value = QVariantList{1, 5};
    model.updateValues({update});

    QCOMPARE(model.rowCount(row), 2);
    QCOMPARE(model.index(0, DataAccessModel::ColValue, row)
                 .data(DataAccessModel::ValueChangedAtRole).toLongLong(), 0);
    QVERIFY(model.index(1, DataAccessModel::ColValue, row)
                .data(DataAccessModel::ValueChangedAtRole).toLongLong() > 0);
    QCOMPARE(model.index(1, DataAccessModel::ColValue, row).data().toString(),
             QStringLiteral("5"));

    // A shorter array replaces its rows rather than leaving stale ones behind.
    update.value = QVariantList{9};
    model.updateValues({update});
    QCOMPARE(model.rowCount(row), 1);
    QCOMPARE(model.index(0, DataAccessModel::ColValue, row).data().toString(),
             QStringLiteral("9"));
}

///
/// \brief A huge array shows its first elements and sums up the rest in one row.
///
void TestModelsDataAccessPresentation::dataAccessCapsTheElementRowsOfHugeArrays()
{
    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);

    QVariantList values;
    const int count = DataAccessModel::MaxExpandedElements + 5;
    for (int index = 0; index < count; ++index)
        values.append(index);

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Big");
    details.dataTypeId = QStringLiteral("ns=0;i=6");
    details.value = values;
    model.addOrUpdate(details);

    const QModelIndex row = model.index(0, 0);
    QCOMPARE(model.rowCount(row), DataAccessModel::MaxExpandedElements + 1);

    const QModelIndex last =
        model.index(DataAccessModel::MaxExpandedElements, DataAccessModel::ColNodeId, row);
    QVERIFY(last.data().toString().contains(QStringLiteral("5")));
    QVERIFY(model.data(model.index(last.row(), DataAccessModel::ColValue, row))
                .toString().isEmpty());
    QVERIFY(!model.hasChildren(last));
}

///
/// \brief The value cell spells out as many array elements as the preference allows.
///
void TestModelsDataAccessPresentation::dataAccessValueCellFollowsTheInlineArrayPreference()
{
    DataAccessModel model;

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Values");
    details.dataTypeId = QStringLiteral("ns=0;i=4");
    details.value = QVariantList{QVariant::fromValue<qint16>(1), QVariant::fromValue<qint16>(2),
                                 QVariant::fromValue<qint16>(3)};
    model.addOrUpdate(details);

    const QModelIndex value = model.index(0, DataAccessModel::ColValue);
    QCOMPARE(value.data().toString(), QStringLiteral("[1, 2, 3]"));

    QSignalSpy changeSpy(&model, &QAbstractItemModel::dataChanged);
    model.setInlineArrayElements(2);
    QCOMPARE(changeSpy.size(), 1);
    QCOMPARE(value.data().toString(), QStringLiteral("Int16[3]"));

    // Re-applying the same preference changes nothing and stays silent.
    model.setInlineArrayElements(2);
    QCOMPARE(changeSpy.size(), 1);

    model.setInlineArrayElements(3);
    QCOMPARE(value.data().toString(), QStringLiteral("[1, 2, 3]"));
}

///
/// \brief Toggling the timestamp mode reformats the source-timestamp column live.
///
void TestModelsDataAccessPresentation::dataAccessTimestampModeReformats()
{
    DataAccessModel model;
    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=TS");
    details.sourceTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5, 678), QTimeZone::UTC);
    model.addOrUpdate(details);

    const QModelIndex timestampIndex = model.index(0, DataAccessModel::ColTimestamp);

    model.setTimestampMode(AppSettings::TimestampMode::LocalTime);
    const QDateTime local = details.sourceTimestamp.toLocalTime();
    QCOMPARE(model.data(timestampIndex).toString(),
             local.toOffsetFromUtc(local.offsetFromUtc()).toString(Qt::ISODateWithMs)
                 .replace(QLatin1Char('T'), QLatin1Char(' ')));

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setTimestampMode(AppSettings::TimestampMode::Utc);
    QVERIFY(spy.count() >= 1);
    const QString utc = model.data(timestampIndex).toString();
    QCOMPARE(utc, details.sourceTimestamp.toUTC().toString(Qt::ISODateWithMs)
                      .replace(QLatin1Char('T'), QLatin1Char(' ')));
    QVERIFY(utc.endsWith(QLatin1Char('Z')));
    QVERIFY(!utc.contains(QLatin1Char('T')));
}

///
/// \brief An image-typed node names its picture instead of dumping hex, and hands out the bytes.
///
void TestModelsDataAccessPresentation::dataAccessSummarisesImagesAndOffersTheirBytes()
{
    const QByteArray png = encodedPng(QSize(6, 4));
    QVERIFY(!png.isEmpty());

    DataAccessModel model;

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=5;s=ImagePNG");
    details.dataTypeId = QStringLiteral("ns=0;i=2003");
    details.value = png;
    model.addOrUpdate(details);

    const QModelIndex value = model.index(0, DataAccessModel::ColValue);
    QVERIFY2(model.data(value).toString().startsWith(QStringLiteral("PNG 6×4, ")),
             qPrintable(model.data(value).toString()));
    QCOMPARE(model.data(value, ValueRoles::ImageDataRole).toByteArray(), png);

    // Only the value column carries the picture; a viewer button has no business elsewhere.
    QVERIFY(model.data(model.index(0, DataAccessModel::ColNodeId),
                       ValueRoles::ImageDataRole).isNull());

    // The same bytes under a plain ByteString DataType stay a hex dump with nothing to view.
    details.nodeId = QStringLiteral("ns=5;s=Bytes");
    details.dataTypeId = QStringLiteral("ns=0;i=15");
    model.addOrUpdate(details);
    const QModelIndex bytes = model.index(1, DataAccessModel::ColValue);
    QCOMPARE(model.data(bytes).toString(), QString::fromLatin1(png.toHex(' ')));
    QVERIFY(model.data(bytes, ValueRoles::ImageDataRole).isNull());
}

///
/// \brief A lost connection greys the listed rows and locks the subscription column (issue #7).
///
void TestModelsDataAccessPresentation::dataAccessOfflineGreysRowsAndLocksEditing()
{
    DataAccessModel model;
    DataAccessItem item;
    item.nodeId = QStringLiteral("ns=2;s=A");
    item.displayName = QStringLiteral("Alpha");
    item.value = QStringLiteral("42");
    item.status = QStringLiteral("Good");
    item.subscriptionName = QStringLiteral("Fast");
    model.setItems({item});

    const QModelIndex valueIndex = model.index(0, DataAccessModel::ColValue);
    const QModelIndex statusIndex = model.index(0, DataAccessModel::ColStatus);
    const QModelIndex subscriptionIndex = model.index(0, DataAccessModel::ColSubscription);
    QVERIFY(!model.isOffline());
    QVERIFY(model.flags(subscriptionIndex).testFlag(Qt::ItemIsEditable));

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setOffline(true);

    QVERIFY(model.isOffline());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(valueIndex, Qt::ForegroundRole).value<QBrush>().color(),
             qApp->palette().color(QPalette::Disabled, QPalette::Text));
    QCOMPARE(model.data(statusIndex, Qt::ForegroundRole).value<QBrush>().color(),
             qApp->palette().color(QPalette::Disabled, QPalette::Text));
    QVERIFY(!model.flags(subscriptionIndex).testFlag(Qt::ItemIsEditable));
    QVERIFY(!model.setData(subscriptionIndex, QStringLiteral("Slow"), Qt::EditRole));
    QCOMPARE(model.itemAt(0).subscriptionName, QStringLiteral("Fast"));

    model.setOffline(false);
    QVERIFY(!model.data(valueIndex, Qt::ForegroundRole).isValid());
    QVERIFY(!model.data(statusIndex, Qt::ForegroundRole).isValid());
    QVERIFY(model.setData(subscriptionIndex, QStringLiteral("Slow"), Qt::EditRole));
}


QTEST_MAIN(TestModelsDataAccessPresentation)

#include "test_models_data_access_presentation.moc"
