// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedtoolbutton.h
/// \brief Declares a theme-aware tool button.
///

#pragma once

#include <QSize>
#include <QToolButton>

///
/// \brief Tool button that refreshes its icon for the active application theme.
///
class ThemedToolButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(bool squareIconOnly READ squareIconOnly WRITE setSquareIconOnly)

public:
    explicit ThemedToolButton(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool squareIconOnly() const;
    void setSquareIconOnly(bool enabled);

    void setIcon(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshIcon();
    QSize squareSize(const QSize &size) const;

    bool _squareIconOnly = false;
    QString _iconName;
};
