#pragma once

#include <QLabel>

///
/// \brief Label that displays a theme-aware icon pixmap.
///
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
