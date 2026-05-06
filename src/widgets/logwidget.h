// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logwidget.h
/// \brief Declares the application log widget.
///

#pragma once

#include <QWidget>

#include "logitem.h"

namespace Ui {
class LogWidget;
}

class LogModel;

///
/// \brief Widget that displays and filters application log messages.
///
class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget() override;

    void addItem(const LogItem &item);

private:
    void setupLogView();

    Ui::LogWidget *ui;
    LogModel      *_model;
};
