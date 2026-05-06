// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodeitem.h
/// \brief Declares node information and reference data types.
///

#pragma once

#include <QPair>
#include <QString>
#include <QVector>

///
/// \brief Describes one name/value row of selected node information.
///
struct NodeInfoItem
{
    QString label;
    QString value;
};

///
/// \brief Describes one reference from the selected OPC UA node.
///
struct ReferenceItem
{
    QString reference;
    QString target;
};
