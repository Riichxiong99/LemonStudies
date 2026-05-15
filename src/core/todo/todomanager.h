#ifndef TODOMANAGER_H
#define TODOMANAGER_H


#include <QAbstractListModel>
#include <QObject>
#include <QDateTime>
#include "todo.h"
class todoManager : public QAbstractListModel
{

    Q_OBJECT


public:
    todoManager(QObject *parent = nullptr);

    enum TodoRoles {
        TitleRole = Qt::UserRole + 1,
        DescriptionRole,
        DueDateRole,
        CompletedRole,
        PriorityRole
    };


    // Methods required by QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;


    //TODO specific methods
    Q_INVOKABLE void addTodo(const QString &title, const QString &description = "",
                             const QDateTime &dueDate = QDateTime(), int priority = 1);
    Q_INVOKABLE void removeTodo(int index);
    Q_INVOKABLE void markAsCompleted(int index, bool completed = true);
    Q_INVOKABLE void updateTodo(int index, const QString &title, const QString &description,
                                const QDateTime &dueDate, int priority);
    Q_INVOKABLE int count() const;
    Q_INVOKABLE void saveTodos(const QString &filename);
    Q_INVOKABLE void loadTodos(const QString &filename);
    Q_INVOKABLE void clearCompleted();
    Q_INVOKABLE void killTodoView();

    signals:
        void todoAdded();
        void todoRemoved();
        void todoUpdated();
        void closeTodoView();

    private:
        QVector<TodoItem*> m_todos;
        void loadTodosFromDatabase();  // Add this private method



};

#endif // TODOMANAGER_H
