// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_addressspacewidget.cpp
/// \brief Tests AddressSpaceWidget selection-dependent detail views.
///

#include <QAbstractItemModel>
#include <QIcon>
#include <QImage>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTableView>
#include <QTest>
#include <QTreeView>

#include "widgets/addressspacewidget.h"
#include "widgets/spinneraction.h"

///
/// \brief UI tests for AddressSpaceWidget.
///
class TestAddressSpaceWidget : public QObject
{
    Q_OBJECT

private slots:
    void selectionRefreshesReferences();
    void searchRequestsSubtreeCrawl();
    void searchSpinnerReplacesClearButton();
    void searchSpinnerRotatesWhileSearching();
    void searchRevealsMatchAsBrowseResultsArrive();
    void searchReportsMissingNode();
    void repeatedReturnContinuesTheSameSearch();
    void editingThePatternCancelsTheSearch();
    void exhaustedSearchReportsNoMoreMatches();
};

namespace {

///
/// \brief Builds an OPC UA node info item for widget tests.
/// \param nodeId NodeId string.
/// \param displayName DisplayName text.
/// \param referenceTypeId Reference type NodeId.
/// \param hasChildren Whether the node may have children.
/// \return Node info item.
///
OpcUaNodeInfo makeNode(const QString &nodeId, const QString &displayName,
                       const QString &referenceTypeId = QString(), bool hasChildren = false)
{
    OpcUaNodeInfo node;
    node.nodeId = nodeId;
    node.browseName = displayName;
    node.displayName = displayName;
    node.referenceTypeId = referenceTypeId;
    node.nodeClass = hasChildren ? 1 : 2;
    node.hasChildren = hasChildren;
    return node;
}

///
/// \brief Reports whether an icon renders any non-transparent pixel.
/// \param icon Icon to inspect.
/// \param size Pixmap size to render.
/// \return True when at least one pixel is drawn.
///
bool hasVisiblePixels(const QIcon &icon, int size)
{
    const QImage frame = icon.pixmap(size, size).toImage();
    for (int y = 0; y < frame.height(); ++y) {
        for (int x = 0; x < frame.width(); ++x) {
            if (qAlpha(frame.pixel(x, y)) > 0)
                return true;
        }
    }
    return false;
}

} // namespace

///
/// \brief Selecting another node replaces the references table instead of leaving stale rows.
///
void TestAddressSpaceWidget::selectionRefreshesReferences()
{
    AddressSpaceWidget widget;
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("addressTree"));
    auto *references = widget.findChild<QTableView *>(QStringLiteral("referencesTable"));
    QVERIFY(tree);
    QVERIFY(references);
    QSignalSpy referencesSpy(&widget, &AddressSpaceWidget::referencesRequested);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    const OpcUaNodeInfo device = makeNode(QStringLiteral("ns=2;s=Device"),
                                          QStringLiteral("Device"),
                                          QStringLiteral("ns=0;i=35"), true);
    const OpcUaNodeInfo levelViaOrganizes = makeNode(QStringLiteral("ns=2;s=Level"),
                                                     QStringLiteral("Level"),
                                                     QStringLiteral("ns=0;i=35"));
    const OpcUaNodeInfo levelViaHasComponent = makeNode(QStringLiteral("ns=2;s=Level"),
                                                        QStringLiteral("Level"),
                                                        QStringLiteral("ns=0;i=47"));

    widget.setRootNode(root);
    widget.setBrowseChildren(root.nodeId, {device}, QString());
    widget.setBrowseReferences(root.nodeId, {device, levelViaOrganizes,
                                             levelViaHasComponent}, QString());

    QCOMPARE(references->model()->rowCount(), 3);
    QCOMPARE(references->model()->data(references->model()->index(2, 0)).toString(),
             QStringLiteral("HasComponent"));

    const QModelIndex rootIndex = tree->model()->index(0, 0);
    const QModelIndex deviceIndex = tree->model()->index(0, 0, rootIndex);
    QVERIFY(deviceIndex.isValid());

    const int requestsBeforeDeviceSelection = referencesSpy.size();
    tree->setCurrentIndex(deviceIndex);
    QCOMPARE(referencesSpy.size(), requestsBeforeDeviceSelection + 1);
    QCOMPARE(referencesSpy.last().first().toString(), device.nodeId);
    QCOMPARE(references->model()->rowCount(), 0);

    const OpcUaNodeInfo alarm = makeNode(QStringLiteral("ns=2;s=Alarm"),
                                         QStringLiteral("Alarm"),
                                         QStringLiteral("ns=0;i=47"));
    widget.setBrowseChildren(device.nodeId, {alarm}, QString());
    widget.setBrowseReferences(device.nodeId, {alarm}, QString());
    QCOMPARE(references->model()->rowCount(), 1);
    QCOMPARE(references->model()->data(references->model()->index(0, 1)).toString(),
             alarm.displayName);

    const QModelIndex alarmIndex = tree->model()->index(0, 0, deviceIndex);
    QVERIFY(alarmIndex.isValid());
    tree->setCurrentIndex(alarmIndex);
    QCOMPARE(references->model()->rowCount(), 0);

    tree->setCurrentIndex(rootIndex);
    QCOMPARE(references->model()->rowCount(), 3);
}

///
/// \brief Pressing Return in the search box crawls the root's subtree and spins the indicator.
///
void TestAddressSpaceWidget::searchRequestsSubtreeCrawl()
{
    AddressSpaceWidget widget;
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    auto *spinner = widget.findChild<SpinnerAction *>();
    QVERIFY(searchEdit);
    QVERIFY(spinner);
    QVERIFY(!spinner->isSpinning());
    QSignalSpy searchSpy(&widget, &AddressSpaceWidget::searchRequested);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    widget.setRootNode(root);

    searchEdit->setText(QStringLiteral("  level  "));
    QTest::keyClick(searchEdit, Qt::Key_Return);

    QCOMPARE(searchSpy.size(), 1);
    QCOMPARE(searchSpy.last().at(0).toString(), root.nodeId);
    QCOMPARE(searchSpy.last().at(1).toString(), QStringLiteral("level"));
    QVERIFY(spinner->isSpinning());
    QVERIFY(spinner->isVisible());
    QVERIFY(hasVisiblePixels(spinner->icon(), spinner->spinnerSize()));
    QVERIFY(!searchEdit->isClearButtonEnabled());
}

///
/// \brief The busy spinner and the clear button never share the trailing icon slot.
///
void TestAddressSpaceWidget::searchSpinnerReplacesClearButton()
{
    AddressSpaceWidget widget;
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    auto *spinner = widget.findChild<SpinnerAction *>();
    QVERIFY(searchEdit);
    QVERIFY(spinner);

    QVERIFY(searchEdit->isClearButtonEnabled());
    QVERIFY(!spinner->isVisible());

    widget.setRootNode(makeNode(QStringLiteral("ns=0;i=84"),
                                QStringLiteral("Root"), QString(), true));
    searchEdit->setText(QStringLiteral("level"));
    QTest::keyClick(searchEdit, Qt::Key_Return);

    QVERIFY(spinner->isVisible());
    QVERIFY(!searchEdit->isClearButtonEnabled());

    widget.setSearchResult({}, QString(), QString());

    QVERIFY(!spinner->isVisible());
    QVERIFY(searchEdit->isClearButtonEnabled());
}

///
/// \brief The spinner redraws itself at a new angle while the search runs.
///
void TestAddressSpaceWidget::searchSpinnerRotatesWhileSearching()
{
    AddressSpaceWidget widget;
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    auto *spinner = widget.findChild<SpinnerAction *>();
    QVERIFY(searchEdit);
    QVERIFY(spinner);

    widget.setRootNode(makeNode(QStringLiteral("ns=0;i=84"),
                                QStringLiteral("Root"), QString(), true));
    searchEdit->setText(QStringLiteral("level"));
    QTest::keyClick(searchEdit, Qt::Key_Return);
    QVERIFY(spinner->isSpinning());

    const int size = spinner->spinnerSize();
    const QImage firstFrame = spinner->icon().pixmap(size, size).toImage();
    QVERIFY(!firstFrame.isNull());
    QTRY_VERIFY(spinner->icon().pixmap(size, size).toImage() != firstFrame);

    widget.setSearchResult({}, QString(), QString());
    QVERIFY(!spinner->isSpinning());
    QVERIFY(spinner->icon().isNull());
    QVERIFY(!spinner->isVisible());
}

///
/// \brief A search match is expanded to and selected once its ancestors' children load.
///
void TestAddressSpaceWidget::searchRevealsMatchAsBrowseResultsArrive()
{
    AddressSpaceWidget widget;
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("addressTree"));
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    QVERIFY(tree);
    QVERIFY(searchEdit);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    const OpcUaNodeInfo device = makeNode(QStringLiteral("ns=2;s=Device"),
                                          QStringLiteral("Device"),
                                          QStringLiteral("ns=0;i=35"), true);
    const OpcUaNodeInfo level = makeNode(QStringLiteral("ns=2;s=Level"),
                                         QStringLiteral("LevelIndicator"),
                                         QStringLiteral("ns=0;i=47"));

    widget.setRootNode(root);
    searchEdit->setText(QStringLiteral("level"));
    QTest::keyClick(searchEdit, Qt::Key_Return);

    auto *spinner = widget.findChild<SpinnerAction *>();
    QVERIFY(spinner);
    QVERIFY(spinner->isSpinning());

    widget.setSearchResult({root.nodeId, device.nodeId}, level.nodeId, QString());
    QVERIFY(!spinner->isSpinning());

    widget.setBrowseChildren(root.nodeId, {device}, QString());
    QVERIFY(tree->isExpanded(tree->model()->index(0, 0)));
    QVERIFY(widget.selectedNode().nodeId != level.nodeId);

    widget.setBrowseChildren(device.nodeId, {level}, QString());

    QCOMPARE(widget.selectedNode().nodeId, level.nodeId);
}

///
/// \brief A search that matches nothing reports it instead of touching the tree selection.
///
void TestAddressSpaceWidget::searchReportsMissingNode()
{
    AddressSpaceWidget widget;
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    auto *spinner = widget.findChild<SpinnerAction *>();
    QVERIFY(searchEdit);
    QVERIFY(spinner);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    widget.setRootNode(root);

    searchEdit->setText(QStringLiteral("missing"));
    QTest::keyClick(searchEdit, Qt::Key_Return);
    widget.setSearchResult({}, QString(), QString());

    QVERIFY(!spinner->isSpinning());
    QVERIFY(!spinner->isVisible());
    QVERIFY(searchEdit->toolTip().contains(QStringLiteral("missing")));
    QCOMPARE(widget.selectedNode().nodeId, root.nodeId);
}

///
/// \brief Pressing Return after a match re-issues the same request so the crawl continues.
///
void TestAddressSpaceWidget::repeatedReturnContinuesTheSameSearch()
{
    AddressSpaceWidget widget;
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    QVERIFY(searchEdit);
    QSignalSpy searchSpy(&widget, &AddressSpaceWidget::searchRequested);
    QSignalSpy cancelSpy(&widget, &AddressSpaceWidget::searchCancelRequested);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    widget.setRootNode(root);

    searchEdit->setText(QStringLiteral("level"));
    QTest::keyClick(searchEdit, Qt::Key_Return);
    widget.setSearchResult({root.nodeId}, QStringLiteral("ns=2;s=First"), QString());

    QTest::keyClick(searchEdit, Qt::Key_Return);

    QCOMPARE(searchSpy.size(), 2);
    QCOMPARE(searchSpy.last().at(0).toString(), root.nodeId);
    QCOMPARE(searchSpy.last().at(1).toString(), QStringLiteral("level"));
    QCOMPARE(cancelSpy.size(), 0);
}

///
/// \brief Changing the pattern drops the paused crawl instead of continuing it.
///
void TestAddressSpaceWidget::editingThePatternCancelsTheSearch()
{
    AddressSpaceWidget widget;
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    QVERIFY(searchEdit);
    QSignalSpy cancelSpy(&widget, &AddressSpaceWidget::searchCancelRequested);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    widget.setRootNode(root);

    searchEdit->setText(QStringLiteral("level"));
    QTest::keyClick(searchEdit, Qt::Key_Return);
    widget.setSearchResult({root.nodeId}, QStringLiteral("ns=2;s=First"), QString());
    QCOMPARE(cancelSpy.size(), 0);

    searchEdit->setText(QStringLiteral("levels"));

    QCOMPARE(cancelSpy.size(), 1);
}

///
/// \brief Running out of matches is reported differently from never finding one.
///
void TestAddressSpaceWidget::exhaustedSearchReportsNoMoreMatches()
{
    AddressSpaceWidget widget;
    auto *searchEdit = widget.findChild<QLineEdit *>(QStringLiteral("searchEdit"));
    QVERIFY(searchEdit);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    widget.setRootNode(root);

    searchEdit->setText(QStringLiteral("level"));
    QTest::keyClick(searchEdit, Qt::Key_Return);
    widget.setSearchResult({root.nodeId}, QStringLiteral("ns=2;s=First"), QString());

    QTest::keyClick(searchEdit, Qt::Key_Return);
    widget.setSearchResult({}, QString(), QString());

    QVERIFY(searchEdit->toolTip().contains(QStringLiteral("No more nodes")));
}

QTEST_MAIN(TestAddressSpaceWidget)

#include "test_addressspacewidget.moc"
