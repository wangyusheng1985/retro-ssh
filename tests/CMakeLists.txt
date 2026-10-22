set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(TEST_SOURCES
    main_test.cpp
    test_profile_manager.cpp
    test_color_utils.cpp
    test_terminal_widget.cpp
    test_ui_dialogs.cpp
)

add_executable(test_suite ${TEST_SOURCES})

target_link_libraries(test_suite PRIVATE
    RetroSSHLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

if (ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang"))
    target_compile_options(test_suite PRIVATE --coverage -O0 -g)
    target_link_options(test_suite PRIVATE --coverage)
endif()

add_test(NAME AllTests COMMAND test_suite)
