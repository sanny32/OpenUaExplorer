include("${PROJECT_SOURCE_DIR}/cmake/qlementine.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/ui_normalization.cmake")

set(UI_FILES
    mainwindow.ui

    dialogs/certificatedetailsdialog.ui
    dialogs/certificatetrustdialog.ui
    dialogs/connectiondialog.ui
    dialogs/dialogabout.ui
    dialogs/editfavoritedialog.ui
    dialogs/settingsdialog.ui
    dialogs/writevaluedialog.ui

    widgets/addressspacewidget.ui
    widgets/attributeswidget.ui
    widgets/certificatedetailswidget.ui
    widgets/certificatesummarywidget.ui
    widgets/connectionstatuswidget.ui
    widgets/dataaccesswidget.ui
    widgets/favoriteswidget.ui
    widgets/logwidget.ui
    widgets/mainstatusbarwidget.ui
    widgets/trendpanelwidget.ui
)

add_library(ouaexp_ui STATIC
    appcolors.h
    appicons.h
    application.cpp
    application.h
    appstyle.cpp
    appstyle.h
    apptheme.cpp
    apptheme.h
    fusionstyle.cpp
    fusionstyle.h
    macappstyle.cpp
    macappstyle.h
    macthemefactory.cpp
    macthemefactory.h
    qlementineappstyle.cpp
    qlementineappstyle.h
    qlementinethemefactory.cpp
    qlementinethemefactory.h
    mainwindow.cpp
    mainwindow.h
    ${UI_FILES}

    dialogs/appbasedialog.cpp
    dialogs/appbasedialog.h
    dialogs/certificatedetailsdialog.cpp
    dialogs/certificatedetailsdialog.h
    dialogs/certificatetrustdialog.cpp
    dialogs/certificatetrustdialog.h
    dialogs/connectiondialog.cpp
    dialogs/connectiondialog.h
    dialogs/dialogabout.cpp
    dialogs/dialogabout.h
    dialogs/editfavoritedialog.cpp
    dialogs/editfavoritedialog.h
    dialogs/settingsdialog.cpp
    dialogs/settingsdialog.h
    dialogs/writevaluedialog.cpp
    dialogs/writevaluedialog.h

    widgets/addressspacewidget.cpp
    widgets/addressspacewidget.h
    widgets/attributeswidget.cpp
    widgets/attributeswidget.h
    widgets/certificatedetailswidget.cpp
    widgets/certificatedetailswidget.h
    widgets/certificatesummarywidget.cpp
    widgets/certificatesummarywidget.h
    widgets/coloredpushbutton.cpp
    widgets/coloredpushbutton.h
    widgets/connectionstatuswidget.cpp
    widgets/connectionstatuswidget.h
    widgets/dataaccesswidget.cpp
    widgets/dataaccesswidget.h
    widgets/dialogbuttonbox.cpp
    widgets/dialogbuttonbox.h
    widgets/valuelineedit.cpp
    widgets/valuelineedit.h
    widgets/endpointdiscoverywidget.cpp
    widgets/endpointdiscoverywidget.h
    widgets/fixedgap.cpp
    widgets/fixedgap.h
    widgets/headerview.cpp
    widgets/headerview.h
    widgets/logwidget.cpp
    widgets/logwidget.h
    widgets/mainstatusbarwidget.cpp
    widgets/mainstatusbarwidget.h
    widgets/maintoolbar.cpp
    widgets/maintoolbar.h
    widgets/favoriteswidget.cpp
    widgets/favoriteswidget.h
    widgets/maintoolbutton.cpp
    widgets/maintoolbutton.h
    widgets/publishingintervaldelegate.cpp
    widgets/publishingintervaldelegate.h
    widgets/subscriptiondelegate.cpp
    widgets/subscriptiondelegate.h
    widgets/tableview.cpp
    widgets/tableview.h
    widgets/themedaction.cpp
    widgets/themedaction.h
    widgets/themediconlabel.cpp
    widgets/themediconlabel.h
    widgets/themedpushbutton.cpp
    widgets/themedpushbutton.h
    widgets/themedtoolbutton.cpp
    widgets/themedtoolbutton.h
    widgets/themepreviewbutton.cpp
    widgets/themepreviewbutton.h
    widgets/trendgraphwidget.cpp
    widgets/trendgraphwidget.h
    widgets/trendpanelwidget.cpp
    widgets/trendpanelwidget.h
)

target_include_directories(ouaexp_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(ouaexp_ui PUBLIC ouaexp_plugins)

ouaexp_configure_ui_enum_normalization(ouaexp_ui)
ouaexp_configure_qt_target(ouaexp_ui)
ouaexp_configure_qlementine_app_style(ouaexp_ui)

if(MSVC)
    target_compile_options(ouaexp_ui PUBLIC /utf-8)
endif()
