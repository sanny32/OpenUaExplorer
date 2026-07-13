// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logwidget.h
/// \brief Declares the activity log widget.
///

#pragma once

#include <QSet>
#include <QWidget>

#include "models/logitem.h"

namespace Ui {
class LogWidget;
}

class AppSettings;
class LogModel;
class ThemedAction;

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

    ///
    /// \brief Persists the log table header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the log table header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

    ///
    /// \brief Prompts for a file and writes the visible log rows to it.
    ///
    void exportLog();

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupLogView();
    void refreshIcons();
    void scrollToBottom();
    void registerSource(const QString &source);
    QString rowsAsText(const QList<int> &rows) const;
    void copyRows(const QList<int> &rows);
    void copySelection();
    void copyAll();
    void showContextMenu(const QPoint &pos);

    Ui::LogWidget *ui;
    LogModel      *_model;
    ThemedAction  *_searchIconAction = nullptr;
    ThemedAction  *_copyAction = nullptr;
    ThemedAction  *_copyAllAction = nullptr;
    bool           _paused = false;
    QSet<QString>  _knownSources;
};
