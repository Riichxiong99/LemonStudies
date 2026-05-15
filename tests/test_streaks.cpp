#include <QtTest/QtTest>
#include <QObject>
#include <QDateTime>
#include "../src/core/streaks/streaks.h"

class TestStreaks : public QObject
{
    Q_OBJECT

private slots:
    void testConstructor();
    void testSetTitle();
    void testIncrementStreak();
    void testLastActivity();
    void testSetLastActivity();
    void testIsActiveToday();
    void testIsStreakBroken();
    void testDaysSinceLastActivity();
    void testBestStreak();
    void testUpdateBestStreak();
    void testRealisticStreakScenario();
};

void TestStreaks::testConstructor()
{
    // Test creating a streak with title and initial duration
    QString expectedTitle = "Daily Reading";
    int expectedDuration = 5;

    Streaks streak(expectedTitle);

    QCOMPARE(streak.title(), expectedTitle);

    for(int i =0;i < 5;i++){
        streak.incrementStreakDuration();
    }
    QCOMPARE(streak.streakDuration(), expectedDuration);
    QCOMPARE(streak.bestStreak(), expectedDuration);
    QVERIFY(streak.lastActivity().isValid());
}

void TestStreaks::testSetTitle()
{
    // Test changing the title
    Streaks streak("Old Title");
    QString newTitle = "New Title";

    streak.setTitle(newTitle);

    QCOMPARE(streak.title(), newTitle);
}

void TestStreaks::testIncrementStreak()
{
    // Test incrementing streak duration
    Streaks streak("Reading", 0);

    streak.incrementStreakDuration();
    QCOMPARE(streak.streakDuration(), 1);

    streak.incrementStreakDuration();
    QCOMPARE(streak.streakDuration(), 2);
}

void TestStreaks::testLastActivity(){

    Streaks streak("Leetcoding");

    streak.incrementStreakDuration();

    QVERIFY(streak.lastActivity().isValid());

    // Get the current time.
    QDateTime now = QDateTime::currentDateTime();
    QDateTime lastActivity = streak.lastActivity();

    QVERIFY(lastActivity.secsTo(now) <= 10);

}

void TestStreaks::testSetLastActivity(){
    Streaks streak("Leetcoding");

    QDateTime now = QDateTime::currentDateTime();

    streak.setLastActivity(now);

    QCOMPARE(streak.lastActivity(),now);
}

void TestStreaks::testIsActiveToday() {
    // Test 1: New streak (no activity) should not be active today
    Streaks streak("Reading");
    QVERIFY(!streak.isActiveToday());  // No last activity set

    // Test 2: Setting today's date should be active
    QDateTime today = QDateTime::currentDateTime();
    streak.setLastActivity(today);
    QVERIFY(streak.isActiveToday());

    // Test 3: Yesterday's activity should NOT be active today
    QDateTime yesterday = QDateTime::currentDateTime().addDays(-1);
    streak.setLastActivity(yesterday);
    QVERIFY(!streak.isActiveToday());

    // Test 4: Incrementing streak should mark as active today
    Streaks newStreak("Exercise");
    newStreak.incrementStreakDuration();  // This should set lastActivity to now
    QVERIFY(newStreak.isActiveToday());
}

// Test cases for isStreakBroken()
void TestStreaks::testIsStreakBroken() {
    Streaks streak("Reading");  // Start with 5-day streak

    for(int i =0;i < 5;i++){
        streak.incrementStreakDuration();
    }

    // Test 1: No last activity = broken
    QVERIFY(!streak.isStreakBroken());

    // Test 2: Active today = not broken
    QDateTime today = QDateTime::currentDateTime();
    streak.setLastActivity(today);
    QVERIFY(!streak.isStreakBroken());

    // Test 3: Yesterday = not broken (grace period)
    QDateTime yesterday = QDateTime::currentDateTime().addDays(-1);
    streak.setLastActivity(yesterday);
    QVERIFY(!streak.isStreakBroken());

    // Test 4: Two days ago = broken
    QDateTime twoDaysAgo = QDateTime::currentDateTime().addDays(-2);
    streak.setLastActivity(twoDaysAgo);
    QVERIFY(streak.isStreakBroken());

    // Test 5: A week ago = definitely broken
    QDateTime weekAgo = QDateTime::currentDateTime().addDays(-7);
    streak.setLastActivity(weekAgo);
    QVERIFY(streak.isStreakBroken());
}

// Test cases for daysSinceLastActivity()
void TestStreaks::testDaysSinceLastActivity() {
    Streaks streak("Reading");

    // Test 1: No last activity should return -1 or some error value
    int daysNoActivity = streak.daysSinceLastActivity();
    QVERIFY(daysNoActivity == -1 || daysNoActivity >= 999);  // Some "invalid" indicator

    // Test 2: Today should be 0 days
    QDateTime today = QDateTime::currentDateTime();
    streak.setLastActivity(today);
    QCOMPARE(streak.daysSinceLastActivity(), 0);

    // Test 3: Yesterday should be 1 day
    QDateTime yesterday = QDateTime::currentDateTime().addDays(-1);
    streak.setLastActivity(yesterday);
    QCOMPARE(streak.daysSinceLastActivity(), 1);

    // Test 4: Three days ago should be 3 days
    QDateTime threeDaysAgo = QDateTime::currentDateTime().addDays(-3);
    streak.setLastActivity(threeDaysAgo);
    QCOMPARE(streak.daysSinceLastActivity(), 3);

    // Test 5: A week ago should be 7 days
    QDateTime weekAgo = QDateTime::currentDateTime().addDays(-7);
    streak.setLastActivity(weekAgo);
    QCOMPARE(streak.daysSinceLastActivity(), 7);
}

// Test cases for bestStreak()
void TestStreaks::testBestStreak() {
    // Test 1: New streak should have best streak of 0
    Streaks streak("Reading");
    QCOMPARE(streak.bestStreak(), 0);

    // Test 2: Setting initial best streak
    Streaks streak2("Exercise");  // If constructor supports initial best streak
    // Or if you have a setter:
    // streak2.setBestStreak(5);
    // QCOMPARE(streak2.bestStreak(), 5);

    // Test 3: Best streak should persist even when current streak resets
    Streaks streak3("Coding");
    // Simulate: streak3 reaches 10, then resets to 0
    // Best streak should still be 10
    // (This test depends on your implementation)
}

// Test cases for updateBestStreak()
void TestStreaks::testUpdateBestStreak() {
    Streaks streak("Reading");

    // Test 1: First increment should set best streak to 1
    streak.setStreakDuration(1);
    streak.updateBestStreak();
    QCOMPARE(streak.bestStreak(), 1);

    // Test 2: Higher streak should update best streak
    streak.setStreakDuration(5);
    streak.updateBestStreak();
    QCOMPARE(streak.bestStreak(), 5);

    // Test 3: Lower streak should NOT update best streak
    streak.setStreakDuration(3);
    streak.updateBestStreak();
    QCOMPARE(streak.bestStreak(), 5);  // Should still be 5, not 3

    // Test 4: Equal streak should not change best streak
    streak.setStreakDuration(5);
    streak.updateBestStreak();
    QCOMPARE(streak.bestStreak(), 5);  // Should remain 5

    // Test 5: New record should update best streak
    streak.setStreakDuration(10);
    streak.updateBestStreak();
    QCOMPARE(streak.bestStreak(), 10);
}

// Bonus: Integration test - realistic streak scenario
void TestStreaks::testRealisticStreakScenario() {
    Streaks streak("Daily Reading");

    // Day 1: Start streak
    streak.incrementStreakDuration();
    QCOMPARE(streak.streakDuration(), 1);
    QVERIFY(streak.isActiveToday());
    QVERIFY(!streak.isStreakBroken());
    QCOMPARE(streak.bestStreak(), 1);

    // Day 5: Continue streak (simulate 4 more days)
    streak.setStreakDuration(5);
    streak.setLastActivity(QDateTime::currentDateTime());
    streak.updateBestStreak();
    QCOMPARE(streak.streakDuration(), 5);
    QCOMPARE(streak.bestStreak(), 5);

    // Day 10: Reach new personal best
    streak.setStreakDuration(10);
    streak.updateBestStreak();
    QCOMPARE(streak.bestStreak(), 10);

    // Streak breaks: Reset to 0 but best streak should remain
    streak.resetStreakDuration();
    QCOMPARE(streak.streakDuration(), 0);
    QCOMPARE(streak.bestStreak(), 10);  // Should remember the record

    // Start new streak: shouldn't affect best streak until it's beaten
    streak.setStreakDuration(3);
    streak.updateBestStreak();
    QCOMPARE(streak.bestStreak(), 10);  // Still 10, not 3
}




// This creates the main function for the test
QTEST_MAIN(TestStreaks)
#include "test_streaks.moc"
