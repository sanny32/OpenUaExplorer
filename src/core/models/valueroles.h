// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file valueroles.h
/// \brief Declares the item roles every model serving OPC UA values shares.
///

#pragma once

#include <Qt>

///
/// \brief Item roles that carry an OPC UA value beyond its display text.
///
/// The data-access and attributes models both hand values to the same delegates and
/// viewers, so the roles are numbered once here instead of per model. They start far
/// enough above Qt::UserRole to leave each model's own roles undisturbed.
///
namespace ValueRoles {

enum Role {
    /// \brief Encoded picture bytes of a cell, empty when the cell shows no picture.
    ImageDataRole = Qt::UserRole + 100
};

} // namespace ValueRoles
