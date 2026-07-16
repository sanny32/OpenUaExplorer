// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file macwindowbuttons.mm
/// \brief Implements native control over the macOS title-bar buttons.
///

#import <AppKit/AppKit.h>

#include <QGuiApplication>
#include <QWidget>

#include "macwindowbuttons.h"

///
/// \brief Hides the minimize and zoom title-bar buttons, keeping only close.
///
/// Only the cocoa platform backs winId() with an NSView; under other QPA plugins
/// (offscreen, minimal — used by the tests) the call is a no-op.
///
/// \param window Widget whose native window is adjusted.
///
void MacWindowButtons::hideMinimizeAndZoomButtons(QWidget *window)
{
    if (QGuiApplication::platformName() != QLatin1String("cocoa"))
        return;

    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    [native standardWindowButton:NSWindowMiniaturizeButton].hidden = YES;
    [native standardWindowButton:NSWindowZoomButton].hidden = YES;
}
