add_executable(kwebappmanager
    main.cpp
    mainwindow.cpp
    mainwindow.h
    pwamanager.cpp
    pwamanager.h
    pwawindow.cpp
    pwawindow.h
    addpwadialog.cpp
    addpwadialog.h
)

target_link_libraries(kwebappmanager
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineWidgets
    KF6::CoreAddons
    KF6::I18n
    KF6::XmlGui
    KF6::WidgetsAddons
    KF6::KIOCore
    KF6::IconThemes
    KF6::ConfigWidgets
)

install(TARGETS kwebappmanager ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kwebappmanagerui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kwebappmanager)
