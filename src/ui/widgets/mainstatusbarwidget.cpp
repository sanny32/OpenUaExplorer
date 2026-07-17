// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.cpp
/// \brief Implements the main status bar widget.
///

#include <QDateTime>
#include <QEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QTimer>
#include <QWidget>

#include "mainstatusbarwidget.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuabackend.h"
#include "opcua/standardnodeid.h"
#include "ui_mainstatusbarwidget.h"

namespace {

///
/// \brief Formats the UTC offset of a local time as a compact "UTC+H[:MM]" suffix.
/// \param dateTime Local date-time whose offset should be described.
/// \return Offset label such as "UTC", "UTC+3", or "UTC-05:30".
///
QString utcOffsetLabel(const QDateTime &dateTime)
{
    const int offsetSeconds = dateTime.offsetFromUtc();
    if (offsetSeconds == 0)
        return QStringLiteral("UTC");

    const int totalMinutes = qAbs(offsetSeconds) / 60;
    const QChar sign = offsetSeconds > 0 ? QLatin1Char('+') : QLatin1Char('-');
    if (totalMinutes % 60 == 0)
        return QStringLiteral("UTC%1%2").arg(sign).arg(totalMinutes / 60);
    return QStringLiteral("UTC%1%2:%3")
        .arg(sign)
        .arg(totalMinutes / 60, 2, 10, QLatin1Char('0'))
        .arg(totalMinutes % 60, 2, 10, QLatin1Char('0'));
}

///
/// \brief Builds a clock value from the widest digit in the current font.
/// \param metrics Font metrics used by the clock label.
/// \return A value shaped as HH:mm:ss that reserves the maximum digit width.
///
QString widestClockValue(const QFontMetrics &metrics)
{
    QChar widestDigit = QLatin1Char('0');
    int widestAdvance = metrics.horizontalAdvance(widestDigit);
    for (char digitValue = '1'; digitValue <= '9'; ++digitValue) {
        const QChar digit = QLatin1Char(digitValue);
        const int advance = metrics.horizontalAdvance(digit);
        if (advance > widestAdvance) {
            widestDigit = digit;
            widestAdvance = advance;
        }
    }

    return QStringLiteral("%1%1:%1%1:%1%1").arg(widestDigit);
}

}

///
/// \brief Builds the status bar and shows the disconnected state.
/// \param parent Parent widget.
///
MainStatusBarWidget::MainStatusBarWidget(QWidget *parent)
    : QStatusBar(parent)
    , ui(new Ui::MainStatusBarWidget)
{
    auto *content = new QWidget(this);
    ui->setupUi(content);
    setupFieldDecorations();
    setConnectionState(OpcUaConnectionState::Disconnected);
    addWidget(content, 1);

    auto *clock = new QTimer(this);
    clock->setInterval(1000);
    connect(clock, &QTimer::timeout, this, &MainStatusBarWidget::updateClocks);
    clock->start();
    updateClocks();
}

///
/// \brief Assigns the field icons and category tooltips that replace the text prefixes.
///
void MainStatusBarWidget::setupFieldDecorations()
{
    ui->securityIconLabel->setIcon(QStringLiteral("lock"), QSize(12, 12));
    ui->authenticationIconLabel->setIcon(QStringLiteral("user"), QSize(12, 12));
    ui->sessionIconLabel->setIcon(QStringLiteral("keyhole"), QSize(12, 12));

    const auto setFieldTooltip = [](QWidget *icon, QWidget *value, const QString &tip) {
        icon->setToolTip(tip);
        value->setToolTip(tip);
    };
    setFieldTooltip(ui->statusIconLabel, ui->connectionLabel, tr("Connection"));
    setFieldTooltip(ui->securityIconLabel, ui->securityLabel, tr("Security policy / mode"));
    setFieldTooltip(ui->authenticationIconLabel, ui->authenticationLabel,
                    tr("Authentication method"));
    setFieldTooltip(ui->sessionIconLabel, ui->sessionLabel, tr("Session"));
    ui->serverTimeLabel->setToolTip(tr("Server time (UTC)"));
    ui->localTimeLabel->setToolTip(tr("Local time"));
}

///
/// \brief Reserves enough space for both clocks in the current font and time-zone format.
/// \param localUtcOffset UTC suffix currently displayed by the local clock.
///
void MainStatusBarWidget::updateClockWidths(const QString &localUtcOffset)
{
    const QString serverValue = widestClockValue(ui->serverTimeLabel->fontMetrics());
    ui->serverTimeLabel->setMinimumWidth(
        ui->serverTimeLabel->fontMetrics().horizontalAdvance(
            tr("Server Time: %1 UTC").arg(serverValue)));

    const QString localValue = widestClockValue(ui->localTimeLabel->fontMetrics());
    ui->localTimeLabel->setMinimumWidth(
        ui->localTimeLabel->fontMetrics().horizontalAdvance(
            tr("Local Time: %1 %2").arg(localValue, localUtcOffset)));
}

///
/// \brief Refreshes the local- and server-time labels every tick.
///
void MainStatusBarWidget::updateClocks()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString localUtcOffset = utcOffsetLabel(now);
    updateClockWidths(localUtcOffset);
    ui->localTimeLabel->setText(tr("Local Time: %1 %2")
        .arg(now.toString(QStringLiteral("HH:mm:ss")), localUtcOffset));

    if (_serverTimeKnown) {
        const QDateTime serverNow =
            now.toUTC().addMSecs(_serverTimeOffsetMs);
        ui->serverTimeLabel->setText(
            tr("Server Time: %1 UTC").arg(serverNow.toString(QStringLiteral("HH:mm:ss"))));
    }
}

///
/// \brief Captures the server clock from a read of the CurrentTime node.
/// \param values Read results.
/// \param error Read error, empty on success.
///
void MainStatusBarWidget::handleServerTime(const QVector<OpcUaDataValue> &values,
                                           const QString &error)
{
    if (!error.isEmpty())
        return;
    for (const OpcUaDataValue &value : values) {
        if (value.nodeId != StandardNodeId::serverCurrentTime())
            continue;
        const QDateTime serverTime = value.value.toDateTime().isValid()
            ? value.value.toDateTime()
            : value.serverTimestamp;
        if (!serverTime.isValid())
            return;
        _serverTimeOffsetMs =
            QDateTime::currentDateTimeUtc().msecsTo(serverTime.toUTC());
        _serverTimeKnown = true;
        updateClocks();
        return;
    }
}

namespace {

///
/// \brief Builds the abbreviated "policy / mode" security text shown in the status bar.
/// \param securityPolicy Security policy URI (or short name).
/// \param securityMode Message security mode value (None=1, Sign=2, Sign & Encrypt=3).
/// \return Compact security description, or an empty string when no policy is set.
///
QString securitySummary(const QString &securityPolicy, int securityMode)
{
    if (securityPolicy.isEmpty())
        return QString();

    const QString policy = securityPolicy.section(QLatin1Char('#'), -1);
    QString mode;
    switch (securityMode) {
    case 2: mode = MainStatusBarWidget::tr("Sign"); break;
    case 3: mode = MainStatusBarWidget::tr("Sign & Encrypt"); break;
    default: break; // None / Invalid: the policy name already reads "None".
    }
    return mode.isEmpty() ? policy : QStringLiteral("%1 / %2").arg(policy, mode);
}

///
/// \brief Builds the authentication-method text shown in the status bar.
/// \param profile Active connection profile.
/// \return Authentication label, naming the user for username authentication.
///
QString authenticationSummary(const ConnectionProfile &profile)
{
    switch (profile.authentication) {
    case ConnectionProfile::Authentication::Username:
        return profile.username.isEmpty()
            ? MainStatusBarWidget::tr("Username")
            : QStringLiteral("%1 (%2)").arg(MainStatusBarWidget::tr("Username"),
                                            profile.username);
    case ConnectionProfile::Authentication::Certificate:
        return MainStatusBarWidget::tr("Certificate");
    case ConnectionProfile::Authentication::Anonymous:
        break;
    }
    return MainStatusBarWidget::tr("Anonymous");
}

}

///
/// \brief Subscribes to the controller's connection state and reflects it in the labels.
/// \param controller Connection controller providing the backend and active profile.
///
void MainStatusBarWidget::setConnectionController(ConnectionController *controller)
{
    _controller = controller;
    connect(controller->backend(), &OpcUaBackend::stateChanged,
            this, &MainStatusBarWidget::updateConnectionState);
    connect(controller->backend(), &OpcUaBackend::dataValuesReady,
            this, &MainStatusBarWidget::handleServerTime);
    updateConnectionState(controller->backend()->state());
}

///
/// \brief Refreshes the labels from the current state and the active profile.
/// \param state Current client state.
///
void MainStatusBarWidget::updateConnectionState(OpcUaConnectionState state)
{
    const ConnectionProfile &profile = _controller->activeProfile();
    const bool connected = state == OpcUaConnectionState::Connected;
    const bool active = connected || state == OpcUaConnectionState::Connecting;
    setConnectionState(state, profile.endpointUrl,
                       active ? profile.securityPolicy : QString(),
                       active ? profile.securityMode : 0,
                       connected ? _controller->activeSessionName() : QString(),
                       active ? authenticationSummary(profile) : QString());

    if (connected) {
        _controller->backend()->readValues(
            { StandardNodeId::serverCurrentTime() });
    } else {
        _serverTimeKnown = false;
    }
}

///
/// \brief Recomputes clock widths when translated text or visual metrics change.
/// \param event Change event being handled.
///
void MainStatusBarWidget::changeEvent(QEvent *event)
{
    QStatusBar::changeEvent(event);

    switch (event->type()) {
    case QEvent::LanguageChange:
        if (_controller)
            updateConnectionState(_controller->backend()->state());
        else
            setConnectionState(OpcUaConnectionState::Disconnected);
        updateClocks();
        break;
    case QEvent::FontChange:
    case QEvent::StyleChange:
        updateClocks();
        break;
    default:
        break;
    }
}

///
/// \brief Updates the status text, icon, security, and session labels for the connection state.
/// \param state Current client state.
/// \param endpoint Connected endpoint.
/// \param securityPolicy Selected security policy URI.
/// \param securityMode Selected message security mode value.
/// \param sessionName Active session name.
/// \param authentication Authentication-method description.
///
void MainStatusBarWidget::setConnectionState(OpcUaConnectionState state,
                                             const QString &endpoint,
                                             const QString &securityPolicy,
                                             int securityMode,
                                             const QString &sessionName,
                                             const QString &authentication)
{
    QString text;
    QString icon = QStringLiteral("disconnected");
    switch (state) {
    case OpcUaConnectionState::Unavailable:
        text = tr("OPC UA unavailable");
        break;
    case OpcUaConnectionState::Discovering:
        text = tr("Discovering endpoints");
        break;
    case OpcUaConnectionState::Connecting:
        text = tr("Connecting: %1").arg(endpoint);
        icon = QStringLiteral("connect");
        break;
    case OpcUaConnectionState::Connected:
        text = endpoint;
        icon = QStringLiteral("connected");
        break;
    case OpcUaConnectionState::Closing:
        text = tr("Disconnecting");
        icon = QStringLiteral("disconnect");
        break;
    case OpcUaConnectionState::Disconnected:
        text = tr("Disconnected");
        break;
    }
    const QString security = securitySummary(securityPolicy, securityMode);
    ui->statusIconLabel->setIcon(icon, QSize(12, 12));
    ui->connectionLabel->setText(text);
    ui->securityLabel->setText(security.isEmpty() ? QStringLiteral("-") : security);
    ui->authenticationLabel->setText(authentication.isEmpty() ? QStringLiteral("-") : authentication);
    ui->sessionLabel->setText(sessionName.isEmpty() ? QStringLiteral("-") : sessionName);
    ui->serverTimeLabel->setText(tr("Server Time: -"));
}

///
/// \brief Destroys the status bar and its generated UI.
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}
