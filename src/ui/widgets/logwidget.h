// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logwidget.h
/// \brief Declares the activity log widget.
///

#pragma once

#include <QAction>
#include <QWidget>

#include "models/logitem.h"

namespace Ui {
class LogWidget;
}

class LogModel;

///
/// \brief Widget that displays, filters and searches application activity log messages.
///
class LogWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the log widget, installs the message handler, and wires its controls.
    /// \param parent Parent widget.
    ///
    explicit LogWidget(QWidget *parent = nullptr);

    ///
    /// \brief Restores the previous message handler and destroys the widget.
    ///
    ~LogWidget() override;

    ///
    /// \brief Appends a log entry unless logging is paused.
    /// \param item Log entry to add.
    ///
    void addItem(const LogItem &item);

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
