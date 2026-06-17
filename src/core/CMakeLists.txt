add_library(ouaexp_core STATIC
    loggingcategories.cpp
    loggingcategories.h

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
    opcua/qtopcuabackend.cpp
    opcua/qtopcuabackend.h
    opcua/secretstore.cpp
    opcua/secretstore.h

    widgets/addressspaceitem.h
    widgets/addressspacemodel.cpp
    widgets/addressspacemodel.h
    widgets/attributesmodel.cpp
    widgets/attributesmodel.h
    widgets/columnalignmentstore.h
    widgets/dataaccessitem.h
    widgets/dataaccessmodel.cpp
    widgets/dataaccessmodel.h
    widgets/endpointmodel.cpp
    widgets/endpointmodel.h
    widgets/eventsmodel.cpp
    widgets/eventsmodel.h
    widgets/historymodel.cpp
    widgets/historymodel.h
    widgets/logitem.h
    widgets/logmodel.cpp
    widgets/logmodel.h
    widgets/nodeinfomodel.cpp
    widgets/nodeinfomodel.h
    widgets/nodeitem.h
    widgets/referencesmodel.cpp
    widgets/referencesmodel.h
    widgets/subscriptionitem.h
    widgets/subscriptionsmodel.cpp
    widgets/subscriptionsmodel.h
)

target_include_directories(ouaexp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(ouaexp_core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::OpcUa
)

ouaexp_configure_dependencies(ouaexp_core)
ouaexp_apply_coverage(ouaexp_core)

if(MSVC)
    target_compile_options(ouaexp_core PUBLIC /utf-8)
endif()
