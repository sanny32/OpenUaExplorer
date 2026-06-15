// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QStringList>

class EndpointHistoryStore
{
public:
    QStringList history() const;
    void save(const QString &endpointUrl) const;
};
