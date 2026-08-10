# Third-Party Software Notices

OpenUaExplorer source code is licensed under the MIT License in `LICENSE`.
Release packages also contain the third-party components below. Each component
remains subject to its own license.

In an installed release, the referenced license texts are under `third-party/`
beside this file. In the source repository, the same files are under
`licenses/`.

## Qt 6

Components: Qt Core, Qt GUI, Qt Widgets, Qt Network, Qt SVG, Qt Charts, Qt OPC UA,
platform and image-format plugins, and Qt translations as selected by the target
platform.

Copyright (C) The Qt Company Ltd. and other contributors.

Source: https://code.qt.io/cgit/qt/

The Qt libraries other than Qt Charts are available under commercial terms or,
for this distribution, the GNU Lesser General Public License version 3. Qt
Charts is available under commercial terms or the GNU General Public License
version 3. The complete GPLv3 and LGPLv3 texts are provided in
`third-party/GPL-3.0-only.txt` and `third-party/LGPL-3.0-only.txt`.

The packages use shared Qt libraries so recipients can replace them with
interface-compatible builds. SPDX SBOM documents supplied by Qt are installed
under `sbom/` when the Qt distribution provides them. They enumerate the
third-party code embedded in the shipped Qt binaries. The corresponding Qt 6.9
third-party notices are also published at:
https://doc.qt.io/qt-6.9/licenses-used-in-qt.html

## Qt OPC UA and open62541

Version: Qt OPC UA follows the bundled Qt version; open62541 1.4.9.

Qt OPC UA is built from https://code.qt.io/cgit/qt/qtopcua.git/ and includes
the repository patches under `src/cmake/patches/`. Its Qt-authored code is
available under commercial terms or LGPL-3.0-only, GPL-2.0-only, or
GPL-3.0-only.

The open62541 project is primarily licensed under MPL-2.0. The amalgamated
`open62541.c` and `open62541.h` files bundled by Qt also incorporate third-party
material under:

- MPL-2.0
- CC0-1.0
- CC-BY-SA-4.0
- BSD-3-Clause
- Apache-2.0
- MIT

Qt 6.9.3 records the combined license expression for that amalgamation as
`MPL-2.0 AND CC0-1.0 AND CC-BY-SA-4.0 AND BSD-3-Clause AND Apache-2.0 AND MIT`.
The corresponding texts and author list are provided under `third-party/`.
Source: https://github.com/open62541/open62541/tree/v1.4.9

## OpenSSL

Version: OpenSSL 3.x.

Copyright The OpenSSL Project Authors.

License: Apache-2.0. The license text is provided in
`third-party/Apache-2.0.txt`; acknowledgements are provided in
`third-party/OpenSSL-ACKNOWLEDGEMENTS.md`.

Source: https://github.com/openssl/openssl

## QtKeychain

Version: 0.16.0.

Copyright (C) Frank Osterfeld and other QtKeychain contributors.

License: BSD-3-Clause. The exact redistribution terms are provided in
`third-party/BSD-3-Clause-QtKeychain.txt`.

Source: https://github.com/frankosterfeld/qtkeychain/tree/0.16.0

## Qlementine

Version: 1.4.2. Included only in builds that enable the Qlementine application
style, currently macOS release builds.

Copyright (c) 2022 Olivier Cléro.

License: MIT. The license text is provided in
`third-party/MIT-Qlementine.txt`.

Qlementine 1.4.2 statically embeds Inter/Inter Display 4.001 under the SIL Open
Font License 1.1 and Roboto Mono 3.000 under Apache-2.0. Their copyright,
license, reserved-name, and trademark notices are provided in
`third-party/Qlementine-FONTS.md`; the OFL text is provided in
`third-party/OFL-1.1.txt`.

Source: https://github.com/oclero/qlementine/tree/v1.4.2

## Lucide

The application artwork includes icons derived from Lucide. Lucide is
available under the ISC License; icons inherited from the Feather project are
available under the MIT License.

Copyright (c) Lucide Icons and Contributors.
Copyright (c) 2013-present Cole Bemis.

The combined notice is provided in `third-party/Lucide-ISC-MIT.txt`.

Source: https://github.com/lucide-icons/lucide
