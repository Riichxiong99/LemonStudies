#include "todo.h"
#include "../database/databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// TodoItem implementation
TodoItem::TodoItem(QObject *parent)
    : QObject(parent),
    m_title(""),
    m_description(""),
    m_dueDate(QDateTime()),
    m_completed(false),
    m_priority(Medium),
    m_id(-1)
{
}

TodoItem::TodoItem(const QString &title, const QString &description,
                   const QDateTime &dueDate, Priority priority,
                   bool completed, int id, QObject *parent)
    : QObject(parent),
    m_title(title),
    m_description(description),
    m_dueDate(dueDate),
    m_completed(completed),
    m_priority(priority),
    m_id(id)
{
}

QString TodoItem::title() const
{
    return m_title;
}

void TodoItem::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
    }
}

QString TodoItem::description() const
{
    return m_description;
}

void TodoItem::setDescription(const QString &description)
{
    if (m_description != description) {
        m_description = description;
        emit descriptionChanged();
    }
}

QDateTime TodoItem::dueDate() const
{
    return m_dueDate;
}

void TodoItem::setDueDate(const QDateTime &dueDate)
{
    if (m_dueDate != dueDate) {
        m_dueDate = dueDate;
        emit dueDateChanged();
    }
}

bool TodoItem::completed() const
{
    return m_completed;
}

void TodoItem::setCompleted(bool completed)
{
    if (m_completed != completed) {
        m_completed = completed;
        emit completedChanged();
    }
}

TodoItem::Priority TodoItem::priority() const
{
    return m_priority;
}

void TodoItem::setPriority(Priority priority)
{
    if (m_priority != priority) {
        m_priority = priority;
        emit priorityChanged();
    }
}

int TodoItem::id() const
{
    return m_id;
}

void TodoItem::setId(int id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged();
    }
}

