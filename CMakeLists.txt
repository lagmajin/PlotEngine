cmake_minimum_required(VERSION 3.30)
project(PlotEngine VERSION 1.0.0 LANGUAGES CXX)

if(NOT CMAKE_TOOLCHAIN_FILE OR NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    message(FATAL_ERROR
        "PlotEngine is configured to build through vcpkg manifest mode. "
        "Use the 'windows-vcpkg' preset or pass the vcpkg toolchain file explicitly."
    )
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD ON)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets Network)
find_package(qtadvanceddocking-qt6 CONFIG REQUIRED)

add_executable(PlotEngine
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/core/novelproject.h
    src/core/novelproject.cpp
    src/core/projectmanager.h
    src/core/projectmanager.cpp
    src/ai/batchgenerator.h
    src/ai/batchgenerator.cpp
    resources/resources.qrc
)

target_sources(PlotEngine
    PRIVATE
        FILE_SET plotengine_modules TYPE CXX_MODULES BASE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        FILES
        src/core/textutils.ixx
        src/core/novelproject.ixx
        src/core/projectmanager.ixx
        src/core/revisionmanager.ixx
        src/ai/contextbuilder.ixx
        src/ai/revisionprompt.cppm
        src/export/textexporter.ixx
)

target_link_libraries(PlotEngine PRIVATE Qt6::Widgets Qt6::Network qt6advanceddocking)
target_include_directories(PlotEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/verdigris
)
