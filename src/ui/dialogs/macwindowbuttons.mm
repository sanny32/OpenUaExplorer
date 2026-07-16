// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file macwindowbuttons.mm
/// \brief Implements native control over the macOS title-bar buttons.
///

#import <AppKit/AppKit.h>

#include <QWidget>

#include "macwindowbuttons.h"

///
/// \brief Hides the minimize and zoom title-bar buttons, keeping only close.
/// \param window Widget whose native window is adjusted.
///
void MacWindowButtons::hideMinimizeAndZoomButtons(QWidget *window)
{
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    [native standardWindowButton:NSWindowMiniaturizeButton].hidden = YES;
    [native standardWindowButton:NSWindowZoomButton].hidden = YES;
}
