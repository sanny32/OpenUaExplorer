#pragma once

#include <QPushButton>

class ThemedPushButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ThemedPushButton(QWidget *parent = nullptr);

    void setIcon(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
};
