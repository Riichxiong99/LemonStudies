#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFile>
#include "../src/core/streaks/streaksmanager.h"
#include "../src/core/database/databasemanager.h"

class TeststreaksManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testConstructor();
    void testAddStreak();
    void testRemoveStreak();
    void testIncrementStreak();
    void testResetStreak();
    void testRowCount();
    void testData();
    void testSetData();
    void testRoleNames();
    void testFlags();
    void testTotalStreaks();
    void testActiveStreaks();
    void testSignalEmission();
    void testInvalidIndices();
    void testDatabasePersistence();

private:
    void clearDatabase();
    streaksManager* m_manager;
};

void TeststreaksManager::initTestCase()
{
    DatabaseManager& dbManager = DatabaseManager::instance();

    QSqlDatabase db = dbManager.database();
    db.close();

    db.setDatabaseName("test_lemonstudys.db");

    if(!dbManager.openDatabase()){
        QFAIL("failed to open database");
    }

    if(!dbManager.createStreaksTable()){
        QFAIL("Failed to create streaks table");
    }

}

void TeststreaksManager::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QFile::remove("test_lemonstudys.db");
}

void TeststreaksManager::init()
{
    // Clear database before each test
    clearDatabase();

    // Create a new manager for each test
    m_manager = new streaksManager(this);
}

void TeststreaksManager::cleanup()
{
    // Delete the manager after each test
    delete m_manager;
    m_manager = nullptr;
}

void TeststreaksManager::clearDatabase()
{
    QSqlQuery query;
    query.exec("DELETE FROM streaks");
    query.exec("DELETE FROM sqlite_sequence WHERE name='streaks'"); // Reset auto-increment
}

void TeststreaksManager::testConstructor()
{
    // Manager should be empty initially (after clearing database)
    QCOMPARE(m_manager->rowCount(), 0);
    QCOMPARE(m_manager->count(), 0);
    QCOMPARE(m_manager->totalStreaks(), 0);
    QCOMPARE(m_manager->activeStreaks(), 0);
}

void TeststreaksManager::testAddStreak()
{
    // Add a streak
    m_manager->addStreak("Reading");

    QCOMPARE(m_manager->rowCount(), 1);
    QCOMPARE(m_manager->count(), 1);
    QCOMPARE(m_manager->totalStreaks(), 1);

    // Add another streak
    m_manager->addStreak("Exercise");

    QCOMPARE(m_manager->rowCount(), 2);
    QCOMPARE(m_manager->count(), 2);
    QCOMPARE(m_manager->totalStreaks(), 2);

    // Check data of first streak
    QModelIndex firstIndex = m_manager->index(0, 0);
    QCOMPARE(m_manager->data(firstIndex, streaksManager::TitleRole).toString(), QString("Reading"));
    QCOMPARE(m_manager->data(firstIndex, streaksManager::StreakDurationRole).toInt(), 0);
}

void TeststreaksManager::testRemoveStreak()
{
    // Add some streaks
    m_manager->addStreak("Reading");
    m_manager->addStreak("Exercise");
    m_manager->addStreak("Coding");

    QCOMPARE(m_manager->rowCount(), 3);

    // Remove middle streak
    m_manager->removeStreak(1);  // Remove "Exercise"

    QCOMPARE(m_manager->rowCount(), 2);

    // Check remaining streaks
    QModelIndex firstIndex = m_manager->index(0, 0);
    QModelIndex secondIndex = m_manager->index(1, 0);

    QCOMPARE(m_manager->data(firstIndex, streaksManager::TitleRole).toString(), QString("Reading"));
    QCOMPARE(m_manager->data(secondIndex, streaksManager::TitleRole).toString(), QString("Coding"));
}

void TeststreaksManager::testIncrementStreak()
{
    m_manager->addStreak("Reading");

    QModelIndex index = m_manager->index(0, 0);

    // Initially duration should be 0
    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 0);
    QCOMPARE(m_manager->data(index, streaksManager::BestStreakRole).toInt(), 0);

    // Increment the streak
    m_manager->incrementStreak(0);

    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 1);
    QCOMPARE(m_manager->data(index, streaksManager::BestStreakRole).toInt(), 1);

    // Check that it's active today after incrementing
    QCOMPARE(m_manager->data(index, streaksManager::IsActiveTodayRole).toBool(), true);

    // Increment again
    m_manager->incrementStreak(0);

    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 2);
    QCOMPARE(m_manager->data(index, streaksManager::BestStreakRole).toInt(), 2);
}

void TeststreaksManager::testResetStreak()
{
    m_manager->addStreak("Reading");

    // Increment a few times
    m_manager->incrementStreak(0);
    m_manager->incrementStreak(0);
    m_manager->incrementStreak(0);

    QModelIndex index = m_manager->index(0, 0);
    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 3);
    QCOMPARE(m_manager->data(index, streaksManager::BestStreakRole).toInt(), 3);

    // Reset the streak
    m_manager->resetStreak(0);

    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 0);
    // Best streak should remain after reset
    QCOMPARE(m_manager->data(index, streaksManager::BestStreakRole).toInt(), 3);
}

void TeststreaksManager::testRowCount()
{
    // Test with invalid parent (should return 0 for hierarchical models)
    QModelIndex invalidParent = m_manager->index(0, 0);
    QCOMPARE(m_manager->rowCount(invalidParent), 0);

    // Test with valid parent (root)
    QCOMPARE(m_manager->rowCount(QModelIndex()), 0);

    // Add streaks and test
    m_manager->addStreak("Streak1");
    QCOMPARE(m_manager->rowCount(QModelIndex()), 1);

    m_manager->addStreak("Streak2");
    QCOMPARE(m_manager->rowCount(QModelIndex()), 2);
}

void TeststreaksManager::testData()
{
    m_manager->addStreak("Reading");

    QModelIndex index = m_manager->index(0, 0);

    // Test all roles
    QCOMPARE(m_manager->data(index, streaksManager::TitleRole).toString(), QString("Reading"));
    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 0);
    QCOMPARE(m_manager->data(index, streaksManager::BestStreakRole).toInt(), 0);
    QVERIFY(!m_manager->data(index, streaksManager::LastActivityRole).toDateTime().isValid());
    QCOMPARE(m_manager->data(index, streaksManager::IsActiveTodayRole).toBool(), false);
    QCOMPARE(m_manager->data(index, streaksManager::IsStreakBrokenRole).toBool(), true);
    QCOMPARE(m_manager->data(index, streaksManager::DaysSinceLastActivityRole).toInt(), -1);

    // Test invalid role (should return invalid QVariant)
    QVERIFY(!m_manager->data(index, 9999).isValid());

    // Test invalid index
    QModelIndex invalidIndex;
    QVERIFY(!m_manager->data(invalidIndex, streaksManager::TitleRole).isValid());
}

void TeststreaksManager::testSetData()
{
    m_manager->addStreak("Reading");

    QModelIndex index = m_manager->index(0, 0);

    // Test setting title
    QVERIFY(m_manager->setData(index, "New Reading", streaksManager::TitleRole));
    QCOMPARE(m_manager->data(index, streaksManager::TitleRole).toString(), QString("New Reading"));

    // Test setting streak duration
    QVERIFY(m_manager->setData(index, 5, streaksManager::StreakDurationRole));
    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 5);

    // Test setting invalid role (should return false)
    QVERIFY(!m_manager->setData(index, "test", 9999));

    // Test setting with invalid index
    QModelIndex invalidIndex;
    QVERIFY(!m_manager->setData(invalidIndex, "test", streaksManager::TitleRole));
}

void TeststreaksManager::testRoleNames()
{
    QHash<int, QByteArray> roles = m_manager->roleNames();

    // Test that all expected roles are present
    QVERIFY(roles.contains(streaksManager::TitleRole));
    QVERIFY(roles.contains(streaksManager::StreakDurationRole));
    QVERIFY(roles.contains(streaksManager::BestStreakRole));
    QVERIFY(roles.contains(streaksManager::LastActivityRole));
    QVERIFY(roles.contains(streaksManager::IsActiveTodayRole));
    QVERIFY(roles.contains(streaksManager::IsStreakBrokenRole));
    QVERIFY(roles.contains(streaksManager::DaysSinceLastActivityRole));

    // Test role name mappings
    QCOMPARE(roles[streaksManager::TitleRole], QByteArray("title"));
    QCOMPARE(roles[streaksManager::StreakDurationRole], QByteArray("streakDuration"));
    QCOMPARE(roles[streaksManager::BestStreakRole], QByteArray("bestStreak"));
    QCOMPARE(roles[streaksManager::LastActivityRole], QByteArray("lastActivity"));
    QCOMPARE(roles[streaksManager::IsActiveTodayRole], QByteArray("isActiveToday"));
    QCOMPARE(roles[streaksManager::IsStreakBrokenRole], QByteArray("isStreakBroken"));
    QCOMPARE(roles[streaksManager::DaysSinceLastActivityRole], QByteArray("daysSinceLastActivity"));
}

void TeststreaksManager::testFlags()
{
    m_manager->addStreak("Reading");

    QModelIndex validIndex = m_manager->index(0, 0);
    QModelIndex invalidIndex;

    // Test valid index flags
    Qt::ItemFlags validFlags = m_manager->flags(validIndex);
    QVERIFY(validFlags & Qt::ItemIsSelectable);
    QVERIFY(validFlags & Qt::ItemIsEnabled);
    QVERIFY(validFlags & Qt::ItemIsEditable);

    // Test invalid index flags
    Qt::ItemFlags invalidFlags = m_manager->flags(invalidIndex);
    QCOMPARE(invalidFlags, Qt::NoItemFlags);
}

void TeststreaksManager::testTotalStreaks()
{
    QCOMPARE(m_manager->totalStreaks(), 0);

    m_manager->addStreak("Reading");
    QCOMPARE(m_manager->totalStreaks(), 1);

    m_manager->addStreak("Exercise");
    QCOMPARE(m_manager->totalStreaks(), 2);

    m_manager->removeStreak(0);
    QCOMPARE(m_manager->totalStreaks(), 1);
}

void TeststreaksManager::testActiveStreaks()
{
    // Initially no active streaks
    QCOMPARE(m_manager->activeStreaks(), 0);

    // Add streaks but don't increment (not active)
    m_manager->addStreak("Reading");
    m_manager->addStreak("Exercise");
    QCOMPARE(m_manager->activeStreaks(), 0);

    // Increment one streak (should be active today)
    m_manager->incrementStreak(0);
    QCOMPARE(m_manager->activeStreaks(), 1);

    // Increment second streak
    m_manager->incrementStreak(1);
    QCOMPARE(m_manager->activeStreaks(), 2);
}

void TeststreaksManager::testSignalEmission()
{
    // Set up signal spies
    QSignalSpy streakAddedSpy(m_manager, &streaksManager::streakAdded);
    QSignalSpy streakRemovedSpy(m_manager, &streaksManager::streakRemoved);
    QSignalSpy streakUpdatedSpy(m_manager, &streaksManager::streakUpdated);
    QSignalSpy totalStreaksChangedSpy(m_manager, &streaksManager::totalStreaksChanged);
    QSignalSpy activeStreaksChangedSpy(m_manager, &streaksManager::activeStreaksChanged);

    // Test add streak signals
    m_manager->addStreak("Reading");
    QCOMPARE(streakAddedSpy.count(), 1);
    QCOMPARE(totalStreaksChangedSpy.count(), 1);
    QCOMPARE(activeStreaksChangedSpy.count(), 1);

    // Test increment streak signals
    m_manager->incrementStreak(0);
    QCOMPARE(streakUpdatedSpy.count(), 1);
    QCOMPARE(activeStreaksChangedSpy.count(), 2);  // Should be called again

    // Test reset streak signals
    m_manager->resetStreak(0);
    QCOMPARE(streakUpdatedSpy.count(), 2);  // Should be called again for reset
    QCOMPARE(activeStreaksChangedSpy.count(), 3);

    // Test remove streak signals
    m_manager->removeStreak(0);
    QCOMPARE(streakRemovedSpy.count(), 1);
    QCOMPARE(totalStreaksChangedSpy.count(), 2);
    QCOMPARE(activeStreaksChangedSpy.count(), 4);
}

void TeststreaksManager::testInvalidIndices()
{
    m_manager->addStreak("Reading");

    // Store initial count
    int initialCount = m_manager->rowCount();

    // Test operations with invalid indices (should do nothing)
    m_manager->removeStreak(-1);
    m_manager->removeStreak(100);
    QCOMPARE(m_manager->rowCount(), initialCount);  // Should still have same count

    m_manager->incrementStreak(-1);  // Should do nothing
    m_manager->incrementStreak(100); // Should do nothing

    m_manager->resetStreak(-1);      // Should do nothing
    m_manager->resetStreak(100);     // Should do nothing

    // Verify the existing streak wasn't affected
    QModelIndex index = m_manager->index(0, 0);
    QCOMPARE(m_manager->data(index, streaksManager::TitleRole).toString(), QString("Reading"));
    QCOMPARE(m_manager->data(index, streaksManager::StreakDurationRole).toInt(), 0);

    // Test data with invalid index
    QModelIndex invalidIndex = m_manager->index(100, 0);
    QVERIFY(!m_manager->data(invalidIndex, streaksManager::TitleRole).isValid());

    // Test setData with invalid index
    QVERIFY(!m_manager->setData(invalidIndex, "test", streaksManager::TitleRole));
}

void TeststreaksManager::testDatabasePersistence()
{
    // Test that data persists across manager instances

    // First manager - add and modify a streak
    {
        streaksManager manager1(this);
        manager1.addStreak("Persistent Streak");

        // Increment it a few times
        manager1.incrementStreak(0);
        manager1.incrementStreak(0);

        QModelIndex index = manager1.index(0, 0);
        QCOMPARE(manager1.data(index, streaksManager::StreakDurationRole).toInt(), 2);
        QCOMPARE(manager1.data(index, streaksManager::BestStreakRole).toInt(), 2);
    }

    // Second manager - should load the persisted data
    {
        streaksManager manager2(this);

        QCOMPARE(manager2.rowCount(), 1);

        QModelIndex index = manager2.index(0, 0);
        QCOMPARE(manager2.data(index, streaksManager::TitleRole).toString(), QString("Persistent Streak"));
        QCOMPARE(manager2.data(index, streaksManager::StreakDurationRole).toInt(), 2);
        QCOMPARE(manager2.data(index, streaksManager::BestStreakRole).toInt(), 2);

        // Verify it's active (was incremented today)
        QCOMPARE(manager2.data(index, streaksManager::IsActiveTodayRole).toBool(), true);
    }
}

QTEST_MAIN(TeststreaksManager)
#include "test_streaksManager.moc"
