#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFile>
#include "../src/core/music/musiclinkmanager.h"
#include "../src/core/pomodoro/pomodorotimer.h"
#include "../src/core/database/databasemanager.h"

class TestMusicLinkManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstructor();
    void testAddMusicLink();
    void testRemoveMusicLink();
    void testRemoveClearsSelectionIfSelected();
    void testSelectMusicLink();
    void testVolume();
    void testRoleNames();
    void testSignalEmission();
    void testInvalidIndices();
    void testDatabasePersistence();
    void testSelectedLinkResetIfMissingOnLoad();

private:
    void clearDatabase();
    PomodoroTimer *m_pomodoro;
    MusicLinkManager *m_manager;
};

void TestMusicLinkManager::initTestCase()
{
    DatabaseManager &dbManager = DatabaseManager::instance();

    QSqlDatabase db = dbManager.database();
    db.close();

    db.setDatabaseName("test_musiclinks.db");

    if (!dbManager.openDatabase()) {
        QFAIL("failed to open database");
    }

    if (!dbManager.createMusicTables()) {
        QFAIL("Failed to create music tables");
    }
}

void TestMusicLinkManager::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QFile::remove("test_musiclinks.db");
}

void TestMusicLinkManager::init()
{
    clearDatabase();

    m_pomodoro = new PomodoroTimer(this);
    m_manager = new MusicLinkManager(m_pomodoro, this);
}

void TestMusicLinkManager::cleanup()
{
    delete m_manager;
    m_manager = nullptr;
    delete m_pomodoro;
    m_pomodoro = nullptr;
}

void TestMusicLinkManager::clearDatabase()
{
    QSqlQuery query;
    query.exec("DELETE FROM music_links");
    query.exec("DELETE FROM sqlite_sequence WHERE name='music_links'");
    query.exec("UPDATE music_settings SET selected_link_id = -1, volume = 0.5 WHERE id = 1");
}

void TestMusicLinkManager::testConstructor()
{
    QCOMPARE(m_manager->rowCount(), 0);
    QCOMPARE(m_manager->count(), 0);
    QCOMPARE(m_manager->selectedLinkId(), -1);
    QCOMPARE(m_manager->volume(), 0.5);
}

void TestMusicLinkManager::testAddMusicLink()
{
    m_manager->addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
    QCOMPARE(m_manager->rowCount(), 1);
    QCOMPARE(m_manager->count(), 1);

    QModelIndex firstIndex = m_manager->index(0, 0);
    QCOMPARE(m_manager->data(firstIndex, MusicLinkManager::LabelRole).toString(), QString("Lo-fi beats"));
    QCOMPARE(m_manager->data(firstIndex, MusicLinkManager::UrlRole).toString(), QString("https://youtube.com/watch?v=1"));
    QVERIFY(m_manager->data(firstIndex, MusicLinkManager::IdRole).toInt() > 0);

    m_manager->addMusicLink("Piano focus", "https://youtube.com/watch?v=2");
    QCOMPARE(m_manager->rowCount(), 2);

    // Adding with a blank label or URL is a no-op.
    m_manager->addMusicLink("", "https://youtube.com/watch?v=3");
    m_manager->addMusicLink("No URL", "   ");
    QCOMPARE(m_manager->rowCount(), 2);
}

void TestMusicLinkManager::testRemoveMusicLink()
{
    m_manager->addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
    m_manager->addMusicLink("Piano focus", "https://youtube.com/watch?v=2");
    m_manager->addMusicLink("Rain sounds", "https://youtube.com/watch?v=3");
    QCOMPARE(m_manager->rowCount(), 3);

    m_manager->removeMusicLink(1); // remove "Piano focus"
    QCOMPARE(m_manager->rowCount(), 2);

    QModelIndex firstIndex = m_manager->index(0, 0);
    QModelIndex secondIndex = m_manager->index(1, 0);
    QCOMPARE(m_manager->data(firstIndex, MusicLinkManager::LabelRole).toString(), QString("Lo-fi beats"));
    QCOMPARE(m_manager->data(secondIndex, MusicLinkManager::LabelRole).toString(), QString("Rain sounds"));
}

void TestMusicLinkManager::testRemoveClearsSelectionIfSelected()
{
    m_manager->addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
    QModelIndex index = m_manager->index(0, 0);
    int id = m_manager->data(index, MusicLinkManager::IdRole).toInt();

    m_manager->selectMusicLink(id);
    QCOMPARE(m_manager->selectedLinkId(), id);

    m_manager->removeMusicLink(0);
    QCOMPARE(m_manager->selectedLinkId(), -1);
}

void TestMusicLinkManager::testSelectMusicLink()
{
    m_manager->addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
    m_manager->addMusicLink("Piano focus", "https://youtube.com/watch?v=2");

    int secondId = m_manager->data(m_manager->index(1, 0), MusicLinkManager::IdRole).toInt();

    m_manager->selectMusicLink(secondId);
    QCOMPARE(m_manager->selectedLinkId(), secondId);

    // Selecting an id that doesn't exist is a no-op.
    m_manager->selectMusicLink(9999);
    QCOMPARE(m_manager->selectedLinkId(), secondId);

    // -1 always clears the selection.
    m_manager->selectMusicLink(-1);
    QCOMPARE(m_manager->selectedLinkId(), -1);
}

void TestMusicLinkManager::testVolume()
{
    QCOMPARE(m_manager->volume(), 0.5);

    m_manager->setVolume(0.8);
    QCOMPARE(m_manager->volume(), 0.8);

    m_manager->setVolume(1.5); // clamps
    QCOMPARE(m_manager->volume(), 1.0);

    m_manager->setVolume(-0.2); // clamps
    QCOMPARE(m_manager->volume(), 0.0);
}

void TestMusicLinkManager::testRoleNames()
{
    QHash<int, QByteArray> roles = m_manager->roleNames();

    QVERIFY(roles.contains(MusicLinkManager::IdRole));
    QVERIFY(roles.contains(MusicLinkManager::LabelRole));
    QVERIFY(roles.contains(MusicLinkManager::UrlRole));

    QCOMPARE(roles[MusicLinkManager::IdRole], QByteArray("id"));
    QCOMPARE(roles[MusicLinkManager::LabelRole], QByteArray("label"));
    QCOMPARE(roles[MusicLinkManager::UrlRole], QByteArray("url"));
}

void TestMusicLinkManager::testSignalEmission()
{
    QSignalSpy addedSpy(m_manager, &MusicLinkManager::musicLinkAdded);
    QSignalSpy removedSpy(m_manager, &MusicLinkManager::musicLinkRemoved);
    QSignalSpy selectedSpy(m_manager, &MusicLinkManager::selectedLinkIdChanged);
    QSignalSpy volumeSpy(m_manager, &MusicLinkManager::volumeChanged);

    m_manager->addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
    QCOMPARE(addedSpy.count(), 1);

    int id = m_manager->data(m_manager->index(0, 0), MusicLinkManager::IdRole).toInt();

    m_manager->selectMusicLink(id);
    QCOMPARE(selectedSpy.count(), 1);

    // Selecting the same id again should not re-emit.
    m_manager->selectMusicLink(id);
    QCOMPARE(selectedSpy.count(), 1);

    m_manager->setVolume(0.9);
    QCOMPARE(volumeSpy.count(), 1);

    m_manager->removeMusicLink(0);
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(selectedSpy.count(), 2); // removing the selected link clears selection
}

void TestMusicLinkManager::testInvalidIndices()
{
    m_manager->addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
    int initialCount = m_manager->rowCount();

    m_manager->removeMusicLink(-1);
    m_manager->removeMusicLink(100);
    QCOMPARE(m_manager->rowCount(), initialCount);

    QModelIndex invalidIndex = m_manager->index(100, 0);
    QVERIFY(!m_manager->data(invalidIndex, MusicLinkManager::LabelRole).isValid());
}

void TestMusicLinkManager::testDatabasePersistence()
{
    int selectedId = -1;

    {
        PomodoroTimer pomodoro1;
        MusicLinkManager manager1(&pomodoro1, this);
        manager1.addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
        manager1.addMusicLink("Piano focus", "https://youtube.com/watch?v=2");

        selectedId = manager1.data(manager1.index(1, 0), MusicLinkManager::IdRole).toInt();
        manager1.selectMusicLink(selectedId);
        manager1.setVolume(0.75);

        QCOMPARE(manager1.rowCount(), 2);
    }

    {
        PomodoroTimer pomodoro2;
        MusicLinkManager manager2(&pomodoro2, this);

        QCOMPARE(manager2.rowCount(), 2);
        QCOMPARE(manager2.selectedLinkId(), selectedId);
        QCOMPARE(manager2.volume(), 0.75);

        QCOMPARE(manager2.data(manager2.index(0, 0), MusicLinkManager::LabelRole).toString(), QString("Lo-fi beats"));
        QCOMPARE(manager2.data(manager2.index(1, 0), MusicLinkManager::LabelRole).toString(), QString("Piano focus"));
    }
}

void TestMusicLinkManager::testSelectedLinkResetIfMissingOnLoad()
{
    m_manager->addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
    int id = m_manager->data(m_manager->index(0, 0), MusicLinkManager::IdRole).toInt();
    m_manager->selectMusicLink(id);

    // Simulate the selected link having been deleted out from under a stale
    // persisted selection (e.g. an interrupted shutdown).
    DatabaseManager::instance().removeMusicLink(id);
    DatabaseManager::instance().saveMusicSettings(id, 0.5);

    PomodoroTimer pomodoro2;
    MusicLinkManager manager2(&pomodoro2, this);

    QCOMPARE(manager2.selectedLinkId(), -1);
}

QTEST_MAIN(TestMusicLinkManager)
#include "test_musicLinkManager.moc"
