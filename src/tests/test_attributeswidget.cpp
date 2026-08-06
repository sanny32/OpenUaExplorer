// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeswidget.cpp
/// \brief Tests AttributesWidget clipboard actions and its value write editor.
///

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QGroupBox>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeView>
#include <QTest>

#include "application.h"
#include "settingsstore.h"
#include "widgets/headerview.h"
#include "widgets/attributeswidget.h"

///
/// \brief UI tests for AttributesWidget clipboard actions and its value write editor.
///
class TestAttributesWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void usesSharedHeaderView();
    void headerSectionsAreResizable();
    void valueSectionStretchesToFillView();
    void valueSectionStretchesAfterStateRestore();
    void copiesCurrentCell();
    void copiesFullTree();
    void contextMenuOnlyUsesValueColumn();
    void booleanNodeOffersValueList();
    void booleanListStartsAtTheCurrentValue();
    void booleanWriteSendsTypedBoolean();
    void nonBooleanNodeKeepsTextField();
    void valueEditorStretchesWithPanel();

private:
    QTemporaryDir _settingsDirectory;
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

///
/// \brief Builds details of a writable variable for write-editor tests.
/// \param valueType QOpcUa::Types numeric value.
/// \param dataTypeId DataType NodeId.
/// \param value Current value of the node.
/// \return Node details for widget tests.
///
OpcUaNodeDetails makeWritableDetails(int valueType, const QString &dataTypeId,
                                     const QVariant &value)
{
    OpcUaNodeDetails details = makeDetails();
    details.valueType = valueType;
    details.dataTypeId = dataTypeId;
    details.value = value;
    details.userAccessLevel = OpcUa::CurrentRead | OpcUa::CurrentWrite;
    return details;
}

///
/// \brief Builds details of a writable Boolean variable.
/// \param value Current value of the node.
/// \return Node details for widget tests.
///
OpcUaNodeDetails makeBooleanDetails(bool value)
{
    return makeWritableDetails(0, QStringLiteral("ns=0;i=1"), value);
}

} // namespace

///
/// \brief Routes QSettings to a temporary directory.
///
void TestAttributesWidget::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("AttributesWidget"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Clears stored settings between tests.
///
void TestAttributesWidget::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

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
/// \brief Verifies the attributes column can be resized by the user.
///
void TestAttributesWidget::headerSectionsAreResizable()
{
    AttributesWidget widget;
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(tree);

    QCOMPARE(tree->header()->sectionResizeMode(0), QHeaderView::Interactive);
}

///
/// \brief Verifies the value column fills the remaining header width.
///
void TestAttributesWidget::valueSectionStretchesToFillView()
{
    AttributesWidget widget;
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(tree);

    QVERIFY(tree->header()->stretchLastSection());
}

///
/// \brief Restored header state keeps the value column filling the view.
///
void TestAttributesWidget::valueSectionStretchesAfterStateRestore()
{
    AppSettings settings;
    settings.clearLayout();

    AttributesWidget savedWidget;
    auto *savedTree = savedWidget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(savedTree);
    auto *savedHeader = qobject_cast<HeaderView *>(savedTree->header());
    QVERIFY(savedHeader);
    savedHeader->setStretchLastSection(false);
    settings.setViewState(savedTree->objectName(), savedHeader->saveLayout());

    AttributesWidget restoredWidget;
    restoredWidget.restoreViewState(settings);
    auto *restoredTree =
        restoredWidget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(restoredTree);
    QVERIFY(restoredTree->header()->stretchLastSection());

    settings.clearLayout();
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
/// \brief A Boolean variable replaces the text field with the True/False list.
///
void TestAttributesWidget::booleanNodeOffersValueList()
{
    AttributesWidget widget;
    widget.setNodeDetails(makeBooleanDetails(false));

    auto *valueCombo = widget.findChild<QComboBox *>(QStringLiteral("valueCombo"));
    auto *valueEdit = widget.findChild<QLineEdit *>(QStringLiteral("valueEdit"));
    QVERIFY(valueCombo);
    QVERIFY(valueEdit);
    QVERIFY(valueCombo->isVisibleTo(&widget));
    QVERIFY(!valueEdit->isVisibleTo(&widget));
    QCOMPARE(valueCombo->count(), 2);
}

///
/// \brief The True/False list opens on the value the node currently holds.
///
void TestAttributesWidget::booleanListStartsAtTheCurrentValue()
{
    AttributesWidget widget;
    auto *valueCombo = widget.findChild<QComboBox *>(QStringLiteral("valueCombo"));
    QVERIFY(valueCombo);

    widget.setNodeDetails(makeBooleanDetails(true));
    QCOMPARE(valueCombo->currentIndex(), 1);

    widget.setNodeDetails(makeBooleanDetails(false));
    QCOMPARE(valueCombo->currentIndex(), 0);
}

///
/// \brief Writing a Boolean sends the picked value as a bool, without text conversion.
///
void TestAttributesWidget::booleanWriteSendsTypedBoolean()
{
    AttributesWidget widget;
    widget.setNodeDetails(makeBooleanDetails(false));

    auto *valueCombo = widget.findChild<QComboBox *>(QStringLiteral("valueCombo"));
    auto *writeButton = widget.findChild<QPushButton *>(QStringLiteral("writeButton"));
    QVERIFY(valueCombo);
    QVERIFY(writeButton);

    QSignalSpy spy(&widget, &AttributesWidget::writeRequested);
    valueCombo->setCurrentIndex(1);
    writeButton->click();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=2;s=Device.Level"));
    QCOMPARE(spy.first().at(1).userType(), static_cast<int>(QMetaType::Bool));
    QCOMPARE(spy.first().at(1).toBool(), true);
    QCOMPARE(spy.first().at(2).toInt(), 0);
}

///
/// \brief Every other type keeps the free-text field with its default value.
///
void TestAttributesWidget::nonBooleanNodeKeepsTextField()
{
    AttributesWidget widget;
    widget.setNodeDetails(makeWritableDetails(6, QStringLiteral("ns=0;i=21"),
                                              QStringLiteral("Level")));

    auto *valueCombo = widget.findChild<QComboBox *>(QStringLiteral("valueCombo"));
    auto *valueEdit = widget.findChild<QLineEdit *>(QStringLiteral("valueEdit"));
    QVERIFY(valueCombo);
    QVERIFY(valueEdit);
    QVERIFY(valueEdit->isVisibleTo(&widget));
    QVERIFY(!valueCombo->isVisibleTo(&widget));
}

///
/// \brief The write group and text editor fill the available panel width.
///
void TestAttributesWidget::valueEditorStretchesWithPanel()
{
    AttributesWidget widget;
    auto *writeGroup = widget.findChild<QGroupBox *>(QStringLiteral("writeValueGroup"));
    auto *valueEdit = widget.findChild<QLineEdit *>(QStringLiteral("valueEdit"));
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("attributesTree"));
    QVERIFY(writeGroup);
    QVERIFY(valueEdit);
    QVERIFY(tree);

    widget.resize(700, 400);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.setNodeDetails(makeWritableDetails(12, QStringLiteral("ns=0;i=3"), 0u));
    widget.layout()->activate();

    QCOMPARE(writeGroup->width(), tree->width());
    const int initialEditorWidth = valueEdit->width();
    QVERIFY(initialEditorWidth > valueEdit->minimumWidth());

    widget.resize(800, 400);
    QCoreApplication::processEvents();
    widget.layout()->activate();

    QCOMPARE(writeGroup->width(), tree->width());
    QCOMPARE(valueEdit->width(), initialEditorWidth + 100);
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
