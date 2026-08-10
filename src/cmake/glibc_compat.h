#ifndef OUAEXP_GLIBC_COMPAT_H
#define OUAEXP_GLIBC_COMPAT_H

// Forced into every translation unit by glibc_compat.cmake on Linux.
//
// glibc 2.29 gave the double math functions a new symbol version, so code built
// against a newer glibc references pow@GLIBC_2.29 and stops loading where 2.28 is
// the newest available. The build container is not the oldest distribution the
// packages support, so the choice cannot be left to the linker: the directives
// below name the baseline entry points, which are the same implementations under
// an older version tag and are present everywhere.
//
// Only the functions glibc has actually revisioned are listed. Adding a symbol
// here is only ever needed when a package fails its minimum glibc check.

#if defined(__linux__) && defined(__GLIBC__)

#if defined(__x86_64__)
#define OUAEXP_GLIBC_BASELINE "GLIBC_2.2.5"
#elif defined(__aarch64__)
#define OUAEXP_GLIBC_BASELINE "GLIBC_2.17"
#endif

#ifdef OUAEXP_GLIBC_BASELINE

#define OUAEXP_PIN_GLIBC_SYMBOL(symbol) \
    __asm__(".symver " symbol "," symbol "@" OUAEXP_GLIBC_BASELINE)

// Raised to GLIBC_2.29.
OUAEXP_PIN_GLIBC_SYMBOL("exp");
OUAEXP_PIN_GLIBC_SYMBOL("exp2");
OUAEXP_PIN_GLIBC_SYMBOL("log");
OUAEXP_PIN_GLIBC_SYMBOL("log2");
OUAEXP_PIN_GLIBC_SYMBOL("pow");

// Raised to GLIBC_2.27.
OUAEXP_PIN_GLIBC_SYMBOL("expf");
OUAEXP_PIN_GLIBC_SYMBOL("exp2f");
OUAEXP_PIN_GLIBC_SYMBOL("logf");
OUAEXP_PIN_GLIBC_SYMBOL("log2f");
OUAEXP_PIN_GLIBC_SYMBOL("powf");

#endif // OUAEXP_GLIBC_BASELINE

#endif // __linux__ && __GLIBC__

#endif // OUAEXP_GLIBC_COMPAT_H
