#pragma once

#include <QWidget>

namespace Ui {
class ConnectionStatusWidget;
}

class QIcon;

class ConnectionStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionStatusWidget(QWidget *parent = nullptr);
    ~ConnectionStatusWidget() override;

    void setIcon(const QIcon &icon);
    void setStatusText(const QString &text);

private:
    Ui::ConnectionStatusWidget *ui;
};
