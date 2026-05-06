#pragma once

#include <QLabel>

class ThemedIconLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ThemedIconLabel(QWidget *parent = nullptr);

    void setIcon(const QString &name, int size);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
    int     _size = 0;
};
