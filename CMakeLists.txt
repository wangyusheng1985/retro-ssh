cmake_minimum_required(VERSION 3.16)
project(RetroSSH VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt (Support both Qt6 and Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Find libssh2
find_path(LIBSSH2_INCLUDE_DIR NAMES libssh2.h
    PATHS
    /usr/local/include
    /usr/local/Cellar/libssh2/*/include
    /opt/homebrew/include
)

find_library(LIBSSH2_LIBRARY NAMES ssh2 libssh2
    PATHS
    /usr/local/lib
    /usr/local/Cellar/libssh2/*/lib
    /opt/homebrew/lib
)

if (NOT LIBSSH2_INCLUDE_DIR OR NOT LIBSSH2_LIBRARY)
    message(FATAL_ERROR "libssh2 not found! Please ensure libssh2 is installed.")
endif()

message(STATUS "Found Qt version: ${QT_VERSION}")
message(STATUS "Found libssh2 include: ${LIBSSH2_INCLUDE_DIR}")
message(STATUS "Found libssh2 lib: ${LIBSSH2_LIBRARY}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIBSSH2_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

set(SOURCES
    src/main.cpp
    src/backend/SshSessionWorker.cpp
    src/backend/SftpWorker.cpp
    src/ui/MainWindow.cpp
    src/ui/TerminalWidget.cpp
    src/ui/QuickConnectDialog.cpp
    src/ui/ProfileSettingsDialog.cpp
    src/ui/SftpWindow.cpp
    src/resources/resources.qrc
)

set(HEADERS
    src/backend/SshSessionWorker.h
    src/backend/SftpWorker.h
    src/ui/MainWindow.h
    src/ui/TerminalWidget.h
    src/ui/QuickConnectDialog.h
    src/ui/ProfileSettingsDialog.h
    src/ui/SftpWindow.h
)

add_executable(RetroSSH ${SOURCES} ${HEADERS})

target_link_libraries(RetroSSH PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${LIBSSH2_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
)

if (APPLE)
    set_target_properties(RetroSSH PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "RetroSSH"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.retrossh.client"
    )
endif()
