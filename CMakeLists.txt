cmake_minimum_required(VERSION 2.8)

project(rpkgmaker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)

add_executable(rpkgmaker
    src/mainwindow.h
    src/mainwindow.cpp
    src/mainwindow.ui
    src/main.cpp
    src/main.qrc)

target_link_libraries(rpkgmaker PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
