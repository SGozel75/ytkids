cmake_minimum_required(VERSION 3.22)

project(ytkids
    VERSION 0.1.0
    DESCRIPTION "YouTube Kids timer with parental controls"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Qt6 modules ───────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    WebEngineWidgets
    WebChannel
    Gui
)

qt_standard_project_setup()

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/playerwindow.cpp
    src/timercontroller.cpp
    src/parentalcontrol.cpp
    src/artworkframe.cpp
    src/config.cpp
)

set(HEADERS
    src/mainwindow.h
    src/playerwindow.h
    src/timercontroller.h
    src/parentalcontrol.h
    src/artworkframe.h
    src/config.h
)

set(RESOURCES
    resources/resources.qrc
)

# ── Target ────────────────────────────────────────────────────────────────────
qt_add_executable(ytkids
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(ytkids PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::Gui
)

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS ytkids
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    packaging/ytkids.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES
    resources/icons/ytkids.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
)

# ── Version header ────────────────────────────────────────────────────────────
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
)

target_include_directories(ytkids PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
)
