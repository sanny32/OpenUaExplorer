// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file suffixtranslator.h
/// \brief Provides a stand-in translator for the language-switch tests.
///

#pragma once

#include <QByteArrayList>
#include <QCoreApplication>
#include <QTranslator>

namespace TestTranslation {

///
/// \brief Translator that suffixes source strings with "-xx", standing in for a language.
///
class SuffixTranslator : public QTranslator
{
public:
    ///
    /// \brief Builds a translator for the given contexts.
    /// \param contexts Translation contexts to suffix; empty suffixes every context.
    ///
    explicit SuffixTranslator(QByteArrayList contexts = {})
        : _contexts(std::move(contexts))
    {
    }

    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override
    {
        Q_UNUSED(disambiguation)
        Q_UNUSED(n)
        if (!_contexts.isEmpty() && !_contexts.contains(QByteArray(context)))
            return QString();
        return QString::fromUtf8(sourceText) + QStringLiteral("-xx");
    }

    bool isEmpty() const override { return false; }

private:
    QByteArrayList _contexts;
};

///
/// \brief Installs or removes a translator and delivers the language change.
/// \param translator Translator to switch on or off.
/// \param install True to install, false to remove.
///
inline void switchLanguage(QTranslator *translator, bool install)
{
    if (install)
        QCoreApplication::installTranslator(translator);
    else
        QCoreApplication::removeTranslator(translator);
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();
}

} // namespace TestTranslation
