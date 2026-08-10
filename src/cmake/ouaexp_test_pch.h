#ifndef OUAEXP_TEST_PCH_H
#define OUAEXP_TEST_PCH_H

// Precompiled header shared by every test executable. All test targets compile
// with the same flags and definitions, so CMake builds this header once for the
// ouaexp_test_pch target and the other targets reuse the result through
// target_precompile_headers(... REUSE_FROM ...). Without that reuse the ~70 test
// targets would each build their own copy and lose the whole benefit.
//
// Only add headers that a large share of the tests include, and never a project
// header: any edit to it would invalidate the PCH for every test at once.

#include "ouaexp_widgets_pch.h"

#include <QSignalSpy>
#include <QTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTableView>

#endif // OUAEXP_TEST_PCH_H
