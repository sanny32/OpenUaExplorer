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

#include "application.h"
#include "appsettings.h"
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
    void logDepthFollowsTheStoredPreference();
    void changingTheDepthTrimsTheOpenLog();
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

///
/// \brief A widget opened after the preference was set starts out at that depth.
///
void TestLogWidget::logDepthFollowsTheStoredPreference()
{
    AppSettings().setMaxLogRows(120);

    LogWidget widget;
    auto *table = widget.findChild<QTableView *>(QStringLiteral("logTable"));
    QVERIFY(table);

    for (int index = 0; index < 130; ++index)
        qCWarning(lcLogWidgetClient).noquote() << "entry" << index;

    QTRY_COMPARE(table->model()->rowCount(), 120);

    AppSettings().setMaxLogRows(AppSettings::defaultMaxLogRows);
}

///
/// \brief Changing the preference retrims a log that is already open.
///
void TestLogWidget::changingTheDepthTrimsTheOpenLog()
{
    AppSettings().setMaxLogRows(AppSettings::defaultMaxLogRows);

    LogWidget widget;
    auto *table = widget.findChild<QTableView *>(QStringLiteral("logTable"));
    QVERIFY(table);

    for (int index = 0; index < 200; ++index)
        qCWarning(lcLogWidgetClient).noquote() << "entry" << index;
    QTRY_COMPARE(table->model()->rowCount(), 200);

    theApp()->setMaxLogRows(150);
    QCOMPARE(table->model()->rowCount(), 150);

    AppSettings().setMaxLogRows(AppSettings::defaultMaxLogRows);
}

// The widget follows the log depth preference through Application, whose signal a plain
// QApplication does not carry.
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestLogWidget test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_logwidget.moc"
