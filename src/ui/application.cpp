// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file application.cpp
/// \brief Implements the application class.
///

#include "appstyle.h"
#include "application.h"
#include "loggingcategories.h"
#include "opcua/pkimanager.h"

#if defined(HAVE_QLEMENTINE_APP_STYLE) && defined(Q_OS_MAC)
#include "macappstyle.h"
#elif defined(HAVE_QLEMENTINE_APP_STYLE)
#include "qlementineappstyle.h"
#endif

///
/// \brief Constructs the application, installs the platform style, and applies the initial theme.
/// \param argc Argument count passed to main.
/// \param argv Argument vector passed to main.
///
Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , _theme(this)
{
#if defined(HAVE_QLEMENTINE_APP_STYLE) && defined(Q_OS_MAC)
    setStyle(new MacAppStyle());
#elif defined(HAVE_QLEMENTINE_APP_STYLE)
    setStyle(new QlementineAppStyle());
#else
    setStyle(new AppStyle());
#endif
    _theme.applyInitialScheme();
    ensureClientCertificate();
}

///
/// \brief Gives access to the application theme manager.
/// \return Reference to the application theme manager.
///
AppTheme& Application::theme()
{
    return _theme;
}

///
/// \brief Returns the running application downcast to this type.
/// \return Pointer to the global Application instance.
///
Application *Application::instance()
{
    return static_cast<Application *>(QApplication::instance());
}

///
/// \brief Generates the application client certificate when the auto-generated pair is absent.
///
void Application::ensureClientCertificate()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    if (pki.existingClientCertificate(&certificateFile, &privateKeyFile))
        return;

    QString error;
    if (!pki.generateClientCertificate(
            PkiManager::clientCertificateCommonName(),
            PkiManager::applicationUri(),
            &certificateFile,
            &privateKeyFile,
            &error)) {
        qCWarning(lcApp).noquote()
            << tr("Could not generate the client certificate: %1").arg(error);
    }
}
