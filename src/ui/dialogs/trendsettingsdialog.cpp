// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendsettingsdialog.cpp
/// \brief Implements the per-chart trend settings dialog.
///

#include "trendsettingsdialog.h"
#include "ui_trendsettingsdialog.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QEvent>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include "appcolors.h"

namespace {

constexpr qint64 kLiveWindowMs = 60000;

/// \brief Item-data role carrying a series' line colour (QColor).
constexpr int kColorRole = Qt::UserRole + 1;
/// \brief Item-data role carrying a series' NodeId (QString).
constexpr int kNodeIdRole = Qt::UserRole + 2;

constexpr int kSwatchWidth = 40;
constexpr int kSwatchHeight = 18;
constexpr int kSwatchMargin = 8;
constexpr int kRowHeight = 30;

///
/// \brief List delegate that draws a colour swatch and edits it on click.
///
/// The standard checkbox and label are painted by the base delegate; this adds a
/// rounded colour swatch on the right edge and opens a colour picker when the
/// swatch is clicked, writing the result back to the model's colour role.
///
class SeriesColorDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), kRowHeight));
        return size;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        const QColor color = index.data(kColorRole).value<QColor>();
        if (!color.isValid())
            return;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawRoundedRect(swatchRect(option.rect), 4, 4);
        painter->restore();
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (swatchRect(option.rect).contains(mouseEvent->position().toPoint())) {
                const QColor current = index.data(kColorRole).value<QColor>();
                auto *parent = const_cast<QWidget *>(option.widget);
                const QColor chosen = QColorDialog::getColor(current, parent,
                                                             tr("Series Colour"));
                if (chosen.isValid())
                    model->setData(index, chosen, kColorRole);
                return true;
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

private:
    static QRect swatchRect(const QRect &itemRect)
    {
        return QRect(itemRect.right() - kSwatchWidth - kSwatchMargin,
                     itemRect.center().y() - kSwatchHeight / 2,
                     kSwatchWidth, kSwatchHeight);
    }
};

}

///
/// \brief Builds the dialog from its generated UI and themed styling.
/// \param parent Parent widget.
///
TrendSettingsDialog::TrendSettingsDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::TrendSettingsDialog)
{
    ui->setupUi(this);

    _periodGroup = new QButtonGroup(this);
    _periodGroup->setExclusive(true);
    _periodGroup->addButton(ui->liveButton);
    _periodGroup->addButton(ui->oneMinuteButton);
    _periodGroup->addButton(ui->tenMinutesButton);
    _periodGroup->addButton(ui->oneHourButton);
    _periodGroup->addButton(ui->oneDayButton);

    _seriesModel = new QStandardItemModel(this);
    ui->seriesList->setModel(_seriesModel);
    ui->seriesList->setItemDelegate(new SeriesColorDelegate(this));

    ui->okButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });

    connect(ui->resetButton, &QPushButton::clicked, this,
            &TrendSettingsDialog::resetToDefaults);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->okButton, &QPushButton::clicked, this, &QDialog::accept);
}

///
/// \brief Destroys the dialog and its generated UI.
///
TrendSettingsDialog::~TrendSettingsDialog()
{
    delete ui;
}

///
/// \brief Pre-fills the display, range and auto-scroll controls.
/// \param settings Current chart settings.
///
void TrendSettingsDialog::setDisplaySettings(const TrendDisplaySettings &settings)
{
    ui->autoScaleCheck->setChecked(settings.autoScale);
    ui->showLegendCheck->setChecked(settings.showLegend);
    ui->showGridCheck->setChecked(settings.showGrid);
    ui->smoothLinesCheck->setChecked(settings.smoothLines);
    ui->showTooltipCheck->setChecked(settings.showValueTooltip);
    ui->labelModeCombo->setCurrentIndex(static_cast<int>(settings.labelMode));
    ui->autoScrollCheck->setChecked(settings.autoScrollLive);
    ui->liveUpdateSpin->setValue(settings.liveUpdateMs);
    selectPeriod(settings.mode, settings.windowMs);
}

///
/// \brief Returns the edited display, range and auto-scroll settings.
/// \return Updated settings.
///
TrendDisplaySettings TrendSettingsDialog::displaySettings() const
{
    TrendDisplaySettings settings;
    settings.autoScale = ui->autoScaleCheck->isChecked();
    settings.showLegend = ui->showLegendCheck->isChecked();
    settings.showGrid = ui->showGridCheck->isChecked();
    settings.smoothLines = ui->smoothLinesCheck->isChecked();
    settings.showValueTooltip = ui->showTooltipCheck->isChecked();
    settings.labelMode = static_cast<TrendLabelMode>(ui->labelModeCombo->currentIndex());
    settings.autoScrollLive = ui->autoScrollCheck->isChecked();
    settings.liveUpdateMs = ui->liveUpdateSpin->value();

    if (ui->oneMinuteButton->isChecked()) {
        settings.mode = 1;
        settings.windowMs = 60000;
    } else if (ui->tenMinutesButton->isChecked()) {
        settings.mode = 1;
        settings.windowMs = 600000;
    } else if (ui->oneHourButton->isChecked()) {
        settings.mode = 1;
        settings.windowMs = 3600000;
    } else if (ui->oneDayButton->isChecked()) {
        settings.mode = 1;
        settings.windowMs = 86400000;
    } else {
        settings.mode = 0;
        settings.windowMs = kLiveWindowMs;
    }
    return settings;
}

///
/// \brief Checks the period button matching a mode and window.
/// \param mode 0 for Live, 1 for History.
/// \param windowMs History window length in milliseconds.
///
void TrendSettingsDialog::selectPeriod(int mode, qint64 windowMs)
{
    if (mode == 0) {
        ui->liveButton->setChecked(true);
        return;
    }
    switch (windowMs) {
    case 600000:   ui->tenMinutesButton->setChecked(true); break;
    case 3600000:  ui->oneHourButton->setChecked(true); break;
    case 86400000: ui->oneDayButton->setChecked(true); break;
    default:       ui->oneMinuteButton->setChecked(true); break;
    }
}

///
/// \brief Populates the series list model, one checkable row per series.
/// \param series Current series in display order.
///
void TrendSettingsDialog::setSeries(const QVector<TrendSeriesInfo> &series)
{
    _seriesModel->clear();
    for (const TrendSeriesInfo &info : series) {
        auto *item = new QStandardItem(info.label);
        item->setEditable(false);
        item->setCheckable(true);
        item->setCheckState(info.visible ? Qt::Checked : Qt::Unchecked);
        item->setData(info.color, kColorRole);
        item->setData(info.nodeId, kNodeIdRole);
        _seriesModel->appendRow(item);
    }
}

///
/// \brief Returns the series with their edited visibility and colours.
/// \return Updated series in the original order.
///
QVector<TrendSeriesInfo> TrendSettingsDialog::series() const
{
    QVector<TrendSeriesInfo> result;
    result.reserve(_seriesModel->rowCount());
    for (int row = 0; row < _seriesModel->rowCount(); ++row) {
        const QStandardItem *item = _seriesModel->item(row);
        TrendSeriesInfo info;
        info.nodeId = item->data(kNodeIdRole).toString();
        info.label = item->text();
        info.color = item->data(kColorRole).value<QColor>();
        info.visible = item->checkState() == Qt::Checked;
        result.append(info);
    }
    return result;
}

///
/// \brief Restores the display, range and visibility controls to their defaults.
///
void TrendSettingsDialog::resetToDefaults()
{
    setDisplaySettings(TrendDisplaySettings{});
    for (int row = 0; row < _seriesModel->rowCount(); ++row)
        _seriesModel->item(row)->setCheckState(Qt::Checked);
}
