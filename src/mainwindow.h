#pragma once

#include <QIcon>
#include <QMainWindow>
#include <QString>

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
    enum class IconTheme {
        Light,
        Dark
    };

    void openConnectionDialog();
    void setupDockOptions();
    void applyThemeIcons();
    IconTheme currentIconTheme() const;
    QIcon themedIcon(const QString &name) const;

    Ui::MainWindow *ui;
    MainStatusBarWidget *_mainStatusBarWidget;
};
