set(OUAEXP_REPOSITORY_ROOT "${PROJECT_SOURCE_DIR}/..")
set(OUAEXP_THIRD_PARTY_LICENSE_DIR "${OUAEXP_REPOSITORY_ROOT}/licenses")

set(OUAEXP_REQUIRED_LICENSE_FILES
    Apache-2.0.txt
    BSD-3-Clause-open62541.txt
    BSD-3-Clause-QtKeychain.txt
    CC-BY-SA-4.0.txt
    CC0-1.0.txt
    GPL-3.0-only.txt
    LGPL-3.0-only.txt
    Lucide-ISC-MIT.txt
    MIT-open62541.txt
    MIT-Qlementine.txt
    MPL-2.0.txt
    open62541-AUTHORS.txt
    OpenSSL-ACKNOWLEDGEMENTS.md
)

foreach(license_file IN LISTS OUAEXP_REQUIRED_LICENSE_FILES)
    if(NOT EXISTS "${OUAEXP_THIRD_PARTY_LICENSE_DIR}/${license_file}")
        message(FATAL_ERROR "Required third-party license is missing: ${license_file}")
    endif()
endforeach()

function(ouaexp_install_license_bundle destination)
    install(FILES
        "${OUAEXP_REPOSITORY_ROOT}/LICENSE"
        "${OUAEXP_REPOSITORY_ROOT}/THIRD_PARTY_NOTICES.md"
        DESTINATION "${destination}"
        COMPONENT licenses)
    install(DIRECTORY "${OUAEXP_THIRD_PARTY_LICENSE_DIR}/"
        DESTINATION "${destination}/third-party"
        COMPONENT licenses)

    set(qt_sbom_dir "${QT_BINARY_DIR}/../sbom")
    foreach(qt_module IN ITEMS qtbase qtcharts qtimageformats qtsvg qttranslations)
        file(GLOB qt_sbom "${qt_sbom_dir}/${qt_module}-*.spdx")
        list(FILTER qt_sbom EXCLUDE REGEX "\\.source\\.spdx$")
        if(qt_sbom)
            install(FILES ${qt_sbom}
                DESTINATION "${destination}/sbom"
                COMPONENT licenses)
        endif()
    endforeach()

    file(GLOB qtopcua_sbom "${QTOPCUA_INSTALL_DIR}/sbom/qtopcua-*.spdx")
    if(qtopcua_sbom)
        install(FILES ${qtopcua_sbom}
            DESTINATION "${destination}/sbom"
            COMPONENT licenses)
    endif()
endfunction()
