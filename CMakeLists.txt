cmake_minimum_required(VERSION 3.16)
project(PlotEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(PlotEngine
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/core/novelproject.h
    src/core/novelproject.cpp
    src/core/projectmanager.h
    src/core/projectmanager.cpp
    src/editor/noveleditor.h
    src/editor/noveleditor.cpp
    src/editor/syntaxhighlighter.h
    src/editor/syntaxhighlighter.cpp
    src/panels/structurepanel.h
    src/panels/structurepanel.cpp
    src/panels/notepanel.h
    src/panels/notepanel.cpp
    resources/resources.qrc
)

target_link_libraries(PlotEngine PRIVATE Qt6::Widgets)
target_include_directories(PlotEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
