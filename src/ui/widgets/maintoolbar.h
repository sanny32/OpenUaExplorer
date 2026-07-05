// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file maintoolbar.h
/// \brief Declares the main application toolbar.
///

#pragma once

#include <QList>
#include <QToolBar>

class QAction;
class QEvent;
class MainToolButton;
class ThemedToolButton;

///
/// \brief Main application toolbar with themed actions and a quick-connect button.
///
class MainToolBar : public QToolBar
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the toolbar.
    /// \param parent Parent widget.
    ///
    explicit MainToolBar(QWidget *parent = nullptr);

    ///
    /// \brief Rebuilds the toolbar from its Designer actions.
    ///
    /// Every action becomes a button in declaration order, except the one named
    /// "actionFavorites", which is right-aligned after an expanding spacer.
    ///
    void setupFromDesignerActions();

    ///
    /// \brief Gives access to the favourites button.
    /// \return The favourites button.
    ///
    ThemedToolButton *favoritesButton() const;

protected:
    ///
    /// \brief Re-equalises button widths when the language, font, or style changes.
    /// \param event Change event being handled.
    ///
    void changeEvent(QEvent *event) override;

private:
    MainToolButton *addMainButton(QAction *action);

    ///
    /// \brief Sizes every tracked button to the widest button's content width.
    ///
    void equalizeButtonWidths();

    ThemedToolButton *_favoritesButton = nullptr;
    QList<ThemedToolButton *> _equalWidthButtons;
};
