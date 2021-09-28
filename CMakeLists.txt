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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(rpkgmaker)
endif()

if (WIN32)
    get_target_property(QMAKE_EXE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    add_custom_command(
        TARGET rpkgmaker
        POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-svg --no-opengl-sw --no-quick --no-translations --release "$<TARGET_FILE:rpkgmaker>")
endif()
