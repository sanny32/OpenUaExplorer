// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectionstatuswidget.cpp
/// \brief Implements the connection status widget.
///

#include <QIcon>

#include "connectionstatuswidget.h"
#include "ui_connectionstatuswidget.h"

///
/// \brief Builds the connection status widget.
/// \param parent Parent widget.
///
ConnectionStatusWidget::ConnectionStatusWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ConnectionStatusWidget)
{
    ui->setupUi(this);
    setStatusText(QStringLiteral("Connected"));
}

///
/// \brief Destroys the widget and its generated UI.
///
ConnectionStatusWidget::~ConnectionStatusWidget()
{
    delete ui;
}

///
/// \brief Sets the status icon.
/// \param icon Icon shown next to the status text.
///
void ConnectionStatusWidget::setIcon(const QIcon &icon)
{
    ui->statusIconLabel->setPixmap(icon.pixmap(12, 12));
}

///
/// \brief Sets the status text.
/// \param text Text to display.
///
void ConnectionStatusWidget::setStatusText(const QString &text)
{
    ui->statusTextLabel->setText(text);
}
