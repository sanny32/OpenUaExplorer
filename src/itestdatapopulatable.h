// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file itestdatapopulatable.h
/// \brief Declares the interface for widgets that can be filled with test data.
///

#pragma once

///
/// \brief Interface for objects that support population with test data.
///
class ITestDataPopulatable
{
public:
    virtual ~ITestDataPopulatable() = default;
    virtual void populateWithTestData() = 0;
};
