cmake_minimum_required(VERSION 3.16)

project(lemonStudys VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Sql Test)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(applemonStudys
    main.cpp
)

qt_add_qml_module(applemonStudys
    URI lemonStudys
    VERSION 1.0
    QML_FILES
        QML_FILES qml/MainMenu.qml
        QML_FILES qml/PomodoroView.qml
        SOURCES src/core/pomodoro/pomodorotimer.h src/core/pomodoro/pomodorotimer.cpp
        QML_FILES qml/todo.qml
        SOURCES src/core/todo/todo.h src/core/todo/todo.cpp
        QML_FILES
        SOURCES src/core/database/databasemanager.cpp
        SOURCES src/core/database/databasemanager.h
        QML_FILES
        SOURCES src/core/streaks/streaks.h src/core/streaks/streaks.cpp
        QML_FILES qml/streaks.qml
        RESOURCES assets/fonts/Fredoka.ttf
        RESOURCES assets/fonts/FredokaOne.ttf
        RESOURCES assets/sounds/water_drop_sped_up.wav
        RESOURCES assets/images/exiticon.png
        SOURCES src/core/streaks/streaksmanager.h src/core/streaks/streaksmanager.cpp
        SOURCES src/core/todo/todomanager.h src/core/todo/todomanager.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(applemonStudys PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.applemonStudys
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(applemonStudys
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
)

qt_add_executable(streaks_tests
    tests/test_streaks.cpp
    src/core/streaks/streaks.cpp
    src/core/streaks/streaks.h
)

target_link_libraries(streaks_tests
    PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
)

# Enable testing
enable_testing()
add_test(NAME streaks_tests COMMAND streaks_tests)

qt_add_executable(streaksmanager_tests
    tests/test_streaksManager.cpp
    src/core/streaks/streaks.cpp
    src/core/streaks/streaks.h
    src/core/streaks/streaksmanager.cpp
    src/core/streaks/streaksmanager.h
    src/core/database/databasemanager.cpp
    src/core/database/databasemanager.h
)
target_link_libraries(streaksmanager_tests
    PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
)
add_test(NAME streaksmanager_tests COMMAND streaksmanager_tests)

include(GNUInstallDirs)
install(TARGETS applemonStudys
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
