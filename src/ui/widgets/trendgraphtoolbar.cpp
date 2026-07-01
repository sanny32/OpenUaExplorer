// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphtoolbar.cpp
/// \brief Implements the trend graph toolbar.
///

#include "trendgraphtoolbar.h"
#include "ui_trendgraphtoolbar.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QEvent>
#include <QSignalBlocker>
#include <QSize>

#include "appcolors.h"

namespace {

constexpr int kLiveModeId = 0;
constexpr int kCustomModeId = -2;
constexpr int kOneMinuteMs = 60000;
constexpr int kTenMinutesMs = 600000;
constexpr int kOneHourMs = 3600000;

} // namespace

///
/// \brief Builds the toolbar and wires its command signals.
/// \param parent Parent widget.
///
TrendGraphToolbar::TrendGraphToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrendGraphToolbar)
    , _modeGroup(new QButtonGroup(this))
{
    ui->setupUi(this);

    _modeGroup->setExclusive(true);
    _modeGroup->addButton(ui->liveButton, kLiveModeId);
    _modeGroup->addButton(ui->oneMinuteButton, kOneMinuteMs);
    _modeGroup->addButton(ui->tenMinutesButton, kTenMinutesMs);
    _modeGroup->addButton(ui->oneHourButton, kOneHourMs);
    _modeGroup->addButton(ui->customButton, kCustomModeId);

    ui->durationIcon->setIcon(QStringLiteral("clock"), QSize(14, 14));
    ui->durationBadge->setAttribute(Qt::WA_StyledBackground, true);
    applyIntervalStyling();

    connect(_modeGroup, &QButtonGroup::idClicked,
            this, &TrendGraphToolbar::handleModeClicked);
    connect(ui->livePauseButton, &QAbstractButton::clicked,
            this, &TrendGraphToolbar::handleLivePauseClicked);
    connect(ui->refreshButton, &QAbstractButton::clicked,
            this, &TrendGraphToolbar::refreshRequested);
    connect(ui->autoScaleButton, &QAbstractButton::clicked,
            this, &TrendGraphToolbar::autoScaleRequested);
    connect(ui->fitButton, &QAbstractButton::clicked,
            this, &TrendGraphToolbar::fitRequested);
    connect(ui->exportButton, &QAbstractButton::clicked,
            this, &TrendGraphToolbar::exportRequested);
    connect(ui->settingsButton, &QAbstractButton::clicked,
            this, &TrendGraphToolbar::settingsRequested);
}

///
/// \brief Destroys the generated UI.
///
TrendGraphToolbar::~TrendGraphToolbar()
{
    delete ui;
}

///
/// \brief Marks Live mode active.
///
void TrendGraphToolbar::selectLive()
{
    const QSignalBlocker blocker(_modeGroup);
    ui->liveButton->setChecked(true);
    ui->livePauseButton->setEnabled(true);
    setLivePaused(false);
}

///
/// \brief Marks the matching history range active.
/// \param windowMs History window length in milliseconds.
///
void TrendGraphToolbar::selectHistoryWindow(qint64 windowMs)
{
    const QSignalBlocker blocker(_modeGroup);
    switch (windowMs) {
    case kOneMinuteMs:  ui->oneMinuteButton->setChecked(true); break;
    case kTenMinutesMs: ui->tenMinutesButton->setChecked(true); break;
    case kOneHourMs:    ui->oneHourButton->setChecked(true); break;
    default:            ui->customButton->setChecked(true); break;
    }
    setLivePaused(false);
    ui->livePauseButton->setEnabled(false);
}

///
/// \brief Marks the custom interval command active.
///
void TrendGraphToolbar::selectCustom()
{
    const QSignalBlocker blocker(_modeGroup);
    ui->customButton->setChecked(true);
    setLivePaused(false);
    ui->livePauseButton->setEnabled(false);
}

///
/// \brief Shows the interval length beside the Custom button.
/// \param range Interval range text; shown as the badge tooltip on hover.
/// \param duration Human-readable interval length; an empty string hides the badge.
///
void TrendGraphToolbar::setInterval(const QString &range, const QString &duration)
{
    ui->durationLabel->setText(duration);
    ui->durationBadge->setToolTip(range);
    ui->durationIcon->setToolTip(range);
    ui->durationLabel->setToolTip(range);
    ui->intervalGroup->setVisible(!duration.isEmpty());
}

///
/// \brief Restyles the interval label and duration badge for the active theme.
///
void TrendGraphToolbar::applyIntervalStyling()
{
    ui->durationLabel->setStyleSheet(
        QStringLiteral("color: %1;").arg(AppColors::intervalBadgeText().name()));
    ui->durationBadge->setStyleSheet(
        QStringLiteral("#durationBadge { background-color: %1; border: 1px solid %2; "
                       "border-radius: 6px; }")
            .arg(AppColors::intervalBadgeBackground().name(),
                 AppColors::intervalBadgeBorder().name()));
}

///
/// \brief Restyles the interval display when the colour scheme changes.
///
void TrendGraphToolbar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        applyIntervalStyling();
    }
}

///
/// \brief Enables the history refresh command.
/// \param enabled Whether refresh can be clicked.
///
void TrendGraphToolbar::setRefreshEnabled(bool enabled)
{
    ui->refreshButton->setEnabled(enabled);
}

///
/// \brief Syncs the live play/pause control without emitting a request.
/// \param paused True to show the paused (resume) state.
///
void TrendGraphToolbar::setLivePaused(bool paused)
{
    _livePaused = paused;
    updateLivePauseButton();
}

///
/// \brief Emits the mode request for the clicked range button.
/// \param id Button id registered in the mode group.
///
void TrendGraphToolbar::handleModeClicked(int id)
{
    if (id == kLiveModeId)
        emit liveRequested();
    else if (id == kCustomModeId)
        emit customIntervalRequested();
    else
        emit historyRequested(id);
}

///
/// \brief Flips the paused state, updates the icon and forwards the request.
///
void TrendGraphToolbar::handleLivePauseClicked()
{
    _livePaused = !_livePaused;
    updateLivePauseButton();
    emit livePauseToggled(_livePaused);
}

///
/// \brief Reflects the paused state in the play/pause icon and tooltip.
///
void TrendGraphToolbar::updateLivePauseButton()
{
    ui->livePauseButton->setIcon(_livePaused ? QStringLiteral("resume")
                                             : QStringLiteral("pause"));
    ui->livePauseButton->setToolTip(_livePaused ? tr("Resume") : tr("Pause"));
}
