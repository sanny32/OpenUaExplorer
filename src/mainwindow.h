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

private:
    void openConnectionDialog();
    void setupMainMenu();
    void setupDockOptions();
    void resetLayout();
    void bindIcons();
    void toggleTheme();
    void populateWithTestData();

    Ui::MainWindow *ui;
};
