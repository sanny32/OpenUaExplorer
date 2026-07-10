// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_logwidget.cpp
/// \brief Tests LogWidget category forwarding and source filtering.
///

#include <QAbstractItemModel>
#include <QComboBox>
#include <QLoggingCategory>
#include <QTableView>
#include <QTest>

#include "models/logmodel.h"
#include "widgets/themedpushbutton.h"
#include "widgets/logwidget.h"

Q_LOGGING_CATEGORY(lcLogWidgetClient, "ouaexp.Client")
Q_LOGGING_CATEGORY(lcLogWidgetOpen62541Plugin, "qt.opcua.plugins.open62541")
Q_LOGGING_CATEGORY(lcLogWidgetOpen62541Client,
                   "qt.opcua.plugins.open62541.sdk.client")

///
/// \brief UI tests for LogWidget.
///
class TestLogWidget : public QObject
{
    Q_OBJECT

private slots:
    void open62541SdkSourcesKeepBackendPrefix();
    void clearButtonUsesTrashIcon();
};

///
/// \brief open62541 SDK categories keep their backend name in the Source column.
///
void TestLogWidget::open62541SdkSourcesKeepBackendPrefix()
{
    LogWidget widget;
    auto *table = widget.findChild<QTableView *>(QStringLiteral("logTable"));
    auto *sourceCombo = widget.findChild<QComboBox *>(QStringLiteral("sourceCombo"));
    QVERIFY(table);
    QVERIFY(sourceCombo);

    qCWarning(lcLogWidgetClient).noquote() << "Application warning";
    qCWarning(lcLogWidgetOpen62541Plugin).noquote() << "Plugin warning";
    qCWarning(lcLogWidgetOpen62541Client).noquote() << "Backend warning";

    QTRY_COMPARE(table->model()->rowCount(), 3);
    QCOMPARE(table->model()->data(table->model()->index(0, LogModel::ColSource)).toString(),
             QStringLiteral("Client"));
    QCOMPARE(table->model()->data(table->model()->index(1, LogModel::ColSource)).toString(),
             QStringLiteral("open62541"));
    QCOMPARE(table->model()->data(table->model()->index(2, LogModel::ColSource)).toString(),
             QStringLiteral("open62541/client"));

    QVERIFY(sourceCombo->findText(QStringLiteral("Client")) >= 0);
    QVERIFY(sourceCombo->findText(QStringLiteral("open62541")) >= 0);
    QVERIFY(sourceCombo->findText(QStringLiteral("open62541/client")) >= 0);
}

///
/// \brief The log Clear button uses the trash icon.
///
void TestLogWidget::clearButtonUsesTrashIcon()
{
    LogWidget widget;
    auto *button = widget.findChild<ThemedPushButton *>(QStringLiteral("clearButton"));
    QVERIFY(button);
    QCOMPARE(button->iconName(), QStringLiteral("trash"));
}

QTEST_MAIN(TestLogWidget)

#include "test_logwidget.moc"
