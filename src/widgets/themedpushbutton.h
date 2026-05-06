#pragma once

#include <QPushButton>

///
/// \brief Push button that refreshes its icon for the active application theme.
///
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
