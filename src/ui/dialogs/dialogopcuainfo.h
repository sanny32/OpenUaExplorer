// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogopcuainfo.h
/// \brief Declares the OPC UA information dialog.
///

#pragma once

#include <QString>
#include <QStringList>

#include "dialogs/appbasedialog.h"

namespace Ui {
class DialogOpcUaInfo;
}

///
/// \brief Dialog that presents local OPC UA stack capabilities and reference links.
///
class DialogOpcUaInfo : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the OPC UA information dialog and fills its runtime fields.
    /// \param parent Parent widget.
    ///
    explicit DialogOpcUaInfo(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~DialogOpcUaInfo() override;

    ///
    /// \brief Returns the plain-text diagnostic summary copied by the dialog.
    /// \return Current OPC UA information summary.
    ///
    QString copyText() const;

    ///
    /// \brief Returns security policies reported by the first available backend.
    /// \return Supported security policy display names.
    ///
    QStringList supportedSecurityPolicies() const;

private slots:
    void copyInfo();

private:
    void setupContent();
    void setupFonts();
    void setupLayout();
    void setupLogo();
    void setupLinks();
    void setRowValue(const QString &objectName, const QString &value);
    QString rowValue(const QString &objectName) const;
    static QStringList fallbackSecurityPolicies();
    static QString securityPolicyName(const QString &policyUri);
    static QString displayList(const QStringList &values);

    Ui::DialogOpcUaInfo *ui;
};
