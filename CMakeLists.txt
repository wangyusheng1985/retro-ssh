cmake_minimum_required(VERSION 3.16)
project(RetroSSH VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Options
option(BUILD_TESTING "Build unit tests" ON)
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

# Find Qt (Support both Qt6 and Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Find libssh2 (vcpkg / system / Homebrew)
find_package(Libssh2 CONFIG QUIET)
if (TARGET Libssh2::libssh2)
    set(LIBSSH2_TARGET Libssh2::libssh2)
elseif (TARGET libssh2::libssh2)
    set(LIBSSH2_TARGET libssh2::libssh2)
else()
    find_path(LIBSSH2_INCLUDE_DIR NAMES libssh2.h
        PATHS
        /usr/local/include
        /usr/local/Cellar/libssh2/*/include
        /opt/homebrew/include
        $ENV{LIBSSH2_ROOT}/include
    )

    find_library(LIBSSH2_LIBRARY NAMES ssh2 libssh2
        PATHS
        /usr/local/lib
        /usr/local/Cellar/libssh2/*/lib
        /opt/homebrew/lib
        $ENV{LIBSSH2_ROOT}/lib
    )

    if (NOT LIBSSH2_INCLUDE_DIR OR NOT LIBSSH2_LIBRARY)
        message(FATAL_ERROR "libssh2 not found! Please ensure libssh2 is installed.")
    endif()

    add_library(libssh2_imported UNKNOWN IMPORTED)
    set_target_properties(libssh2_imported PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSSH2_INCLUDE_DIR}"
        IMPORTED_LOCATION "${LIBSSH2_LIBRARY}"
    )
    set(LIBSSH2_TARGET libssh2_imported)
endif()

message(STATUS "Found Qt version: ${QT_VERSION}")

# Platform-specific link libraries
set(PLATFORM_LIBS)
if (WIN32)
    list(APPEND PLATFORM_LIBS ws2_32 crypt32)
endif()

set(LIB_SOURCES
    src/backend/SshSessionWorker.cpp
    src/backend/SftpWorker.cpp
    src/ui/MainWindow.cpp
    src/ui/TerminalWidget.cpp
    src/ui/QuickConnectDialog.cpp
    src/ui/ProfileSettingsDialog.cpp
    src/ui/SftpWindow.cpp
    src/resources/resources.qrc
)

set(LIB_HEADERS
    src/backend/ProfileManager.h
    src/backend/SshSessionWorker.h
    src/backend/SftpWorker.h
    src/ui/MainWindow.h
    src/ui/TerminalWidget.h
    src/ui/QuickConnectDialog.h
    src/ui/ProfileSettingsDialog.h
    src/ui/SftpWindow.h
)

# Core Static Library for RetroSSH
add_library(RetroSSHLib STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(RetroSSHLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(RetroSSHLib PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${LIBSSH2_TARGET}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${PLATFORM_LIBS}
)

# Coverage instrumentation
if (ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang"))
    message(STATUS "Code coverage instrumentation ENABLED")
    target_compile_options(RetroSSHLib PUBLIC --coverage -O0 -g)
    target_link_options(RetroSSHLib PUBLIC --coverage)
endif()

# Main Application Executable
set(MACOSX_ICON ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icons/RetroSSH.icns)
set(APP_SOURCES src/main.cpp)
if (APPLE)
    set_source_files_properties(${MACOSX_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    list(APPEND APP_SOURCES ${MACOSX_ICON})
endif()

add_executable(RetroSSH ${APP_SOURCES})
target_link_libraries(RetroSSH PRIVATE RetroSSHLib)

if (ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang"))
    target_compile_options(RetroSSH PRIVATE --coverage -O0 -g)
    target_link_options(RetroSSH PRIVATE --coverage)
endif()

if (APPLE)
    set_target_properties(RetroSSH PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "RetroSSH"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.retrossh.client"
        MACOSX_BUNDLE_ICON_FILE "RetroSSH.icns"
    )
endif()

# Tests
if (BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()
