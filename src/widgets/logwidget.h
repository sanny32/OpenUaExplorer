// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logwidget.h
/// \brief Declares the activity log widget.
///

#pragma once

#include <QAction>
#include <QLoggingCategory>
#include <QWidget>

#include "itestdatapopulatable.h"
#include "logitem.h"

Q_DECLARE_LOGGING_CATEGORY(lcApp)
Q_DECLARE_LOGGING_CATEGORY(lcClient)
Q_DECLARE_LOGGING_CATEGORY(lcSession)

namespace Ui {
class LogWidget;
}

class LogModel;

///
/// \brief Widget that displays, filters and searches application activity log messages.
///
class LogWidget : public QWidget, public ITestDataPopulatable
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget() override;

    void addItem(const LogItem &item);
    void populateWithTestData() override;

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupLogView();
    void refreshIcons();
    void scrollToBottom();

    Ui::LogWidget *ui;
    LogModel      *_model;
    QAction       *_searchIconAction = nullptr;
    bool           _paused = false;
};
