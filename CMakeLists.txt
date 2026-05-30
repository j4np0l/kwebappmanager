cmake_minimum_required(VERSION 3.20...3.31)
project(kwebappmanager VERSION 0.1.0 LANGUAGES CXX)

set(QT_DEFAULT_MAJOR_VERSION 6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    I18n
    XmlGui
    WidgetsAddons
    KIO
    IconThemes
    ConfigWidgets
    Notifications
)

add_subdirectory(src)
add_subdirectory(data)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
