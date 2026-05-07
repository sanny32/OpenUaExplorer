// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.h
/// \brief Declares the main application window.
///

#pragma once

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

///
/// \brief Main application window that coordinates docks, toolbar and theme actions.
///
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void on_actionNewConnection_triggered();
    void on_actionConnect_triggered();
    void on_actionExit_triggered();
    void on_actionTheme_triggered();
    void on_actionAbout_triggered();
    void on_actionViewAddressSpace_toggled(bool checked);
    void on_addressSpaceDock_visibilityChanged(bool visible);
    void on_actionViewActivity_toggled(bool checked);
    void on_logDock_visibilityChanged(bool visible);
    void on_actionViewDataAccess_triggered();
    void on_actionViewSubscriptions_triggered();
    void on_actionViewEvents_triggered();
    void on_actionViewHistory_triggered();
    void on_actionResetLayout_triggered();

private:
    void openConnectionDialog();
    void setupMainMenu();
    void setupDockOptions();
    void resetLayout();
    void bindIcons();
    void toggleTheme();
    void populateWithTestData();

private:
    Ui::MainWindow *ui;
};
