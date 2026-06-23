// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file builtinfeatures.cpp
/// \brief Implements registration for built-in UI features.
///

#include "builtinfeatures.h"

#include "addressspacefeature.h"
#include "attributesfeature.h"
#include "featuremanager.h"
#include "logfeature.h"

///
/// \brief Registers the built-in UI features in initialization order.
/// \param manager Feature manager receiving the features.
///
void registerBuiltinFeatures(FeatureManager &manager)
{
    manager.registerFeature(new LogFeature);
    manager.registerFeature(new AttributesFeature);
    manager.registerFeature(new AddressSpaceFeature);
}
