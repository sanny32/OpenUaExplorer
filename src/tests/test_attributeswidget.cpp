// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeswidget.cpp
/// \brief Tests AttributesWidget clipboard actions.
///

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QTimer>
#include <QTreeView>
#include <QTest>

#include "application.h"
#include "widgets/headerview.h"
#include "widgets/attributeswidget.h"

///
/// \brief UI tests for AttributesWidget clipboard actions.
///
class TestAttributesWidget : public QObject
{
    Q_OBJECT

private slots:
    void usesSharedHeaderView();
    void headerSectionsAreResizable();
    void copiesCurrentCell();
    void copiesFullTree();
    void contextMenuOnlyUsesValueColumn();
};

namespace {

///
/// \brief Builds one displayable attribute for widget tests.
/// \param name Attribute name.
/// \param value Display value.
/// \return Attribute item.
///
OpcUaNodeAttribute attributeItem(const QString &name, const QString &value)
{
    OpcUaNodeAttribute attribute;
    attribute.name = name;
    attribute.displayValue = value;
    return attribute;
}

///
/// \brief Builds selected-node details with a small nested attribute tree.
/// \return Node details for widget tests.
///
OpcUaNodeDetails makeDetails()
{
    OpcUaNodeAttribute value = attributeItem(QStringLiteral("Value"), QStringLiteral("42"));
    value.children.append(attributeItem(QStringLiteral("StatusCode"), QStringLiteral("Good")));
    value.children.append(attributeItem(QStringLiteral("SourceTimestamp"),
                                        QStringLiteral("2026-06-23T10:00:00.000Z")));

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Device.Level");
    details.nodeClass = OpcUa::Variable;
    details.attributes = {
        attributeItem(QStringLiteral("NodeId"), details.nodeId),
        value,
        attributeItem(QStringLiteral("DisplayName"), QStringLiteral("Level"))
    };
    return details;
}

} // namespace

///
/// \brief Verifies the attributes tree uses the shared header implementation.
///
void TestAttributesWidget::usesSharedHeaderView()
{
    AttributesWidget widget;
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(tree);
    QVERIFY(qobject_cast<HeaderView *>(tree->header()));
}

///
/// \brief Verifies the attributes columns can be resized by the user.
///
void TestAttributesWidget::headerSectionsAreResizable()
{
    AttributesWidget widget;
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(tree);

    QCOMPARE(tree->header()->sectionResizeMode(0), QHeaderView::Interactive);
    QCOMPARE(tree->header()->sectionResizeMode(1), QHeaderView::Interactive);
}

///
/// \brief Verifies the Copy Cell action writes the current cell text.
///
void TestAttributesWidget::copiesCurrentCell()
{
    AttributesWidget widget;
    widget.setNodeDetails(makeDetails());

    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(tree);
    const QModelIndex valueIndex = tree->model()->index(1, 1);
    QVERIFY(valueIndex.isValid());
    tree->setCurrentIndex(valueIndex);

    QAction *copyCellAction =
        widget.findChild<QAction *>(QStringLiteral("actionCopyAttributeCell"));
    QVERIFY(copyCellAction);
    copyCellAction->trigger();

    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("42"));
}

///
/// \brief Verifies the Copy Tree action writes every attribute row in tree order.
///
void TestAttributesWidget::copiesFullTree()
{
    AttributesWidget widget;
    widget.setNodeDetails(makeDetails());

    QAction *copyTreeAction =
        widget.findChild<QAction *>(QStringLiteral("actionCopyAttributeTree"));
    QVERIFY(copyTreeAction);
    copyTreeAction->trigger();

    const QString expected =
        QStringLiteral("NodeId\tns=2;s=Device.Level\n"
                       "Value\t42\n"
                       "  StatusCode\tGood\n"
                       "  SourceTimestamp\t2026-06-23T10:00:00.000Z\n"
                       "DisplayName\tLevel");
    QCOMPARE(QApplication::clipboard()->text(), expected);
}

///
/// \brief Verifies the tree context menu is accepted only from Value cells.
///
void TestAttributesWidget::contextMenuOnlyUsesValueColumn()
{
    AttributesWidget widget;
    widget.setNodeDetails(makeDetails());
    widget.resize(420, 260);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(tree);

    const QModelIndex current = tree->model()->index(0, 1);
    const QModelIndex attribute = tree->model()->index(2, 0);
    const QModelIndex value = tree->model()->index(2, 1);
    QVERIFY(current.isValid());
    QVERIFY(attribute.isValid());
    QVERIFY(value.isValid());
    tree->setCurrentIndex(current);

    const QPoint attributePos = tree->visualRect(attribute).center();
    QVERIFY(QMetaObject::invokeMethod(tree, "customContextMenuRequested",
                                      Q_ARG(QPoint, attributePos)));
    QCOMPARE(tree->currentIndex(), current);

    QTimer::singleShot(0, &widget, [&widget]() {
        if (QMenu *menu = widget.findChild<QMenu *>())
            menu->close();
    });
    const QPoint valuePos = tree->visualRect(value).center();
    QVERIFY(QMetaObject::invokeMethod(tree, "customContextMenuRequested",
                                      Q_ARG(QPoint, valuePos)));
    QCOMPARE(tree->currentIndex(), value);
}

///
/// \brief Runs the suite under Application so theme-aware actions can load icons.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestAttributesWidget test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_attributeswidget.moc"
