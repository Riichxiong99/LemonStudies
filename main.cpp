#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "src/core/pomodoro/pomodorotimer.h"
#include "src/core/todo/todo.h"
#include "src/core/database/databasemanager.h"
#include "src/core/todo/todomanager.h"
#include "src/core/streaks/streaksmanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Initialize database before creating models
    DatabaseManager& dbManager = DatabaseManager::instance();

    if (!dbManager.openDatabase()) {
        qDebug() << "Failed to open database!";
        return -1;
    }

    // Create all necessary tables
    if (!dbManager.createTables()) {  // Creates todos table
        qDebug() << "Failed to create todos table!";
        return -1;
    }

    if (!dbManager.createStreaksTable()) {
        qDebug() << "Failed to create streaks table!";
        return -1;
    }

    // Register QML types
    qmlRegisterType<PomodoroTimer>("MyPomodoro", 1, 0, "PomodoroTimer");
    qmlRegisterType<TodoItem>("MyTodo", 1, 0, "TodoItem");
    qmlRegisterType<todoManager>("MyTodo", 1, 0, "TodoManager");
    qmlRegisterType<streaksManager>("com.lemonStudys", 1, 0, "StreaksManager");

    // Create model instances
    todoManager *todoModel = new todoManager(&app);
    streaksManager *streaksModel = new streaksManager(&app);
    PomodoroTimer *pomodoroTimer = new PomodoroTimer(&app);

    QQmlApplicationEngine engine;

    // Expose models to QML
    engine.rootContext()->setContextProperty("todoModelInstance", todoModel);
    engine.rootContext()->setContextProperty("streaksModelInstance", streaksModel);
    engine.rootContext()->setContextProperty("pomodoroTimer",pomodoroTimer);

    // Handle creation failures
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("lemonStudys", "MainMenu");

    return app.exec();
}
