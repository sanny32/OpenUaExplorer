// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatesdialog.cpp
/// \brief Implements the PKI certificate management dialog.
///

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSslCertificate>
#include <QStandardItem>
#include <QStandardItemModel>

#include "appcolors.h"
#include "appicons.h"
#include "certificatedetailsdialog.h"
#include "certificatesdialog.h"
#include "formatters/datetimeformatter.h"
#include "opcua/certificateinfo.h"
#include "textviewdialog.h"
#include "ui_certificatesdialog.h"

namespace {

constexpr int OriginTrusted = 0;
constexpr int OriginRejected = 1;
constexpr int OriginNew = 2;

constexpr int TargetTrusted = 0;
constexpr int TargetRejected = 1;
constexpr int TargetRemoved = 2;

constexpr int ClientNone = 0;
constexpr int ClientImportReplace = 1;
constexpr int ClientRemove = 2;

constexpr int DerRole = Qt::UserRole;
constexpr int OriginRole = Qt::UserRole + 1;
constexpr int TargetRole = Qt::UserRole + 2;

/// \brief Reads a DER or PEM certificate file and returns it in DER encoding.
QByteArray readCertificateAsDer(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray raw = file.readAll();
    QList<QSslCertificate> chain = QSslCertificate::fromData(raw, QSsl::Der);
    if (chain.isEmpty())
        chain = QSslCertificate::fromData(raw, QSsl::Pem);
    return chain.isEmpty() ? QByteArray() : chain.constFirst().toDer();
}

/// \brief Applies the Trusted/Rejected text, icon, and colour to a status cell.
void styleStatusItem(QStandardItem *item, int target)
{
    const bool trusted = target == TargetTrusted;
    item->setText(trusted ? QObject::tr("Trusted") : QObject::tr("Rejected"));
    item->setIcon(AppIcons::themed(trusted ? QStringLiteral("shield-trusted")
                                           : QStringLiteral("shield-warning")));
    item->setForeground(trusted ? AppColors::statusSuccess() : AppColors::statusError());
}

} // namespace

///
/// \brief Filters the trust-store table by category and a free-text search.
///
class TrustFilterProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    /// \brief Restricts rows to a category, or -1 for all categories.
    void setCategory(int category)
    {
        _category = category;
        invalidateFilter();
    }

    /// \brief Restricts rows to those whose name or issuer contain the text.
    void setSearch(const QString &text)
    {
        _search = text.trimmed();
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        const QModelIndex nameIndex = sourceModel()->index(row, 0, parent);
        const int target = nameIndex.data(TargetRole).toInt();
        if (target == TargetRemoved)
            return false;
        if (_category == TargetTrusted && target != TargetTrusted)
            return false;
        if (_category == TargetRejected && target != TargetRejected)
            return false;
        if (!_search.isEmpty()) {
            const QString name = nameIndex.data(Qt::DisplayRole).toString();
            const QString issuer = sourceModel()->index(row, 1, parent).data(Qt::DisplayRole).toString();
            if (!name.contains(_search, Qt::CaseInsensitive)
                && !issuer.contains(_search, Qt::CaseInsensitive)) {
                return false;
            }
        }
        return true;
    }

private:
    int _category = -1;
    QString _search;
};

///
/// \brief Builds the dialog, loads the client certificate and trust store.
/// \param parent Parent widget.
///
CertificatesDialog::CertificatesDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::CertificatesDialog)
{
    ui->setupUi(this);

    const QString captionStyle = QStringLiteral("color: %1;").arg(AppColors::fieldLabel().name());
    const QList<QLabel *> captions = {
        ui->certificateFileCaption, ui->privateKeyCaption, ui->validUntilCaption,
        ui->issuerCaption, ui->thumbprintCaption, ui->statusCaption
    };
    for (QLabel *caption : captions)
        caption->setStyleSheet(captionStyle);

    const int gridRowHeight = ui->viewCertificateButton->sizeHint().height();
    for (int row = 0; row < ui->clientCertificateGrid->rowCount(); ++row)
        ui->clientCertificateGrid->setRowMinimumHeight(row, gridRowHeight);

    setupTrustStore();

    connect(ui->viewCertificateButton, &QAbstractButton::clicked,
            this, &CertificatesDialog::viewClientCertificate);
    connect(ui->viewKeyButton, &QAbstractButton::clicked,
            this, &CertificatesDialog::viewPrivateKey);
    connect(ui->importClientButton, &QPushButton::clicked,
            this, &CertificatesDialog::importClientCertificate);
    connect(ui->replaceClientButton, &QPushButton::clicked,
            this, &CertificatesDialog::replaceClientCertificate);
    connect(ui->exportClientButton, &QPushButton::clicked,
            this, &CertificatesDialog::exportClientCertificate);
    connect(ui->removeClientButton, &QPushButton::clicked,
            this, &CertificatesDialog::removeClientCertificate);

    connect(ui->trustButton, &QPushButton::clicked, this, &CertificatesDialog::trustSelected);
    connect(ui->rejectButton, &QPushButton::clicked, this, &CertificatesDialog::rejectSelected);
    connect(ui->removeCertificateButton, &QPushButton::clicked,
            this, &CertificatesDialog::removeSelected);
    connect(ui->detailsButton, &QPushButton::clicked,
            this, &CertificatesDialog::showSelectedDetails);
    connect(ui->importCertificateButton, &QPushButton::clicked,
            this, &CertificatesDialog::importCertificate);

    connect(ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &CertificatesDialog::apply);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    loadClientCertificate();
    loadTrustStore();
    updateTrustSelectionActions();
}

///
/// \brief Destroys the dialog and its generated UI.
///
CertificatesDialog::~CertificatesDialog()
{
    delete ui;
}

///
/// \brief Creates the trust-store model, proxy, table view, and filter controls.
///
void CertificatesDialog::setupTrustStore()
{
    _model = new QStandardItemModel(0, 4, this);
    _model->setHorizontalHeaderLabels(
        { tr("Certificate Name"), tr("Issuer"), tr("Expiry Date"), tr("Status") });

    _proxy = new TrustFilterProxyModel(this);
    _proxy->setSourceModel(_model);
    ui->tableView->setModel(_proxy);

    ui->filterComboBox->addItem(tr("All"), -1);
    ui->filterComboBox->addItem(tr("Trusted"), TargetTrusted);
    ui->filterComboBox->addItem(tr("Rejected"), TargetRejected);

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CertificatesDialog::updateTrustSelectionActions);
    connect(ui->tableView, &QAbstractItemView::doubleClicked,
            this, &CertificatesDialog::showSelectedDetails);
    connect(ui->filterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CertificatesDialog::applyFilter);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &CertificatesDialog::applyFilter);
}

///
/// \brief Loads the current client certificate and resets any staged client changes.
///
void CertificatesDialog::loadClientCertificate()
{
    _clientOp = ClientNone;
    _pendingCertificateSource.clear();
    _pendingKeySource.clear();
    _pendingCertificate.clear();
    _clientCertificatePath.clear();
    _clientKeyPath.clear();
    _clientCertificate.clear();

    if (_pki.clientCertificatePaths(&_clientCertificatePath, &_clientKeyPath)) {
        QFile file(_clientCertificatePath);
        if (file.open(QIODevice::ReadOnly))
            _clientCertificate = file.readAll();
    }
    refreshClientCertificateView();
}

///
/// \brief Rebuilds the trust-store rows from the on-disk trusted and rejected certificates.
///
void CertificatesDialog::loadTrustStore()
{
    _model->removeRows(0, _model->rowCount());
    for (const QByteArray &der : _pki.certificates(PkiManager::Category::Trusted))
        addTrustRow(der, OriginTrusted, TargetTrusted);
    for (const QByteArray &der : _pki.certificates(PkiManager::Category::Rejected))
        addTrustRow(der, OriginRejected, TargetRejected);
    _proxy->invalidate();
    updatePendingState();
}

///
/// \brief Appends one trust-store row for a certificate.
/// \param der DER-encoded certificate.
/// \param origin On-disk category the certificate came from.
/// \param target Staged category the certificate should end up in.
///
void CertificatesDialog::addTrustRow(const QByteArray &der, int origin, int target)
{
    const CertificateInfo info = CertificateInfo::fromDer(der);
    const QString name = info.readable && !info.subject.isEmpty()
        ? info.subject
        : tr("Unknown certificate");
    QString issuer = info.issuer;
    if (info.selfSigned) {
        issuer = issuer.isEmpty() ? tr("Self-signed")
                                  : tr("%1 (self-signed)").arg(issuer);
    }
    const QString expiry = info.expiryDate.isValid() ? formatDateTime(info.expiryDate)
                                                      : tr("Unknown");

    auto *nameItem = new QStandardItem(name);
    nameItem->setData(der, DerRole);
    nameItem->setData(origin, OriginRole);
    nameItem->setData(target, TargetRole);
    auto *issuerItem = new QStandardItem(issuer);
    auto *expiryItem = new QStandardItem(expiry);
    auto *statusItem = new QStandardItem();
    styleStatusItem(statusItem, target);
    _model->appendRow({ nameItem, issuerItem, expiryItem, statusItem });
}

///
/// \brief Returns the column-0 model item of the selected row, or nullptr.
/// \return Selected source item carrying the row's certificate data.
///
QStandardItem *CertificatesDialog::selectedSourceItem() const
{
    const QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if (selection.isEmpty())
        return nullptr;
    const QModelIndex source = _proxy->mapToSource(selection.constFirst());
    return _model->item(source.row(), 0);
}

///
/// \brief Enables the trust-store actions appropriate to the selected row.
///
void CertificatesDialog::updateTrustSelectionActions()
{
    const QStandardItem *item = selectedSourceItem();
    const bool hasSelection = item != nullptr;
    const int target = hasSelection ? item->data(TargetRole).toInt() : -1;
    ui->trustButton->setEnabled(hasSelection && target == TargetRejected);
    ui->rejectButton->setEnabled(hasSelection && target == TargetTrusted);
    ui->removeCertificateButton->setEnabled(hasSelection);
    ui->detailsButton->setEnabled(hasSelection);
}

///
/// \brief Applies the category filter and search text to the trust-store table.
///
void CertificatesDialog::applyFilter()
{
    _proxy->setCategory(ui->filterComboBox->currentData().toInt());
    _proxy->setSearch(ui->searchEdit->text());
    updateTrustSelectionActions();
}

///
/// \brief Re-targets the selected certificate to a category, or marks it removed.
/// \param target Destination staged category.
///
void CertificatesDialog::stageTargetForSelection(int target)
{
    QStandardItem *item = selectedSourceItem();
    if (!item)
        return;
    const int row = item->row();
    item->setData(target, TargetRole);
    if (target != TargetRemoved)
        styleStatusItem(_model->item(row, 3), target);
    _proxy->invalidate();
    updatePendingState();
    updateTrustSelectionActions();
}

///
/// \brief Marks the selected certificate as trusted.
///
void CertificatesDialog::trustSelected()
{
    stageTargetForSelection(TargetTrusted);
}

///
/// \brief Marks the selected certificate as rejected.
///
void CertificatesDialog::rejectSelected()
{
    stageTargetForSelection(TargetRejected);
}

///
/// \brief Marks the selected certificate for removal on Apply.
///
void CertificatesDialog::removeSelected()
{
    stageTargetForSelection(TargetRemoved);
}

///
/// \brief Opens the read-only details dialog for the selected certificate.
///
void CertificatesDialog::showSelectedDetails()
{
    const QStandardItem *item = selectedSourceItem();
    if (!item)
        return;
    CertificateDetailsDialog dialog(this);
    dialog.setCertificate(item->data(DerRole).toByteArray());
    dialog.exec();
}

///
/// \brief Imports a certificate file into the trust store as trusted.
///
void CertificatesDialog::importCertificate()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Import Certificate"), QString(),
        tr("Certificates (*.der *.pem *.crt);;All Files (*)"));
    if (path.isEmpty())
        return;
    const QByteArray der = readCertificateAsDer(path);
    if (der.isEmpty()) {
        QMessageBox::warning(this, tr("Import Certificate"),
                             tr("The selected file is not a readable certificate."));
        return;
    }

    const QString fingerprint = PkiManager::fingerprint(der);
    for (int row = 0; row < _model->rowCount(); ++row) {
        QStandardItem *item = _model->item(row, 0);
        if (item->data(TargetRole).toInt() != TargetRemoved
            && PkiManager::fingerprint(item->data(DerRole).toByteArray()) == fingerprint) {
            QMessageBox::information(this, tr("Import Certificate"),
                                    tr("This certificate is already in the trust store."));
            return;
        }
    }

    addTrustRow(der, OriginNew, TargetTrusted);
    _proxy->invalidate();
    updatePendingState();
    const int lastRow = _model->rowCount() - 1;
    const QModelIndex proxyIndex = _proxy->mapFromSource(_model->index(lastRow, 0));
    if (proxyIndex.isValid())
        ui->tableView->selectRow(proxyIndex.row());
    updateTrustSelectionActions();
}

///
/// \brief Enables the Apply button when there are staged changes to commit.
///
void CertificatesDialog::updatePendingState()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(hasPendingChanges());
}

///
/// \brief Reports whether any staged change differs from the on-disk state.
/// \return True when Apply has work to do.
///
bool CertificatesDialog::hasPendingChanges() const
{
    if (_clientOp != ClientNone)
        return true;
    for (int row = 0; row < _model->rowCount(); ++row) {
        const QStandardItem *item = _model->item(row, 0);
        const int origin = item->data(OriginRole).toInt();
        const int target = item->data(TargetRole).toInt();
        if (target == TargetRemoved) {
            if (origin != OriginNew)
                return true;
            continue;
        }
        if (origin == OriginNew || origin != target)
            return true;
    }
    return false;
}

///
/// \brief Returns the certificate currently shown for the client identity.
/// \return DER-encoded client certificate, or an empty array.
///
QByteArray CertificatesDialog::effectiveClientCertificate() const
{
    if (_clientOp == ClientRemove)
        return {};
    if (_clientOp == ClientImportReplace)
        return _pendingCertificate;
    return _clientCertificate;
}

///
/// \brief Returns the private-key file currently shown for the client identity.
/// \return Path of the private key, or an empty string.
///
QString CertificatesDialog::effectiveClientKeyPath() const
{
    if (_clientOp == ClientRemove)
        return {};
    if (_clientOp == ClientImportReplace)
        return _pendingKeySource;
    return _clientKeyPath;
}

///
/// \brief Repaints the client-certificate card from the effective (staged) state.
///
void CertificatesDialog::refreshClientCertificateView()
{
    const QByteArray der = effectiveClientCertificate();
    const bool present = !der.isEmpty();
    const QString certPath = _clientOp == ClientImportReplace
        ? _pendingCertificateSource
        : (_clientOp == ClientRemove ? QString() : _clientCertificatePath);
    const QString keyPath = effectiveClientKeyPath();

    ui->viewCertificateButton->setEnabled(present);
    ui->exportClientButton->setEnabled(present);
    ui->replaceClientButton->setEnabled(present);
    ui->removeClientButton->setEnabled(present);
    ui->viewKeyButton->setEnabled(present && !keyPath.isEmpty());
    ui->importClientButton->setEnabled(!present);

    if (!present) {
        ui->certificateFileValue->setText(tr("No client certificate"));
        ui->privateKeyValue->clear();
        ui->validUntilValue->clear();
        ui->issuerValue->clear();
        ui->thumbprintValue->clear();
        ui->statusIcon->setVisible(false);
        ui->statusValue->clear();
        return;
    }

    const CertificateInfo info = CertificateInfo::fromDer(der);
    ui->certificateFileValue->setText(QFileInfo(certPath).fileName());
    ui->privateKeyValue->setText(QFileInfo(keyPath).fileName());
    ui->validUntilValue->setText(info.expiryDate.isValid() ? formatDateTime(info.expiryDate)
                                                            : tr("Unknown"));
    QString issuer = info.issuer;
    if (info.selfSigned)
        issuer = issuer.isEmpty() ? tr("Self-signed") : tr("%1 (self-signed)").arg(issuer);
    ui->issuerValue->setText(issuer);
    ui->thumbprintValue->setText(info.fingerprint);

    QString statusText;
    bool valid = false;
    switch (info.status) {
    case CertificateInfo::Status::Valid:       statusText = tr("Valid"); valid = true; break;
    case CertificateInfo::Status::Expired:     statusText = tr("Expired"); break;
    case CertificateInfo::Status::NotYetValid: statusText = tr("Not yet valid"); break;
    default:                                   statusText = tr("Invalid"); break;
    }
    ui->statusIcon->setIcon(valid ? QStringLiteral("shield-trusted")
                                   : QStringLiteral("shield-warning"), QSize(18, 18));
    ui->statusIcon->setVisible(true);
    ui->statusValue->setText(statusText);
    ui->statusValue->setStyleSheet(QStringLiteral("color: %1; font-weight: 600;")
        .arg((valid ? AppColors::statusSuccess() : AppColors::statusError()).name()));
}

///
/// \brief Opens the read-only details dialog for the client certificate.
///
void CertificatesDialog::viewClientCertificate()
{
    const QByteArray der = effectiveClientCertificate();
    if (der.isEmpty())
        return;
    const QString certPath = _clientOp == ClientImportReplace
        ? _pendingCertificateSource
        : _clientCertificatePath;
    CertificateDetailsDialog dialog(this);
    dialog.setCertificate(der, certPath);
    dialog.exec();
}

///
/// \brief Shows the private key file contents in a read-only, copyable text viewer.
///
void CertificatesDialog::viewPrivateKey()
{
    const QString keyPath = effectiveClientKeyPath();
    if (keyPath.isEmpty())
        return;

    QFile file(keyPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Private Key"),
                              tr("Could not read the private key file."));
        return;
    }

    TextViewDialog::showText(this, tr("Private Key"), QString::fromUtf8(file.readAll()));
}

///
/// \brief Prompts for a certificate and key file and stages them as the client identity.
/// \param title File-dialog caption for the certificate selection.
///
void CertificatesDialog::chooseAndStageClientCertificate(const QString &title)
{
    const QString certificate = QFileDialog::getOpenFileName(
        this, title, QString(), tr("Certificates (*.der *.pem *.crt);;All Files (*)"));
    if (certificate.isEmpty())
        return;
    const QString key = QFileDialog::getOpenFileName(
        this, tr("Select Private Key"), QString(),
        tr("Private Keys (*.pem *.key);;All Files (*)"));
    if (key.isEmpty())
        return;
    const QByteArray der = readCertificateAsDer(certificate);
    if (der.isEmpty()) {
        QMessageBox::warning(this, title,
                             tr("The selected file is not a readable certificate."));
        return;
    }
    _clientOp = ClientImportReplace;
    _pendingCertificateSource = certificate;
    _pendingKeySource = key;
    _pendingCertificate = der;
    refreshClientCertificateView();
    updatePendingState();
}

///
/// \brief Stages a new client certificate when none is present.
///
void CertificatesDialog::importClientCertificate()
{
    chooseAndStageClientCertificate(tr("Import Client Certificate"));
}

///
/// \brief Stages a replacement client certificate.
///
void CertificatesDialog::replaceClientCertificate()
{
    chooseAndStageClientCertificate(tr("Replace Client Certificate"));
}

///
/// \brief Stages removal of the client certificate.
///
void CertificatesDialog::removeClientCertificate()
{
    if (effectiveClientCertificate().isEmpty())
        return;
    _clientOp = ClientRemove;
    _pendingCertificateSource.clear();
    _pendingKeySource.clear();
    _pendingCertificate.clear();
    refreshClientCertificateView();
    updatePendingState();
}

///
/// \brief Exports the currently shown client certificate to a chosen file.
///
void CertificatesDialog::exportClientCertificate()
{
    const QByteArray der = effectiveClientCertificate();
    if (der.isEmpty())
        return;
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Certificate"), QStringLiteral("client_certificate.der"),
        tr("DER Certificate (*.der);;PEM Certificate (*.pem)"));
    if (path.isEmpty())
        return;

    QByteArray output = der;
    if (path.endsWith(QStringLiteral(".pem"), Qt::CaseInsensitive)) {
        const QList<QSslCertificate> chain = QSslCertificate::fromData(der, QSsl::Der);
        if (!chain.isEmpty())
            output = chain.constFirst().toPem();
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(output) != output.size()) {
        QMessageBox::warning(this, tr("Export Certificate"),
                             tr("Could not write the certificate to %1.").arg(path));
    }
}

///
/// \brief Writes every staged change to the PKI store, then reloads the committed state.
///
void CertificatesDialog::apply()
{
    QString error;

    if (_clientOp == ClientRemove) {
        if (!_pki.removeClientCertificate(&error)) {
            QMessageBox::warning(this, tr("Certificates"), error);
            return;
        }
    } else if (_clientOp == ClientImportReplace) {
        if (!_pki.importClientCertificate(_pendingCertificateSource, _pendingKeySource, &error)) {
            QMessageBox::warning(this, tr("Certificates"), error);
            return;
        }
    }

    for (int row = 0; row < _model->rowCount(); ++row) {
        const QStandardItem *item = _model->item(row, 0);
        const QByteArray der = item->data(DerRole).toByteArray();
        const int origin = item->data(OriginRole).toInt();
        const int target = item->data(TargetRole).toInt();
        bool ok = true;
        if (target == TargetRemoved) {
            if (origin != OriginNew)
                ok = _pki.removeCertificate(der, &error);
        } else if (origin == OriginNew || origin != target) {
            const PkiManager::Category category = target == TargetTrusted
                ? PkiManager::Category::Trusted
                : PkiManager::Category::Rejected;
            ok = _pki.setCertificateCategory(der, category, &error);
        }
        if (!ok) {
            QMessageBox::warning(this, tr("Certificates"), error);
            break;
        }
    }

    loadClientCertificate();
    loadTrustStore();
    updateTrustSelectionActions();
}

///
/// \brief Confirms discarding staged changes before closing the dialog.
///
void CertificatesDialog::reject()
{
    if (hasPendingChanges()) {
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this, tr("Discard Changes"),
            tr("Discard the pending certificate changes?"),
            QMessageBox::Discard | QMessageBox::Cancel);
        if (choice != QMessageBox::Discard)
            return;
    }
    AppBaseDialog::reject();
}
