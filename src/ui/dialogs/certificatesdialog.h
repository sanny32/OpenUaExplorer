// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatesdialog.h
/// \brief Declares the PKI certificate management dialog.
///

#pragma once

#include <QByteArray>
#include <QString>

#include "appbasedialog.h"
#include "opcua/pkimanager.h"

class QModelIndex;
class QStandardItem;
class QStandardItemModel;

namespace Ui {
class CertificatesDialog;
}

///
/// \brief Manages the client certificate and the trusted/rejected trust store.
///
/// Edits are staged in memory and written to the PKI directory only on Apply, so
/// closing the dialog without applying leaves the store untouched.
///
class CertificatesDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog, loads the client certificate and trust store.
    /// \param parent Parent widget.
    ///
    explicit CertificatesDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~CertificatesDialog() override;

protected:
    void reject() override;

private slots:
    void viewClientCertificate();
    void viewPrivateKey();
    void importClientCertificate();
    void replaceClientCertificate();
    void exportClientCertificate();
    void removeClientCertificate();

    void updateTrustSelectionActions();
    void trustSelected();
    void rejectSelected();
    void removeSelected();
    void showSelectedDetails();
    void importCertificate();
    void applyFilter();

    void apply();

private:
    void setupTrustStore();
    void loadClientCertificate();
    void loadTrustStore();
    void refreshClientCertificateView();
    void updatePendingState();
    void chooseAndStageClientCertificate(const QString &title);
    void stageTargetForSelection(int target);
    QStandardItem *selectedSourceItem() const;
    void addTrustRow(const QByteArray &der, int origin, int target);
    bool hasPendingChanges() const;
    QByteArray effectiveClientCertificate() const;
    QString effectiveClientKeyPath() const;

    Ui::CertificatesDialog *ui;
    PkiManager _pki;
    QStandardItemModel *_model = nullptr;
    class TrustFilterProxyModel *_proxy = nullptr;

    QString _clientCertificatePath;
    QString _clientKeyPath;
    QByteArray _clientCertificate;

    int _clientOp = 0;
    QString _pendingCertificateSource;
    QString _pendingKeySource;
    QByteArray _pendingCertificate;
};
