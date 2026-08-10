// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QSettings>
#include <QSplitter>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include "application.h"
#include "appsettings.h"
#include "mainwindow.h"
#include "settingsstore.h"
#include "widgets/dataview.h"
#include "widgets/trendpanelwidget.h"

namespace {

///
/// \brief Builds a splitter state that collapses the trend section, as saved by
///        builds that still allowed collapsing.
/// \return Serialized state for the vertical central splitter.
///
QByteArray collapsedSplitterState()
{
    QSplitter splitter(Qt::Vertical);
    splitter.addWidget(new QWidget(&splitter));
    splitter.addWidget(new QWidget(&splitter));
    splitter.setChildrenCollapsible(true);
    splitter.setSizes({500, 0});
    return splitter.saveState();
}

}

class TestMainWindowLayout : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void centralSplitterSectionsAreNotCollapsible();
    void restoredStateKeepsSectionsUncollapsible();
    void restoredZeroHeightTrendPanelStaysVisible();
    void viewMenuEntryHidesAndShowsTrendPanel();
    void hiddenTrendPanelIsRestoredHidden();
    void hidingTheTrendPanelIsPersistedOnClose();
    void viewMenuEntriesTrackTheDataViewTabs();
    void closedDataViewTabIsRestoredClosed();
    void closingEveryTabCollapsesTheDataViewSection();

private:
    QSplitter *centralSplitter(MainWindow &window) const;
    TrendPanelWidget *trendPanel(MainWindow &window) const;
    QAction *trendPanelAction(MainWindow &window) const;
    DataView *dataView(MainWindow &window) const;
    QAction *eventsPageAction(MainWindow &window) const;

    QTemporaryDir _settingsDirectory;
};

void TestMainWindowLayout::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerLayoutTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerLayoutTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestMainWindowLayout::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

QSplitter *TestMainWindowLayout::centralSplitter(MainWindow &window) const
{
    return window.findChild<QSplitter *>(QStringLiteral("centralSplitter"));
}

TrendPanelWidget *TestMainWindowLayout::trendPanel(MainWindow &window) const
{
    return window.findChild<TrendPanelWidget *>(QStringLiteral("trendPanelWidget"));
}

QAction *TestMainWindowLayout::trendPanelAction(MainWindow &window) const
{
    return window.findChild<QAction *>(QStringLiteral("actionViewTrendPanel"));
}

DataView *TestMainWindowLayout::dataView(MainWindow &window) const
{
    return window.findChild<DataView *>(QStringLiteral("dataView"));
}

QAction *TestMainWindowLayout::eventsPageAction(MainWindow &window) const
{
    return window.findChild<QAction *>(QStringLiteral("actionViewEvents"));
}

void TestMainWindowLayout::centralSplitterSectionsAreNotCollapsible()
{
    MainWindow window;

    QSplitter *splitter = centralSplitter(window);
    QVERIFY(splitter);
    QVERIFY(!splitter->childrenCollapsible());
}

void TestMainWindowLayout::restoredStateKeepsSectionsUncollapsible()
{
    AppSettings settings;
    settings.setRestoreLayoutOnStartup(true);
    settings.setCentralSplitterState(collapsedSplitterState());

    MainWindow window;

    QSplitter *splitter = centralSplitter(window);
    QVERIFY(splitter);
    QVERIFY(!splitter->childrenCollapsible());
}

void TestMainWindowLayout::restoredZeroHeightTrendPanelStaysVisible()
{
    AppSettings settings;
    settings.setRestoreLayoutOnStartup(true);
    settings.setCentralSplitterState(collapsedSplitterState());

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    TrendPanelWidget *panel = trendPanel(window);
    QVERIFY(panel);
    QVERIFY(panel->height() > 0);

    const QList<int> sizes = centralSplitter(window)->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY(sizes.at(1) > 0);
}

void TestMainWindowLayout::viewMenuEntryHidesAndShowsTrendPanel()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QAction *action = trendPanelAction(window);
    TrendPanelWidget *panel = trendPanel(window);
    QVERIFY(action);
    QVERIFY(panel);
    QVERIFY(action->isChecked());
    QVERIFY(panel->isVisible());

    action->trigger();
    QVERIFY(panel->isHidden());

    action->trigger();
    QVERIFY(panel->isVisible());
}

void TestMainWindowLayout::hiddenTrendPanelIsRestoredHidden()
{
    AppSettings settings;
    settings.setRestoreLayoutOnStartup(true);
    settings.setTrendPanelVisible(false);

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(trendPanel(window)->isHidden());
    QVERIFY(!trendPanelAction(window)->isChecked());

    // The hidden panel reports a zero section size, which must not be mistaken for
    // a section dragged away.
    const QList<int> sizes = centralSplitter(window)->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY(sizes.at(0) > 0);
    QCOMPARE(sizes.at(1), 0);
}

void TestMainWindowLayout::hidingTheTrendPanelIsPersistedOnClose()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    trendPanelAction(window)->trigger();
    QVERIFY(window.close());

    QVERIFY(!AppSettings().trendPanelVisible());
}

void TestMainWindowLayout::viewMenuEntriesTrackTheDataViewTabs()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    DataView *view = dataView(window);
    QAction *action = eventsPageAction(window);
    QVERIFY(view);
    QVERIFY(action);
    QVERIFY(action->isCheckable());
    QVERIFY(action->isChecked());

    action->trigger();
    QVERIFY(!view->isPageVisible(DataView::EventsPage));

    action->trigger();
    QVERIFY(view->isPageVisible(DataView::EventsPage));
    QCOMPARE(view->currentPage(), static_cast<int>(DataView::EventsPage));

    // Closing the tab itself unchecks the entry instead of reopening the page.
    view->setPageVisible(DataView::EventsPage, false);
    QVERIFY(!action->isChecked());
    QVERIFY(!view->isPageVisible(DataView::EventsPage));
}

void TestMainWindowLayout::closedDataViewTabIsRestoredClosed()
{
    {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        eventsPageAction(window)->trigger();
        QVERIFY(window.close());
    }

    AppSettings settings;
    settings.setRestoreLayoutOnStartup(true);
    QCOMPARE(settings.closedDataAccessPages(), QList<int>{int(DataView::EventsPage)});

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(!dataView(window)->isPageVisible(DataView::EventsPage));
    QVERIFY(!eventsPageAction(window)->isChecked());
}

void TestMainWindowLayout::closingEveryTabCollapsesTheDataViewSection()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    DataView *view = dataView(window);
    TrendPanelWidget *panel = trendPanel(window);
    QVERIFY(view);
    QVERIFY(panel);
    QVERIFY(view->isVisible());

    view->setClosedPages({int(DataView::DataAccessPage), int(DataView::EventsPage),
                          int(DataView::DataHistoryPage), int(DataView::EventsHistoryPage)});
    QTest::qWait(50);

    QVERIFY(!view->hasVisiblePages());
    QVERIFY(view->isHidden());
    QVERIFY(panel->height() > 0);

    eventsPageAction(window)->trigger();
    QTest::qWait(50);

    QVERIFY(view->isVisible());
    QVERIFY(view->height() > 0);
    QVERIFY(panel->height() > 0);
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestMainWindowLayout test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mainwindow_layout.moc"
