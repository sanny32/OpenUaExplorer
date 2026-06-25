// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogabout.h
/// \brief Declares the application about dialog.
///

#pragma once

#include <QJsonObject>
#include <QSize>
#include <QString>
#include <QUrl>

#include "dialogs/appbasedialog.h"

class QFrame;
class QWidget;

namespace Ui {
class DialogAbout;
}

///
/// \brief Dialog that presents application identity, links and project information.
///
class DialogAbout : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the About dialog and fills its content.
    /// \param parent Parent widget.
    ///
    explicit DialogAbout(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~DialogAbout() override;

private:
    void setupContent();
    void setupFonts();
    void setupLayout();
    void setupAuthors();
    void setupComponents();
    QJsonObject aboutData() const;
    QString authorRoleDescription(const QString &sectionName, const QString &role) const;
    void addComponent(const QString &title,
                      const QString &version,
                      const QString &description,
                      const QUrl &url,
                      int *layoutIndex,
                      int *componentIndex);
    QWidget *createAuthorRow(const QString &name, const QString &role, const QUrl &url, int index);
    QWidget *createAuthorContactButton(const QUrl &url, int index);
    QFrame *createAuthorSeparator(const QString &objectName);
    QWidget *createComponentRow(const QString &title,
                                const QString &version,
                                const QString &description,
                                const QUrl &url,
                                int index);
    QWidget *createComponentContactButton(const QUrl &url, int index);
    QFrame *createComponentSeparator(const QString &objectName);
    QString iconButtonStyleSheet() const;
    QString licenseHtml() const;
    QString licenseStyleSheet() const;
    void openAuthorUrl();
    void openComponentUrl();

private:
    Ui::DialogAbout *ui;
};
