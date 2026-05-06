#pragma once

#include <QToolButton>

///
/// \brief Tool button that refreshes its icon for the active application theme.
///
class ThemedToolButton : public QToolButton
{
    Q_OBJECT

public:
    explicit ThemedToolButton(QWidget *parent = nullptr);

    void setIcon(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();

    QString _iconName;
};
