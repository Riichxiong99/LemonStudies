#ifndef TODO_H
#define TODO_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QAbstractListModel>
#include <QQmlEngine>

class TodoItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QDateTime dueDate READ dueDate WRITE setDueDate NOTIFY dueDateChanged)
    Q_PROPERTY(bool completed READ completed WRITE setCompleted NOTIFY completedChanged)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)  // Add this property

public:
    enum Priority {
        Low = 0,
        Medium = 1,
        High = 2
    };
    Q_ENUM(Priority)

    explicit TodoItem(QObject *parent = nullptr);
    TodoItem(const QString &title, const QString &description = "",
             const QDateTime &dueDate = QDateTime(), Priority priority = Medium,
             bool completed = false, int id = -1, QObject *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);

    QString description() const;
    void setDescription(const QString &description);

    QDateTime dueDate() const;
    void setDueDate(const QDateTime &dueDate);

    bool completed() const;
    void setCompleted(bool completed);

    Priority priority() const;
    void setPriority(Priority priority);

    int id() const;
    void setId(int id);

signals:
    void titleChanged();
    void descriptionChanged();
    void dueDateChanged();
    void completedChanged();
    void priorityChanged();
    void idChanged();
private:
    QString m_title;
    QString m_description;
    QDateTime m_dueDate;
    bool m_completed;
    Priority m_priority;
    int m_id;
};


#endif // TODO_H
