// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file builtinfeatures.h
/// \brief Declares registration for built-in UI features.
///

#pragma once

class FeatureManager;

///
/// \brief Registers the built-in UI features in initialization order.
/// \param manager Feature manager receiving the features.
///
void registerBuiltinFeatures(FeatureManager &manager);
