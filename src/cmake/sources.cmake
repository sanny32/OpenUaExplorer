set(CORE_SOURCES
    loggingcategories.cpp

    opcua/attributeformatter.cpp
    opcua/certificateinfo.cpp
    opcua/connectioncontroller.cpp
    opcua/connectionprofilestore.cpp
    opcua/connectionprofilevalidator.cpp
    opcua/endpointhistorystore.cpp
    opcua/opcuaclientservice.cpp
    opcua/pkimanager.cpp
    opcua/qtopcuabackend.cpp
    opcua/secretstore.cpp

    widgets/addressspacemodel.cpp
    widgets/attributesmodel.cpp
    widgets/dataaccessmodel.cpp
    widgets/endpointmodel.cpp
    widgets/eventsmodel.cpp
    widgets/historymodel.cpp
    widgets/logmodel.cpp
    widgets/nodeinfomodel.cpp
    widgets/referencesmodel.cpp
    widgets/subscriptionsmodel.cpp
)

set(CORE_HEADERS
    loggingcategories.h

    opcua/attributeformatter.h
    opcua/certificateinfo.h
    opcua/certificatetrustdecider.h
    opcua/connectioncontroller.h
    opcua/connectionprofile.h
    opcua/connectionprofilestore.h
    opcua/connectionprofilevalidator.h
    opcua/endpointhistorystore.h
    opcua/opcuabackend.h
    opcua/opcuaclientservice.h
    opcua/opcuatypes.h
    opcua/pkimanager.h
    opcua/qtopcuabackend.h
    opcua/secretstore.h

    widgets/addressspaceitem.h
    widgets/addressspacemodel.h
    widgets/attributesmodel.h
    widgets/columnalignmentstore.h
    widgets/dataaccessitem.h
    widgets/dataaccessmodel.h
    widgets/endpointmodel.h
    widgets/eventsmodel.h
    widgets/historymodel.h
    widgets/logitem.h
    widgets/logmodel.h
    widgets/nodeinfomodel.h
    widgets/nodeitem.h
    widgets/referencesmodel.h
    widgets/subscriptionitem.h
    widgets/subscriptionsmodel.h
)

set(UI_SOURCES
    appstyle.cpp
    apptheme.cpp
    application.cpp
    fusionstyle.cpp
    macthemefactory.cpp
    qlementineappstyle.cpp
    qlementinethemefactory.cpp
    macappstyle.cpp

    dialogs/appbasedialog.cpp
    dialogs/connectiondialog.cpp
    dialogs/dialogabout.cpp
    dialogs/certificatetrustdialog.cpp
    dialogs/writevaluedialog.cpp

    mainwindow.cpp

    widgets/addressspacewidget.cpp
    widgets/attributeswidget.cpp
    widgets/coloredpushbutton.cpp
    widgets/connectionstatuswidget.cpp
    widgets/dataaccesswidget.cpp
    widgets/endpointdiscoverywidget.cpp
    widgets/endpointselectorwidget.cpp
    widgets/fixedgap.cpp
    widgets/headerview.cpp
    widgets/tableview.cpp
    widgets/logwidget.cpp
    widgets/mainstatusbarwidget.cpp
    widgets/maintoolbar.cpp
    widgets/maintoolbutton.cpp
    widgets/subscriptiondelegate.cpp
    widgets/themediconlabel.cpp
    widgets/themedpushbutton.cpp
    widgets/themedtoolbutton.cpp
    widgets/securityselectorwidget.cpp
    widgets/trendpanelwidget.cpp
    widgets/trendgraphwidget.cpp
)

set(UI_HEADERS
    appicons.h
    appstyle.h
    apptheme.h
    application.h
    fusionstyle.h
    macthemefactory.h
    qlementineappstyle.h
    qlementinethemefactory.h
    macappstyle.h

    dialogs/appbasedialog.h
    dialogs/connectiondialog.h
    dialogs/dialogabout.h
    dialogs/certificatetrustdialog.h
    dialogs/writevaluedialog.h

    mainwindow.h

    widgets/addressspacewidget.h
    widgets/attributeswidget.h
    widgets/coloredpushbutton.h
    widgets/connectionstatuswidget.h
    widgets/dataaccesswidget.h
    widgets/endpointdiscoverywidget.h
    widgets/endpointselectorwidget.h
    widgets/fixedgap.h
    widgets/headerview.h
    widgets/tableview.h
    widgets/logwidget.h
    widgets/mainstatusbarwidget.h
    widgets/maintoolbar.h
    widgets/maintoolbutton.h
    widgets/securityselectorwidget.h
    widgets/subscriptiondelegate.h
    widgets/themediconlabel.h
    widgets/themedpushbutton.h
    widgets/themedtoolbutton.h
    widgets/trendpanelwidget.h
    widgets/trendgraphwidget.h
)

set(UI_FILES
    mainwindow.ui

    dialogs/connectiondialog.ui
    dialogs/dialogabout.ui
    dialogs/certificatetrustdialog.ui
    dialogs/writevaluedialog.ui

    widgets/addressspacewidget.ui
    widgets/attributeswidget.ui
    widgets/connectionstatuswidget.ui
    widgets/dataaccesswidget.ui
    widgets/endpointselectorwidget.ui
    widgets/logwidget.ui
    widgets/mainstatusbarwidget.ui
    widgets/securityselectorwidget.ui
    widgets/trendpanelwidget.ui
)

set(RESOURCE_FILES
    resources.qrc
)
