set(SOURCES
    main.cpp
    appstyle.cpp
    apptheme.cpp
    application.cpp
    fusionstyle.cpp
    loggingcategories.cpp

    opcua/attributeformatter.cpp
    opcua/connectionprofilestore.cpp
    opcua/opcuaclientservice.cpp
    opcua/pkimanager.cpp
    opcua/secretstore.cpp

    dialogs/appbasedialog.cpp
    dialogs/connectiondialog.cpp
    dialogs/dialogabout.cpp
    dialogs/certificatetrustdialog.cpp
    dialogs/writevaluedialog.cpp

    mainwindow.cpp

    widgets/addressspacemodel.cpp
    widgets/addressspacewidget.cpp
    widgets/nodeinfomodel.cpp
    widgets/referencesmodel.cpp
    widgets/attributesmodel.cpp
    widgets/attributeswidget.cpp
    widgets/coloredpushbutton.cpp
    widgets/connectionstatuswidget.cpp
    widgets/dataaccesswidget.cpp
    widgets/dataaccessmodel.cpp
    widgets/eventsmodel.cpp
    widgets/historymodel.cpp
    widgets/subscriptiondelegate.cpp
    widgets/subscriptionsmodel.cpp
    widgets/endpointselectorwidget.cpp
    widgets/fixedgap.cpp
    widgets/headerview.cpp
    widgets/tableview.cpp
    widgets/logmodel.cpp
    widgets/logwidget.cpp
    widgets/mainstatusbarwidget.cpp
    widgets/maintoolbar.cpp
    widgets/maintoolbutton.cpp
    widgets/themediconlabel.cpp
    widgets/themedpushbutton.cpp
    widgets/themedtoolbutton.cpp
    widgets/securityselectorwidget.cpp
    widgets/trendpanelwidget.cpp
    widgets/trendgraphwidget.cpp
)

set(HEADERS
    appicons.h
    appstyle.h
    apptheme.h
    application.h
    fusionstyle.h
    loggingcategories.h

    opcua/attributeformatter.h
    opcua/connectionprofile.h
    opcua/connectionprofilestore.h
    opcua/opcuaclientservice.h
    opcua/opcuatypes.h
    opcua/pkimanager.h
    opcua/secretstore.h

    dialogs/appbasedialog.h
    dialogs/connectiondialog.h
    dialogs/dialogabout.h
    dialogs/certificatetrustdialog.h
    dialogs/writevaluedialog.h

    itestdatapopulatable.h
    mainwindow.h
    testdata.h

    widgets/addressspaceitem.h
    widgets/addressspacemodel.h
    widgets/addressspacewidget.h
    widgets/nodeitem.h
    widgets/nodeinfomodel.h
    widgets/referencesmodel.h
    widgets/attributesmodel.h
    widgets/attributeswidget.h
    widgets/coloredpushbutton.h
    widgets/connectionstatuswidget.h
    widgets/dataaccesswidget.h
    widgets/dataaccessitem.h
    widgets/dataaccessmodel.h
    widgets/eventsmodel.h
    widgets/historymodel.h
    widgets/subscriptiondelegate.h
    widgets/subscriptionitem.h
    widgets/subscriptionsmodel.h
    widgets/endpointselectorwidget.h
    widgets/fixedgap.h
    widgets/headerview.h
    widgets/tableview.h
    widgets/logitem.h
    widgets/logmodel.h
    widgets/logwidget.h
    widgets/mainstatusbarwidget.h
    widgets/maintoolbar.h
    widgets/maintoolbutton.h
    widgets/themediconlabel.h
    widgets/themedpushbutton.h
    widgets/themedtoolbutton.h
    widgets/securityselectorwidget.h
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
