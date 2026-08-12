// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesswidget_presentation.cpp
/// \brief Tests DataAccessWidget delegates, palettes, and change highlighting.
///

#include <QHeaderView>
#include <QTest>

#include "test_dataaccesswidget_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestDataAccessWidgetPresentation : public QObject
{
    Q_OBJECT

private slots:
    void numberColumnExpandsToFitItsContents();
    void valueAndStatusColumnsShareTheStateDelegate();
    void selectedStatusUsesContrastText();
    void subscriptionEditorHasOpaqueBackground();
    void contextMenuOverridesChangeHighlight();
    void contextMenuOffersAddOnlyWithoutSelection();
    void contextMenuRequestsAddressSpaceReveal();
    void selectedRowsStillShowTheChangeWash();
    void fastSubscriptionsFadeWithinTheirInterval();
};

///
/// \brief The row-number column grows when its contents need more than the initial width.
///
void TestDataAccessWidgetPresentation::numberColumnExpandsToFitItsContents()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    QVector<OpcUaNodeInfo> nodes;
    nodes.reserve(99);
    for (int number = 1; number <= 99; ++number)
        nodes.append(makeVariable(number));
    widget.addPendingNodes(nodes);
    const int twoDigitWidth = view->header()->sectionSize(DataAccessModel::ColNumber);

    widget.addPendingNodes({makeVariable(100)});

    QTRY_VERIFY(view->header()->sectionSize(DataAccessModel::ColNumber) > twoDigitWidth);
    QCOMPARE(view->model()->index(99, DataAccessModel::ColNumber).data().toInt(), 100);
}

///
/// \brief Value and status cells are painted by one state delegate, and values align right.
///
void TestDataAccessWidgetPresentation::valueAndStatusColumnsShareTheStateDelegate()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    QAbstractItemDelegate *valueDelegate =
        view->itemDelegateForColumn(DataAccessModel::ColValue);
    QVERIFY(valueDelegate);
    QCOMPARE(view->itemDelegateForColumn(DataAccessModel::ColStatus), valueDelegate);
    QVERIFY(view->itemDelegateForColumn(DataAccessModel::ColSubscription) != valueDelegate);

    widget.addNode(makeNodeDetails());
    QCOMPARE(view->model()->index(0, DataAccessModel::ColValue)
                 .data(Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));
}

///
/// \brief Selected status text adapts to the system highlight colour.
///
void TestDataAccessWidgetPresentation::selectedStatusUsesContrastText()
{
    ItemPaletteStyle style;
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNodeWithDefaultSubscription(makeNodeDetails());

    view->setStyle(&style);
    QPalette palette = view->palette();
    const QColor highlight(0xef, 0x25, 0x20);
    const QColor systemText(Qt::white);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, systemText);
    view->setPalette(palette);

    const QModelIndex status = view->model()->index(0, DataAccessModel::ColStatus);
    QStyleOptionViewItem option;
    option.initFrom(view);
    option.widget = view;
    option.rect = QRect(0, 0, 100, 24);
    option.state |= QStyle::State_Selected;

    QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    view->itemDelegateForColumn(DataAccessModel::ColStatus)->paint(&painter, option, status);
    painter.end();

    QCOMPARE(style.text, systemText);
    QCOMPARE(style.highlightedText, systemText);
    QVERIFY(style.highlightedText != AppColors::statusSuccess());
    view->setStyle(nullptr);
}

///
/// \brief The subscription editor covers the cell text with an opaque palette background.
///
void TestDataAccessWidgetPresentation::subscriptionEditorHasOpaqueBackground()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>();
    QVERIFY(view);

    QAbstractItemDelegate *delegate =
        view->itemDelegateForColumn(DataAccessModel::ColSubscription);
    QVERIFY(delegate);

    QStyleOptionViewItem option;
    QScopedPointer<QWidget> editor(delegate->createEditor(view->viewport(), option, QModelIndex()));
    auto *combo = qobject_cast<QComboBox *>(editor.data());
    QVERIFY(combo);
    QVERIFY(combo->autoFillBackground());
    QCOMPARE(combo->backgroundRole(), QPalette::Base);
}

///
/// \brief The context menu switches change highlighting off for the selected rows only.
///
void TestDataAccessWidgetPresentation::contextMenuOverridesChangeHighlight()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeNodeDetails());
    widget.addNode(makeBooleanNodeDetails(false, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    const QModelIndex first = view->model()->index(0, DataAccessModel::ColValue);
    const QModelIndex second = view->model()->index(1, DataAccessModel::ColValue);
    QVERIFY(!first.data(DataAccessModel::HighlightChangesRole).toBool());

    view->selectionModel()->select(view->model()->index(0, 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QTimer::singleShot(0, &widget, [&widget]() {
        QMenu *menu = widget.findChild<QMenu *>();
        QVERIFY(menu);
        for (QAction *action : menu->actions()) {
            if (!action->isCheckable())
                continue;
            QVERIFY(!action->isChecked());
            action->trigger();
            break;
        }
        menu->close();
    });
    QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested",
                                      Q_ARG(QPoint, view->visualRect(first).center())));

    QVERIFY(first.data(DataAccessModel::HighlightChangesRole).toBool());
    QVERIFY(!second.data(DataAccessModel::HighlightChangesRole).toBool());

    // The override belongs to the node, so a saved session carries it per row.
    const QVector<SessionNode> nodes = widget.monitoredNodes();
    QCOMPARE(nodes.size(), 2);
    QCOMPARE(nodes.at(0).highlight, HighlightMode::Enabled);
    QCOMPARE(nodes.at(1).highlight, HighlightMode::FollowDefault);
}

///
/// \brief With no Data Access selection the context menu offers adding the address-space node.
///
void TestDataAccessWidgetPresentation::contextMenuOffersAddOnlyWithoutSelection()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeNodeDetails());
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    view->selectionModel()->clearSelection();

    QSignalSpy addSpy(&widget, &DataAccessWidget::addSelectedNodeRequested);
    QVERIFY(addSpy.isValid());
    QTimer::singleShot(0, &widget, [&widget]() {
        QAction *action = widget.findChild<QAction *>(QStringLiteral("actionAddSelectedNode"));
        QVERIFY(action);
        QMenu *menu = qobject_cast<QMenu *>(action->parent());
        QVERIFY(menu);
        QVERIFY(!widget.findChild<QAction *>(QStringLiteral("actionShowInAddressSpace")));
        QCOMPARE(menu->actions().first(), action);
        QVERIFY(menu->actions().size() > 1);
        QVERIFY(!menu->actions().at(1)->isSeparator());
        action->trigger();
        menu->close();
    });
    QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested",
                                      Q_ARG(QPoint, QPoint(2, view->viewport()->height() - 2))));

    QCOMPARE(addSpy.count(), 1);
}

///
/// \brief The address-space context action requests the selected node by its exact NodeId.
///
void TestDataAccessWidgetPresentation::contextMenuRequestsAddressSpaceReveal()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    const OpcUaNodeDetails details = makeNodeDetails();
    widget.addNode(details);
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    const QModelIndex row = view->model()->index(0, 0);
    view->selectionModel()->select(row,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QSignalSpy revealSpy(&widget, &DataAccessWidget::showInAddressSpaceRequested);
    QVERIFY(revealSpy.isValid());

    QTimer::singleShot(0, &widget, [&widget]() {
        QAction *action = widget.findChild<QAction *>(QStringLiteral("actionShowInAddressSpace"));
        QVERIFY(action);
        QMenu *menu = qobject_cast<QMenu *>(action->parent());
        QVERIFY(menu);
        for (QAction *menuAction : menu->actions())
            QVERIFY(menuAction->text() != QStringLiteral("Add Node"));
        QVERIFY(action->isEnabled());
        action->trigger();
        menu->close();
    });
    QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested",
                                      Q_ARG(QPoint, view->visualRect(row).center())));

    QCOMPARE(revealSpy.count(), 1);
    QCOMPARE(revealSpy.takeFirst().at(0).toString(), details.nodeId);
}

///
/// \brief Selecting a row does not suppress the change wash on its value cell.
///
void TestDataAccessWidgetPresentation::selectedRowsStillShowTheChangeWash()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.setHighlightValueChanges(true);
    widget.addNode(makeNodeDetails());
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    view->selectionModel()->select(view->model()->index(0, 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    OpcUaDataValue value;
    value.nodeId = makeNodeDetails().nodeId;
    value.value = 42.0;
    value.status = QStringLiteral("Good");
    widget.updateValues({value});

    const QModelIndex valueIndex = view->model()->index(0, DataAccessModel::ColValue);
    QVERIFY(valueIndex.data(DataAccessModel::HighlightChangesRole).toBool());
    const QImage washed = renderViewport(view);

    widget.setHighlightValueChanges(false);
    QVERIFY(!valueIndex.data(DataAccessModel::HighlightChangesRole).toBool());
    const QImage plain = renderViewport(view);
    // Without the wash the row no longer changes over time, so the difference
    // below is the wash itself and not the fade between two captures.
    QCOMPARE(plain, renderViewport(view));
    QVERIFY(washed != plain);
}

///
/// \brief A short publishing interval shortens the wash instead of leaving the row tinted.
///
void TestDataAccessWidgetPresentation::fastSubscriptionsFadeWithinTheirInterval()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.setHighlightValueChanges(true);

    const OpcUaNodeDetails details = makeNodeDetails();
    SubscriptionItem subscription;
    subscription.name = QStringLiteral("Fast");
    subscription.publishingInterval = 250.0;
    widget.addNodeWithDefaultSubscription(details, subscription);
    widget.setNodeRevisedInterval(details.nodeId, 250.0);
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    const QModelIndex valueIndex = view->model()->index(0, DataAccessModel::ColValue);
    QCOMPARE(valueIndex.data(DataAccessModel::ExpectedIntervalRole).toDouble(), 250.0);

    OpcUaDataValue value;
    value.nodeId = details.nodeId;
    value.value = 42.0;
    value.status = QStringLiteral("Good");
    widget.updateValues({value});
    const QImage washed = renderViewport(view);

    // Past the interval the cell must be back to exactly its unwashed look, where the
    // default 800 ms wash would still be tinting it half-strength.
    QTest::qWait(400);
    const QImage faded = renderViewport(view);

    widget.setHighlightValueChanges(false);
    const QImage unwashed = renderViewport(view);
    QVERIFY(washed != unwashed);
    QCOMPARE(faded, unwashed);
}

QTEST_MAIN(TestDataAccessWidgetPresentation)

#include "test_dataaccesswidget_presentation.moc"
