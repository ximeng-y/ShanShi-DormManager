cmake_minimum_required(VERSION 3.16)

project(ShanShiDormManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        src/main.cpp
        src/ui/mainwidget.cpp
        src/ui/mainwidget.h
        src/ui/mainwidget.ui
        src/ui/appstyle.cpp
        src/ui/appstyle.h
        src/ui/infobutton.cpp
        src/ui/infobutton.h
        src/ui/uifeedback.cpp
        src/ui/uifeedback.h
        src/ui/overviewpage.cpp
        src/ui/overviewpage.h
        src/ui/overviewpage.ui
        src/ui/studentpage.cpp
        src/ui/studentpage.h
        src/ui/studentpage.ui
        src/ui/studentdetaildialog.cpp
        src/ui/studentdetaildialog.h
        src/ui/studentdetaildialog.ui
        src/ui/addstudentdialog.cpp
        src/ui/addstudentdialog.h
        src/ui/addstudentdialog.ui
        src/ui/gendercorrectiondialog.cpp
        src/ui/gendercorrectiondialog.h
        src/ui/gendercorrectiondialog.ui
        src/ui/dormresourcepage.cpp
        src/ui/dormresourcepage.h
        src/ui/dormresourcepage.ui
        src/ui/addbuildingdialog.cpp
        src/ui/addbuildingdialog.h
        src/ui/addbuildingdialog.ui
        src/core/student.cpp
        src/core/student.h
        src/core/dorm.cpp
        src/core/dorm.h
        src/core/studentmanager.cpp
        src/core/studentmanager.h
        src/core/dormmanager.cpp
        src/core/dormmanager.h
        src/system/check.cpp
        src/system/check.h
        src/core/building.cpp
        src/core/building.h
        src/core/buildingmanager.cpp
        src/core/buildingmanager.h
        src/core/school.cpp
        src/core/school.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ShanShiDormManager
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ShanShiDormManager APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ShanShiDormManager SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ShanShiDormManager
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ShanShiDormManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(ShanShiDormManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ShanShiDormManager)
endif()
set_target_properties(ShanShiDormManager PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ShanShiDormManager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ShanShiDormManager)
endif()
