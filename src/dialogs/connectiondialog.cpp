#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>

#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "widgets/coloredpushbutton.h"

///
/// \brief isDarkTheme
/// \return
///
static bool isDarkTheme()
{
    return qApp->palette().color(QPalette::Window).lightness() < 128;
}

///
/// \brief svgToPixmap
/// \param resource
/// \param size
/// \return
///
static QPixmap svgToPixmap(const QString &resource, int size)
{
    QSvgRenderer renderer(resource);
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    return pixmap;
}

///
/// \brief themedIcon
/// \param name
/// \param size
/// \return
///
static QPixmap themedIcon(const QString &name, int size)
{
    const QString theme = isDarkTheme() ? "dark" : "light";
    return svgToPixmap(QString(":/icons/%1/%2.svg").arg(theme, name), size);
}

///
/// \brief ConnectionDialog::ConnectionDialog
/// \param parent
///
ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);

    updateTheme();

    for (QSpinBox *sb : {
             ui->sessionTimeoutSpinBox,
             ui->endpointTimeoutSpinBox,
             ui->connectTimeoutSpinBox,
             ui->requestTimeoutSpinBox,
             ui->secureChannelLifetimeSpinBox,
             ui->maxMessageSizeSpinBox
         }) {
        sb->setFixedWidth(100);
    }

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->connectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->showPasswordCheckBox, &QCheckBox::toggled, ui->passwordEdit, [this](bool checked) {
        ui->passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
}

///
/// \brief ConnectionDialog::~ConnectionDialog
///
ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

void ConnectionDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange)
        updateTheme();
    QDialog::changeEvent(event);
}

///
/// \brief ConnectionDialog::updateTheme
///
void ConnectionDialog::updateTheme()
{
    if (isDarkTheme()) {
        ui->connectButton->setColors({
            QColor(0x1a8fe8),
            QColor(0x2b9df5),
            QColor(0x0d72c4),
        });
    } else {
        ui->connectButton->setColors({
            QColor(0x0a74d1),
            QColor(0x1682df),
            QColor(0x075ca7),
        });
    }

    ui->clientCertificateHintIcon->setPixmap(themedIcon("info", 24));
    ui->serverCertificateIconLabel->setPixmap(themedIcon("lock", 48));
    ui->statusIconLabel->setPixmap(themedIcon("disconnected", 16));
}
