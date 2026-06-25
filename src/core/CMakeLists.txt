add_library(ouaexp_core STATIC
    appsettings.cpp
    appsettings.h
    loggingcategories.cpp
    loggingcategories.h
    utils.cpp
    utils.h

    opcua/attributeformatter.cpp
    opcua/attributeformatter.h
    opcua/certificateinfo.cpp
    opcua/certificateinfo.h
    opcua/certificatetrustdecider.h
    opcua/connectioncontroller.cpp
    opcua/connectioncontroller.h
    opcua/connectionprofile.h
    opcua/connectionprofilestore.cpp
    opcua/connectionprofilestore.h
    opcua/connectionprofilevalidator.cpp
    opcua/connectionprofilevalidator.h
    opcua/endpointhistorystore.cpp
    opcua/endpointhistorystore.h
    opcua/opcuabackend.h
    opcua/opcuaclientservice.cpp
    opcua/opcuaclientservice.h
    opcua/opcuatypes.h
    opcua/pkimanager.cpp
    opcua/pkimanager.h
    opcua/recentconnectionstore.cpp
    opcua/recentconnectionstore.h
    opcua/qtopcuabackend.cpp
    opcua/qtopcuabackend.h
    opcua/qtopcuaconnectionmanager.cpp
    opcua/qtopcuaconnectionmanager.h
    opcua/qtopcuarequestcoordinator.cpp
    opcua/qtopcuarequestcoordinator.h
    opcua/qtopcuatypemapper.cpp
    opcua/qtopcuatypemapper.h
    opcua/secretstore.cpp
    opcua/secretstore.h
    opcua/standardnodeid.h

    models/addressspaceitem.h
    models/addressspacemimedata.cpp
    models/addressspacemimedata.h
    models/addressspacemodel.cpp
    models/addressspacemodel.h
    models/attributesmodel.cpp
    models/attributesmodel.h
    models/columnalignmentstore.h
    models/dataaccessitem.h
    models/dataaccessmodel.cpp
    models/dataaccessmodel.h
    models/endpointmodel.cpp
    models/endpointmodel.h
    models/eventsmodel.cpp
    models/eventsmodel.h
    models/historymodel.cpp
    models/historymodel.h
    models/logitem.h
    models/logmodel.cpp
    models/logmodel.h
    models/nodeinfomodel.cpp
    models/nodeinfomodel.h
    models/nodeitem.h
    models/referencesmodel.cpp
    models/referencesmodel.h
    models/subscriptionitem.h
    models/subscriptionsmodel.cpp
    models/subscriptionsmodel.h
)

target_include_directories(ouaexp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ouaexp_core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::OpcUa
)

if(QT_VERSION_MAJOR EQUAL 5 AND QTOPCUA_OPEN62541_HISTORY_READY)
    add_library(ouaexp_open62541_history STATIC
        "${OUAEXP_QTOPCUA_SOURCE_DIR}/src/3rdparty/open62541/open62541.c"
    )
    target_include_directories(ouaexp_open62541_history PUBLIC
        "${OUAEXP_QTOPCUA_SOURCE_DIR}/src/3rdparty/open62541"
    )
    target_compile_definitions(ouaexp_open62541_history PUBLIC
        UA_ENABLE_HISTORIZING
    )
    if(QTOPCUA_OPEN62541_ENCRYPTION_AVAILABLE)
        target_compile_definitions(ouaexp_open62541_history PUBLIC
            UA_ENABLE_ENCRYPTION
        )
        target_include_directories(ouaexp_open62541_history PRIVATE
            "${QTOPCUA_MBEDTLS_INCLUDE_DIR}"
        )
        target_link_libraries(ouaexp_open62541_history PUBLIC
            ${QTOPCUA_MBEDTLS_LIBRARIES}
        )
    endif()
    if(WIN32)
        target_link_libraries(ouaexp_open62541_history PUBLIC ws2_32 iphlpapi)
    endif()
    target_sources(ouaexp_core PRIVATE
        opcua/uahistorybackend.cpp
        opcua/uahistorybackend.h
    )
    target_link_libraries(ouaexp_core PRIVATE
        ouaexp_open62541_history
    )
    target_compile_definitions(ouaexp_core PUBLIC
        OUAEXP_HAS_OPEN62541_HISTORY
    )
endif()

ouaexp_configure_dependencies(ouaexp_core)
ouaexp_apply_coverage(ouaexp_core)

if(MSVC)
    target_compile_options(ouaexp_core PUBLIC /utf-8)
endif()
