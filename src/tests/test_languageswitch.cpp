// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_languageswitch.cpp
/// \brief Tests that long-lived widgets rewrite the texts they set from code on a language change.
///

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QToolButton>

#include "application.h"
#include "dialogs/nodemonitordialog.h"
#include "opcua/qtopcuabackend.h"
#include "suffixtranslator.h"
#include "themecoordinator.h"
#include "widgets/favoriteswidget.h"
#include "widgets/nodelineedit.h"
#include "widgets/subscriptioncombobox.h"
#include "widgets/valuelineedit.h"

using TestTranslation::SuffixTranslator;
using TestTranslation::switchLanguage;

namespace {

///
/// \brief Returns the tooltip of the trailing reset action of a line edit.
/// \param edit Line edit carrying the action.
/// \return Tooltip text, or an empty string when the action is missing.
///
QString resetToolTip(const QLineEdit *edit)
{
    const QList<QAction *> actions = edit->actions();
    return actions.isEmpty() ? QString() : actions.constFirst()->toolTip();
}

} // namespace

///
/// \brief Verifies widgets outside the generated retranslation follow the interface language.
///
class TestLanguageSwitch : public QObject
{
    Q_OBJECT

private slots:
    void nodeMonitorKeepsItsTargetAndPauseState();
    void nodeMonitorPlaceholderFollowsTheLanguage();
    void lineEditResetTooltipsFollowTheLanguage();
    void subscriptionComboRelabelsTheCreateEntry();
    void favoritesAddTooltipFollowsTheLanguage();
    void themeTooltipFollowsTheLanguage();
};

///
/// \brief A monitored node keeps its title, NodeId line and Resume label in the new language.
///
void TestLanguageSwitch::nodeMonitorKeepsItsTargetAndPauseState()
{
    QtOpcUaBackend service;
    NodeMonitorDialog dialog(&service);

    OpcUaNodeInfo node;
    node.nodeId = QStringLiteral("ns=2;s=Demo");
    node.displayName = QStringLiteral("Demo");
    node.nodeClass = OpcUa::Variable;
    dialog.setTarget(node);

    auto *name = dialog.findChild<QLabel *>(QStringLiteral("nameLabel"));
    auto *nodeId = dialog.findChild<QLabel *>(QStringLiteral("nodeIdLabel"));
    auto *pause = dialog.findChild<QPushButton *>(QStringLiteral("pauseButton"));
    QVERIFY(name);
    QVERIFY(nodeId);
    QVERIFY(pause);
    pause->setChecked(true);

    SuffixTranslator translator({"NodeMonitorDialog"});
    switchLanguage(&translator, true);
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Node Monitor — Demo-xx"));
    QCOMPARE(name->text(), QStringLiteral("Demo"));
    QCOMPARE(nodeId->text(), QStringLiteral("NodeId: ns=2;s=Demo-xx"));
    QCOMPARE(pause->text(), QStringLiteral("Resume-xx"));

    switchLanguage(&translator, false);
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Node Monitor — Demo"));
    QCOMPARE(pause->text(), QStringLiteral("Resume"));
}

///
/// \brief An empty monitor shows its drop prompt in the new language.
///
void TestLanguageSwitch::nodeMonitorPlaceholderFollowsTheLanguage()
{
    QtOpcUaBackend service;
    NodeMonitorDialog dialog(&service);
    auto *name = dialog.findChild<QLabel *>(QStringLiteral("nameLabel"));
    QVERIFY(name);

    SuffixTranslator translator({"NodeMonitorDialog"});
    switchLanguage(&translator, true);
    QCOMPARE(name->text(), QStringLiteral("No node-xx"));
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Node Monitor-xx"));

    switchLanguage(&translator, false);
    QCOMPARE(name->text(), QStringLiteral("No node"));
}

///
/// \brief Both line edits relabel their reset action, each with its own text.
///
void TestLanguageSwitch::lineEditResetTooltipsFollowTheLanguage()
{
    ValueLineEdit value;
    NodeLineEdit node;
    QCOMPARE(resetToolTip(&value), QStringLiteral("Reset to default value"));
    QCOMPARE(resetToolTip(&node), QStringLiteral("Clear node"));

    SuffixTranslator translator({"ValueLineEdit", "NodeLineEdit"});
    switchLanguage(&translator, true);
    QCOMPARE(resetToolTip(&value), QStringLiteral("Reset to default value-xx"));
    QCOMPARE(resetToolTip(&node), QStringLiteral("Clear node-xx"));

    switchLanguage(&translator, false);
    QCOMPARE(resetToolTip(&node), QStringLiteral("Clear node"));
}

///
/// \brief The create entry is relabelled without losing the selected subscription.
///
void TestLanguageSwitch::subscriptionComboRelabelsTheCreateEntry()
{
    SubscriptionComboBox combo;
    combo.setCreateEntryVisible(true);
    SubscriptionItem fast;
    fast.name = QStringLiteral("Fast");
    fast.id = 7;
    combo.setSubscriptions({fast});
    combo.setCurrentSubscription(QStringLiteral("Fast"));

    SuffixTranslator translator({"SubscriptionComboBox"});
    switchLanguage(&translator, true);
    QVERIFY(combo.findText(QStringLiteral("<New subscription>-xx")) >= 0);
    QCOMPARE(combo.currentSubscription(), QStringLiteral("Fast"));

    switchLanguage(&translator, false);
    QVERIFY(combo.findText(QStringLiteral("<New subscription>")) >= 0);
}

///
/// \brief The add button keeps the tooltip matching its enabled state in the new language.
///
void TestLanguageSwitch::favoritesAddTooltipFollowsTheLanguage()
{
    FavoritesWidget widget;
    widget.setCanAddFavorite(false);
    auto *add = widget.findChild<QAbstractButton *>(QStringLiteral("addButton"));
    QVERIFY(add);

    SuffixTranslator translator({"FavoritesWidget"});
    switchLanguage(&translator, true);
    QCOMPARE(add->toolTip(), QStringLiteral("Connect to a server to add it to favourites-xx"));

    switchLanguage(&translator, false);
    QCOMPARE(add->toolTip(), QStringLiteral("Connect to a server to add it to favourites"));
}

///
/// \brief The theme toggle's tooltip is rewritten when the coordinator retranslates.
///
void TestLanguageSwitch::themeTooltipFollowsTheLanguage()
{
    QAction theme;
    QAction light;
    QAction dark;
    QAction system;
    ThemeCoordinator coordinator(&theme, &light, &dark, &system);
    const QString original = theme.toolTip();
    QVERIFY(!original.isEmpty());

    SuffixTranslator translator({"ThemeCoordinator"});
    switchLanguage(&translator, true);
    coordinator.retranslate();
    QCOMPARE(theme.toolTip(), original + QStringLiteral("-xx"));

    switchLanguage(&translator, false);
    coordinator.retranslate();
    QCOMPARE(theme.toolTip(), original);
}

///
/// \brief Runs the suite under Application so the theme and styles are in place.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestLanguageSwitch test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_languageswitch.moc"
