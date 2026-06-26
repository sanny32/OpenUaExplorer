// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventswidget.cpp
/// \brief Implements the OPC UA events tab widget.
///

#include <QHeaderView>

#include "appsettings.h"
#include "eventswidget.h"
#include "headerview.h"
#include "models/eventsmodel.h"
#include "tableview.h"
#include "ui_eventswidget.h"

///
/// \brief Builds the events widget and its table view.
/// \param parent Parent widget.
///
EventsWidget::EventsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EventsWidget)
    , _eventsModel(new EventsModel(this))
{
    ui->setupUi(this);
    setupEventsView();
}

///
/// \brief Destroys the widget and its generated UI.
///
EventsWidget::~EventsWidget()
{
    delete ui;
}

///
/// \brief Removes all displayed events.
///
void EventsWidget::clear()
{
    _eventsModel->setItems({});
}

///
/// \brief Persists the events table header state.
/// \param settings Settings store to write to.
///
void EventsWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->eventsTable->objectName(), ui->eventsTable->headerView()->saveLayout());
}

///
/// \brief Restores the events table header state.
/// \param settings Settings store to read from.
///
void EventsWidget::restoreViewState(AppSettings &settings)
{
    ui->eventsTable->headerView()->restoreLayout(settings.viewState(ui->eventsTable->objectName()));
}

///
/// \brief Binds and lays out the events table.
///
void EventsWidget::setupEventsView()
{
    ui->eventsTable->setModel(_eventsModel);
    ui->eventsTable->verticalHeader()->hide();

    auto *eventsHeader = ui->eventsTable->headerView();
    connect(eventsHeader, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _eventsModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });
    eventsHeader->setSectionResizeMode(EventsModel::ColTime,    QHeaderView::Fixed);
    eventsHeader->setSectionResizeMode(EventsModel::ColMessage, QHeaderView::Stretch);
    ui->eventsTable->setColumnWidth(EventsModel::ColTime, 95);
}
