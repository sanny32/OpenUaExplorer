// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themepreviewbutton.h
/// \brief Declares a theme-selection card with an application preview.
///

#pragma once

#include <QAbstractButton>

///
/// \brief Checkable theme card that previews light, dark, or system appearance.
///
class ThemePreviewButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(QString previewMode READ previewMode WRITE setPreviewMode)

public:
    ///
    /// \brief Constructs a theme preview card.
    /// \param parent Parent widget.
    ///
    explicit ThemePreviewButton(QWidget *parent = nullptr);

    ///
    /// \brief Returns the preview mode name.
    /// \return Light, dark, or system.
    ///
    QString previewMode() const;

    ///
    /// \brief Selects which colour scheme the card previews.
    /// \param mode Light, dark, or system.
    ///
    void setPreviewMode(const QString &mode);

    ///
    /// \brief Returns the card's preferred size.
    /// \return Size suitable for the preview and caption.
    ///
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString _previewMode = QStringLiteral("system");
};
