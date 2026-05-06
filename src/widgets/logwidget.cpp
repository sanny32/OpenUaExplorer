// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logwidget.cpp
/// \brief Implements the application log widget.
///

#include "headerview.h"
#include "logmodel.h"
#include "logwidget.h"
#include "testdata.h"
#include "ui_logwidget.h"

///
/// \brief LogWidget::LogWidget
/// \param parent
///
LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogWidget)
    , _model(new LogModel(this))
{
    ui->setupUi(this);
    setupLogView();

    for (const LogItem &item : TestData::logItems())
        _model->addItem(item);

    connect(ui->clearButton, &QPushButton::clicked, _model, &LogModel::clear);

    connect(ui->levelCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        switch (index) {
        case 0: _model->clearFilterLevel();                        break;
        case 1: _model->setFilterLevel(LogItem::Level::Info);     break;
        case 2: _model->setFilterLevel(LogItem::Level::Warning);  break;
        case 3: _model->setFilterLevel(LogItem::Level::Error);    break;
        }
    });
}

///
/// \brief LogWidget::~LogWidget
///
LogWidget::~LogWidget()
{
    delete ui;
}

///
/// \brief LogWidget::addItem
/// \param item
///
void LogWidget::addItem(const LogItem &item)
{
    _model->addItem(item);
}

///
/// \brief LogWidget::setupLogView
///
void LogWidget::setupLogView()
{
    auto header = new HeaderView(Qt::Horizontal, ui->logTable);
    connect(header, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _model->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    ui->logTable->setModel(_model);
    ui->logTable->setHorizontalHeader(header);
    ui->logTable->verticalHeader()->hide();

    header->setStretchLastSection(false);
    header->setSectionResizeMode(LogModel::ColTimestamp, QHeaderView::Fixed);
    header->setSectionResizeMode(LogModel::ColLevel,     QHeaderView::Fixed);
    header->setSectionResizeMode(LogModel::ColSource,    QHeaderView::Fixed);
    header->setSectionResizeMode(LogModel::ColMessage,   QHeaderView::Stretch);

    header->setSectionAlignment(LogModel::ColLevel,  Qt::AlignCenter);
    header->setSectionAlignment(LogModel::ColSource, Qt::AlignCenter);

    ui->logTable->setColumnWidth(LogModel::ColTimestamp, 95);
    ui->logTable->setColumnWidth(LogModel::ColLevel,     80);
    ui->logTable->setColumnWidth(LogModel::ColSource,    60);
}
