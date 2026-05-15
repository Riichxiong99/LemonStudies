#include "todomanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>
#include <QVariant>

// Constructor for todoManager class
todoManager::todoManager(QObject *parent)
    : QAbstractListModel(parent)
{
    loadTodosFromDatabase();
}

int todoManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_todos.size();
}

QVariant todoManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_todos.size())
        return QVariant();

    TodoItem *item = m_todos.at(index.row());

    switch (role) {
    case TitleRole:
        return item->title();
    case DescriptionRole:
        return item->description();
    case DueDateRole:
        return item->dueDate();
    case CompletedRole:
        return item->completed();
    case PriorityRole:
        return item->priority();
    default:
        return QVariant();
    }
}

bool todoManager::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_todos.size())
        return false;

    TodoItem *item = m_todos.at(index.row());
    bool changed = false;

    switch (role) {
    case TitleRole:
        item->setTitle(value.toString());
        changed = true;
        break;
    case DescriptionRole:
        item->setDescription(value.toString());
        changed = true;
        break;
    case DueDateRole:
        item->setDueDate(value.toDateTime());
        changed = true;
        break;
    case CompletedRole:
        item->setCompleted(value.toBool());
        changed = true;
        break;
    case PriorityRole:
        item->setPriority(static_cast<TodoItem::Priority>(value.toInt()));
        changed = true;
        break;
    default:
        break;
    }

    if (changed) {
        emit dataChanged(index, index, QVector<int>() << role);
        emit todoUpdated();
        return true;
    }
    return false;
}

Qt::ItemFlags todoManager::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEditable | QAbstractListModel::flags(index);
}

QHash<int, QByteArray> todoManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TitleRole] = "title";
    roles[DescriptionRole] = "description";
    roles[DueDateRole] = "dueDate";
    roles[CompletedRole] = "completed";
    roles[PriorityRole] = "priority";
    return roles;
}

void todoManager::addTodo(const QString &title, const QString &description,
                          const QDateTime &dueDate, int priority)
{
    QSqlQuery query;
    query.prepare("INSERT INTO todos (title, description, due_date, priority) "
                  "VALUES (:title, :description, :due_date, :priority)");
    query.bindValue(":title", title);
    query.bindValue(":description", description);
    query.bindValue(":due_date", dueDate.isValid() ? dueDate : QVariant());
    query.bindValue(":priority", priority);

    if (!query.exec()) {
        qDebug() << "Error adding todo:" << query.lastError().text();
        return;
    }

    int newId = query.lastInsertId().toInt();

    // Add to the model
    beginInsertRows(QModelIndex(), m_todos.size(), m_todos.size());
    TodoItem *newItem = new TodoItem(
        title,
        description,
        dueDate,
        static_cast<TodoItem::Priority>(priority),
        false,
        newId,
        this
        );
    m_todos.append(newItem);
    endInsertRows();

    emit todoAdded();
}

void todoManager::removeTodo(int index)
{
    if (index < 0 || index >= m_todos.size())
        return;

    TodoItem *item = m_todos.at(index);
    int todoId = item->id();

    QSqlQuery query;
    query.prepare("DELETE FROM todos WHERE id = :id");
    query.bindValue(":id", todoId);

    if (!query.exec()) {
        qDebug() << "Error removing todo:" << query.lastError().text();
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_todos.removeAt(index);
    delete item;
    endRemoveRows();

    emit todoRemoved();
}

void todoManager::markAsCompleted(int index, bool completed)
{
    if (index < 0 || index >= m_todos.size())
        return;

    TodoItem *item = m_todos.at(index);
    int todoId = item->id();

    QSqlQuery query;
    query.prepare("UPDATE todos SET completed = :completed WHERE id = :id");
    query.bindValue(":completed", completed);
    query.bindValue(":id", todoId);

    if (!query.exec()) {
        qDebug() << "Error updating todo:" << query.lastError().text();
        return;
    }

    QModelIndex modelIndex = createIndex(index, 0);
    setData(modelIndex, completed, CompletedRole);
}

void todoManager::updateTodo(int index, const QString &title, const QString &description,
                             const QDateTime &dueDate, int priority)
{
    if (index < 0 || index >= m_todos.size())
        return;

    TodoItem *item = m_todos.at(index);
    int todoId = item->id();

    QSqlQuery query;
    query.prepare("UPDATE todos SET title = :title, description = :description, "
                  "due_date = :due_date, priority = :priority WHERE id = :id");
    query.bindValue(":title", title);
    query.bindValue(":description", description);
    query.bindValue(":due_date", dueDate.isValid() ? dueDate : QVariant());
    query.bindValue(":priority", priority);
    query.bindValue(":id", todoId);

    if (!query.exec()) {
        qDebug() << "Error updating todo:" << query.lastError().text();
        return;
    }

    QModelIndex modelIndex = createIndex(index, 0);
    setData(modelIndex, title, TitleRole);
    setData(modelIndex, description, DescriptionRole);
    setData(modelIndex, dueDate, DueDateRole);
    setData(modelIndex, priority, PriorityRole);
}

int todoManager::count() const
{
    return m_todos.size();
}

void todoManager::saveTodos(const QString &filename)
{
    // No longer needed - database automatically saves
    Q_UNUSED(filename);
    qDebug() << "Todos are automatically saved to database";
}

void todoManager::loadTodos(const QString &filename)
{
    // Load from database instead
    Q_UNUSED(filename);
    loadTodosFromDatabase();
}

void todoManager::loadTodosFromDatabase()
{
    beginResetModel();

    qDeleteAll(m_todos);
    m_todos.clear();

    QSqlQuery query("SELECT id, title, description, due_date, priority, completed "
                    "FROM todos ORDER BY created_date DESC");

    if (!query.exec()) {
        qDebug() << "Error loading todos from database:" << query.lastError().text();
        endResetModel();
        return;
    }

    while (query.next()) {
        int id = query.value(0).toInt();
        QString title = query.value(1).toString();
        QString description = query.value(2).toString();
        QDateTime dueDate = query.value(3).toDateTime();
        int priority = query.value(4).toInt();
        bool completed = query.value(5).toBool();

        TodoItem *item = new TodoItem(
            title,
            description,
            dueDate,
            static_cast<TodoItem::Priority>(priority),
            completed,
            id,
            this
            );

        m_todos.append(item);
    }

    endResetModel();
}

void todoManager::clearCompleted()
{
    QSqlQuery query;
    query.prepare("DELETE FROM todos WHERE completed = 1");

    if (!query.exec()) {
        qDebug() << "Error clearing completed todos:" << query.lastError().text();
        return;
    }

    for (int i = m_todos.size() - 1; i >= 0; i--) {
        if (m_todos.at(i)->completed()) {
            beginRemoveRows(QModelIndex(), i, i);
            TodoItem *item = m_todos.takeAt(i);
            delete item;
            endRemoveRows();
        }
    }

    emit todoRemoved();
}

void todoManager::killTodoView()
{
    emit closeTodoView();
    qDebug() << "Closing Todo View";
}
