cmake_minimum_required(VERSION 3.30)
project(PlotEngine VERSION 1.0.0 LANGUAGES CXX)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    message(FATAL_ERROR
        "PlotEngine expects the vcpkg toolchain file. "
        "Use the 'windows-vcpkg' preset or pass CMAKE_TOOLCHAIN_FILE explicitly."
    )
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# PlotEngine policy: project domain code uses C++ modules and imports the standard library as a module.
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets Network)
find_package(qtadvanceddocking-qt6 CONFIG REQUIRED)

add_executable(PlotEngine
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/editor/noveleditor.h
    src/editor/noveleditor.cpp
    src/editor/syntaxhighlighter.h
    src/editor/syntaxhighlighter.cpp
    src/widgets/textcompareview.h
    src/widgets/textcompareview.cpp
    src/panels/structurepanel.h
    src/panels/structurepanel.cpp
    src/panels/notepanel.h
    src/panels/notepanel.cpp
    src/panels/searchpanel.h
    src/panels/searchpanel.cpp
    src/panels/reviewpanel.h
    src/panels/reviewpanel.cpp
    src/panels/revisionhistorypanel.h
    src/panels/revisionhistorypanel.cpp
    src/panels/protectedsnippetpanel.h
    src/panels/protectedsnippetpanel.cpp
    src/panels/sceneboardpanel.h
    src/panels/sceneboardpanel.cpp
    src/core/novelproject.h
    src/core/novelproject.cpp
    src/core/projectmanager.h
    src/core/projectmanager.cpp
    src/core/autosaver.h
    src/core/autosaver.cpp
    src/ai/batchgenerator.h
    src/ai/batchgenerator.cpp
    src/ai/iaiprovider.h
    src/ai/iaiprovider.cpp
    src/ai/openaiprovider.h
    src/ai/openaiprovider.cpp
    src/ai/anthropicprovider.h
    src/ai/anthropicprovider.cpp
    src/ai/veniceprovider.h
    src/ai/veniceprovider.cpp
    src/ai/revisionprompt_helpers.h
    src/ai/revisionprompt_helpers.cpp
    src/ai/revisiondiff.cpp
    src/ai/revisionprompt_impl.cppm
    resources/resources.qrc
)

if(WIN32)
    target_sources(PlotEngine PRIVATE resources/appicon.rc)
endif()

target_sources(PlotEngine
    PRIVATE
        # PlotEngine policy: domain code is authored as .ixx/.cppm module units.
        FILE_SET plotengine_modules TYPE CXX_MODULES BASE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        FILES
        src/app/metadata.ixx
        src/app/application.cppm
        src/ui/dockpane.ixx
        src/ui/iconfactory.cppm
        src/core/textutils.ixx
        src/core/novelproject.ixx
        src/core/projectmanager.ixx
        src/core/documentcommands.ixx
        src/core/revisionmanager.ixx
        src/core/revisionstore.ixx
        src/ai/contextbuilder.ixx
        src/ai/aidataaction.ixx
        src/ai/secretstore.cppm
        src/ai/revisionprompt.cppm
        src/export/textexporter.ixx
)

target_link_libraries(PlotEngine PRIVATE Qt6::Widgets Qt6::Network ads::qtadvanceddocking-qt6)
if(WIN32)
    target_link_libraries(PlotEngine PRIVATE Crypt32)
endif()
target_include_directories(PlotEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/verdigris
)

if(WIN32)
    set(QT_PLUGIN_DIR_DEBUG "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/Qt6/plugins")
    set(QT_PLUGIN_DIR_RELEASE "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")

    add_custom_command(TARGET PlotEngine POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E echo "Deploying Qt runtime for $<TARGET_FILE_NAME:PlotEngine>"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<IF:$<CONFIG:Debug>,${QT_PLUGIN_DIR_DEBUG}/platforms,${QT_PLUGIN_DIR_RELEASE}/platforms>"
            "$<TARGET_FILE_DIR:PlotEngine>/platforms"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<IF:$<CONFIG:Debug>,${QT_PLUGIN_DIR_DEBUG}/styles,${QT_PLUGIN_DIR_RELEASE}/styles>"
            "$<TARGET_FILE_DIR:PlotEngine>/styles"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<IF:$<CONFIG:Debug>,${QT_PLUGIN_DIR_DEBUG}/tls,${QT_PLUGIN_DIR_RELEASE}/tls>"
            "$<TARGET_FILE_DIR:PlotEngine>/tls"
        VERBATIM
    )
endif()
