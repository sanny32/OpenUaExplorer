#pragma once

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainStatusBarWidget;

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
    void setupDockOptions();
    void applyThemeIcons();
    void toggleTheme();

    Ui::MainWindow *ui;
    MainStatusBarWidget *_mainStatusBarWidget;
};
