#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFile>
#include "../src/core/music/musiclinkmanager.h"
#include "../src/core/pomodoro/pomodorotimer.h"
#include "../src/core/database/databasemanager.h"
#include "fakemusiclinkresolver.h"
#include "fakemusicplayer.h"

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

    void testNoSelectionMeansNoPlaybackOnWorking();
    void testSingleEntryPlaysOnWorkingAndLoopsOnFinish();
    void testSingleEntryRetryThenSilentError();
    void testPlaylistAdvancesToNextEntryOnFinish();
    void testPlaylistLoopsBackToFirstEntry();
    void testPlaylistSkipsBrokenMidEntry();
    void testPlaylistGoesSilentWhenAllEntriesFail();
    void testPauseResumeStopMirrorToPlayer();
    void testStopClearsPlayingState();
    void testStopClearsPlaybackError();
    void testPauseResumeAreNoOpsWhenNotPlaying();
    void testUnresolvableMusicLinkShowsError();
    void testRepeatedPlaybackErrorsEventuallyStopWithError();

    void testResolutionDoesNotBlockTheCaller();
    void testStopWhileResolvingDropsTheLateAnswer();
    void testResolveRetryIsDelayedNotImmediate();
    void testPausedSessionReportsNotPlaying();
    void testStreamErrorWhilePausedStaysSilentUntilResume();
    void testSelectingAnotherLinkMidSessionSwitchesImmediately();
    void testRemovingThePlayingLinkMidSessionStopsIt();
    void testRecoveredErrorsDoNotAccumulateAcrossHealthyPlayback();
    void testLatePlaybackErrorAfterGivingUpStaysSilent();
    void testLatePlaybackErrorAfterStopShowsNoError();
    void testPlayerFailingSynchronouslyIsStillHandled();
    void testStoppingWhilePausedNeverResumesAudio();

    void testPickingALinkResolvesItEagerly();
    void testStartUsesThePrefetchedResultWithoutReResolving();
    void testAStalePrefetchIsResolvedAgain();
    void testAPrefetchKeepsItsEntriesWhenOnlyTheStreamFails();
    void testAPrefetchedPlaylistStillAdvancesThroughItsEntries();
    void testAFailedPrefetchIsSilentAndStartStillReportsIt();
    void testPickingDuringASessionSwitchesRatherThanPrefetching();
    void testStartAbandonsAnInFlightPrefetchAndStillPlays();
    void testASupersededPrefetchIsCancelled();
    void testClickingThroughLinksOnlyResolvesTheOneSettledOn();

private:
    void clearDatabase();
    void selectSingleLink(const QString &url, const QVector<QString> &entries = {});
    void enablePrefetch();
    MusicLinkManager::PlaybackTuning m_tuning;
    PomodoroTimer *m_pomodoro;
    MusicLinkManager *m_manager;
    FakeMusicLinkResolver *m_resolver;
    FakeMusicPlayer *m_player;
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
    m_resolver = new FakeMusicLinkResolver();
    m_player = new FakeMusicPlayer();
    m_manager = new MusicLinkManager(m_pomodoro, m_resolver, m_player, this);

    // No test should sit through a real retry delay. healthyPlaybackMs keeps
    // its production default, so the give-up cap behaves as it does in the app.
    m_tuning = MusicLinkManager::PlaybackTuning();
    m_tuning.retryDelayMs = 0;
    // Prefetching stays off unless a test asks for it, so picking a link never
    // resolves anything behind an unrelated test's back.
    m_tuning.prefetchDebounceMs = 60000;
    m_manager->setPlaybackTuning(m_tuning);
}

void TestMusicLinkManager::cleanup()
{
    delete m_manager;
    m_manager = nullptr;
    delete m_player;
    m_player = nullptr;
    delete m_resolver;
    m_resolver = nullptr;
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

// Adds a single Music Link, tells the resolver what it resolves to, and
// selects it. Entries are configured before the pick, because picking is what
// triggers prefetching - a test that set them afterwards would be relying on
// the prefetch finding nothing.
void TestMusicLinkManager::selectSingleLink(const QString &url, const QVector<QString> &entries)
{
    if (!entries.isEmpty())
        m_resolver->entriesByUrl[url] = entries;

    m_manager->addMusicLink("Test Link", url);
    const int id = m_manager->data(m_manager->index(0, 0), MusicLinkManager::IdRole).toInt();
    m_manager->selectMusicLink(id);
}

// Prefetching is off by default across the suite; the tests that are about it
// turn it on and then pump the event loop for the debounce.
void TestMusicLinkManager::enablePrefetch()
{
    m_tuning.prefetchDebounceMs = 0;
    m_manager->setPlaybackTuning(m_tuning);
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

    QCOMPARE(m_player->lastVolume, 0.0);
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
        FakeMusicLinkResolver resolver1;
        FakeMusicPlayer player1;
        MusicLinkManager manager1(&pomodoro1, &resolver1, &player1, this);
        manager1.addMusicLink("Lo-fi beats", "https://youtube.com/watch?v=1");
        manager1.addMusicLink("Piano focus", "https://youtube.com/watch?v=2");

        selectedId = manager1.data(manager1.index(1, 0), MusicLinkManager::IdRole).toInt();
        manager1.selectMusicLink(selectedId);
        manager1.setVolume(0.75);

        QCOMPARE(manager1.rowCount(), 2);
    }

    {
        PomodoroTimer pomodoro2;
        FakeMusicLinkResolver resolver2;
        FakeMusicPlayer player2;
        MusicLinkManager manager2(&pomodoro2, &resolver2, &player2, this);

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
    FakeMusicLinkResolver resolver2;
    FakeMusicPlayer player2;
    MusicLinkManager manager2(&pomodoro2, &resolver2, &player2, this);

    QCOMPARE(manager2.selectedLinkId(), -1);
}

void TestMusicLinkManager::testNoSelectionMeansNoPlaybackOnWorking()
{
    m_pomodoro->startSession(60, 10);

    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls.count(), 0);
}

void TestMusicLinkManager::testSingleEntryPlaysOnWorkingAndLoopsOnFinish()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});

    m_pomodoro->startSession(60, 10);

    QVERIFY(m_manager->isPlaying());
    QCOMPARE(m_manager->currentEntryUrl(), QString("https://youtube.com/watch?v=solo"));
    QCOMPARE(m_player->playedStreamUrls, QStringList({"https://youtube.com/watch?v=solo"}));

    // The single entry finishing before the session does loops it again.
    m_player->simulateFinished();

    QVERIFY(m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls,
             QStringList({"https://youtube.com/watch?v=solo", "https://youtube.com/watch?v=solo"}));
}

void TestMusicLinkManager::testSingleEntryRetryThenSilentError()
{
    selectSingleLink("https://youtube.com/watch?v=broken", {"https://youtube.com/watch?v=broken"});
    // First attempt and the retry both fail - no other entry to fall back to.
    m_resolver->outcomesByEntry["https://youtube.com/watch?v=broken"] = {false, false};

    QSignalSpy errorSpy(m_manager, &MusicLinkManager::playbackErrorChanged);

    m_pomodoro->startSession(60, 10);

    QTRY_VERIFY(!m_manager->playbackError().isEmpty());
    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls.count(), 0);
    QCOMPARE(m_resolver->streamUrlRequestCount, 2); // initial attempt + one retry
    QVERIFY(errorSpy.count() >= 1);
}

void TestMusicLinkManager::testPlaylistAdvancesToNextEntryOnFinish()
{
    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB", "entryC"});

    m_pomodoro->startSession(60, 10);
    QCOMPARE(m_player->playedStreamUrls, QStringList({"entryA"}));
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryA"));

    m_player->simulateFinished();
    QCOMPARE(m_player->playedStreamUrls, QStringList({"entryA", "entryB"}));
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryB"));

    m_player->simulateFinished();
    QCOMPARE(m_player->playedStreamUrls, QStringList({"entryA", "entryB", "entryC"}));
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryC"));
}

void TestMusicLinkManager::testPlaylistLoopsBackToFirstEntry()
{
    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB"});

    m_pomodoro->startSession(60, 10);
    m_player->simulateFinished(); // -> entryB
    m_player->simulateFinished(); // -> should loop back to entryA

    QCOMPARE(m_player->playedStreamUrls, QStringList({"entryA", "entryB", "entryA"}));
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryA"));
    QVERIFY(m_manager->isPlaying());
}

void TestMusicLinkManager::testPlaylistSkipsBrokenMidEntry()
{
    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB", "entryC"});
    // entryB fails both its initial attempt and its retry.
    m_resolver->outcomesByEntry["entryB"] = {false, false};

    m_pomodoro->startSession(60, 10);
    QCOMPARE(m_player->playedStreamUrls, QStringList({"entryA"}));

    // entryA finishes; entryB is tried and fails twice, so entryC plays instead.
    m_player->simulateFinished();

    QTRY_COMPARE(m_player->playedStreamUrls, QStringList({"entryA", "entryC"}));
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryC"));
    QVERIFY(m_manager->isPlaying());
    QVERIFY(m_manager->playbackError().isEmpty()); // a mid-playlist skip is silent, not an error
}

void TestMusicLinkManager::testPlaylistGoesSilentWhenAllEntriesFail()
{
    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB"});
    m_resolver->outcomesByEntry["entryA"] = {false, false};
    m_resolver->outcomesByEntry["entryB"] = {false, false};

    m_pomodoro->startSession(60, 10);

    // Two entries, each tried twice, then the sweep gives up.
    QTRY_COMPARE(m_resolver->streamUrlRequestCount, 4);
    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls.count(), 0);
    // A fully-broken playlist falls silent without surfacing a repeated (or any) error.
    QVERIFY(m_manager->playbackError().isEmpty());
}

void TestMusicLinkManager::testPauseResumeStopMirrorToPlayer()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);

    // Pausing the timer is the only thing the UI does - the music follows the
    // timer's own paused()/resumed() signals rather than a second explicit call.
    m_pomodoro->pause();
    QCOMPARE(m_player->pauseCallCount, 1);

    m_pomodoro->resume();
    QCOMPARE(m_player->resumeCallCount, 1);

    // PomodoroTimer::startSession() internally calls stop() before
    // transitioning to Working, so the player may already have seen a
    // (harmless, no-op) stop() call by this point - only assert on the
    // increment caused by this explicit manual Stop.
    const int stopCallCountBeforeStop = m_player->stopCallCount;
    m_pomodoro->stop(); // manual Stop -> Idle
    QVERIFY(m_player->stopCallCount > stopCallCountBeforeStop);
    QVERIFY(!m_manager->isPlaying());
}

void TestMusicLinkManager::testStopClearsPlayingState()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);
    QVERIFY(m_manager->isPlaying());

    m_pomodoro->stop();

    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_manager->currentEntryUrl(), QString());
}

void TestMusicLinkManager::testStopClearsPlaybackError()
{
    selectSingleLink("https://youtube.com/watch?v=broken", {"https://youtube.com/watch?v=broken"});
    m_resolver->outcomesByEntry["https://youtube.com/watch?v=broken"] = {false, false};

    m_pomodoro->startSession(60, 10);
    QTRY_VERIFY(!m_manager->playbackError().isEmpty());

    m_pomodoro->stop();

    QVERIFY(m_manager->playbackError().isEmpty());
}

void TestMusicLinkManager::testPauseResumeAreNoOpsWhenNotPlaying()
{
    // Nothing selected, no session started - pause/resume must not reach the player.
    m_pomodoro->pause();
    m_pomodoro->resume();
    QCOMPARE(m_player->pauseCallCount, 0);
    QCOMPARE(m_player->resumeCallCount, 0);

    // Same after a session ends (Idle): resume() must not resurrect stale audio.
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);
    m_pomodoro->stop();

    m_pomodoro->resume();
    QCOMPARE(m_player->resumeCallCount, 0);
}

void TestMusicLinkManager::testUnresolvableMusicLinkShowsError()
{
    selectSingleLink("https://youtube.com/playlist?list=dead");
    // No entriesByUrl configured for this url - fetchEntries() returns empty,
    // simulating a Music Link that can't be resolved at all (dead/private link).

    m_pomodoro->startSession(60, 10);

    QVERIFY(!m_manager->isPlaying());
    QVERIFY(!m_manager->playbackError().isEmpty());
}

void TestMusicLinkManager::testRepeatedPlaybackErrorsEventuallyStopWithError()
{
    // Resolves fine every time, but the stream itself never actually plays
    // (e.g. bad codec) - errorOccurred() fires instead of finished().
    selectSingleLink("https://youtube.com/watch?v=unplayable", {"https://youtube.com/watch?v=unplayable"});

    m_pomodoro->startSession(60, 10);
    QVERIFY(m_manager->isPlaying());

    // Keep failing well past any reasonable retry budget. Each failure is
    // immediate, so none of them counts as healthy playback.
    for (int i = 0; i < 10 && m_manager->isPlaying(); ++i)
        m_player->simulateError();

    QVERIFY(!m_manager->isPlaying());
    QVERIFY(!m_manager->playbackError().isEmpty());
}

// Start must return without waiting on the resolver: in the real app that call
// shells out to yt-dlp, and blocking here freezes the UI and the countdown.
void TestMusicLinkManager::testResolutionDoesNotBlockTheCaller()
{
    m_resolver->deferred = true;
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});

    m_pomodoro->startSession(60, 10);

    // Control is back before anything has been resolved.
    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls.count(), 0);
    QCOMPARE(m_pomodoro->currentState(), PomodoroTimer::Working);

    QTRY_VERIFY(m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls, QStringList({"https://youtube.com/watch?v=solo"}));
}

void TestMusicLinkManager::testStopWhileResolvingDropsTheLateAnswer()
{
    m_resolver->deferred = true;
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});

    m_pomodoro->startSession(60, 10);
    m_pomodoro->stop(); // Stop lands before the resolver has answered

    QVERIFY(m_resolver->cancelCount > 0);

    QTest::qWait(50); // give any queued answer every chance to land

    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls.count(), 0);
    QVERIFY(m_manager->playbackError().isEmpty());
}

void TestMusicLinkManager::testResolveRetryIsDelayedNotImmediate()
{
    m_tuning.retryDelayMs = 60000; // long enough that an immediate retry is obvious
    m_manager->setPlaybackTuning(m_tuning);

    selectSingleLink("https://youtube.com/watch?v=flaky", {"https://youtube.com/watch?v=flaky"});
    m_resolver->outcomesByEntry["https://youtube.com/watch?v=flaky"] = {false, false};

    m_pomodoro->startSession(60, 10);

    // The first attempt failed. The retry must still be waiting, not already
    // spent re-asking during the same blip.
    QCOMPARE(m_resolver->streamUrlRequestCount, 1);
    QVERIFY(!m_manager->isPlaying());
    QVERIFY(m_manager->playbackError().isEmpty());
}

void TestMusicLinkManager::testPausedSessionReportsNotPlaying()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);
    QVERIFY(m_manager->isPlaying());

    QSignalSpy playingSpy(m_manager, &MusicLinkManager::playingChanged);

    m_pomodoro->pause();
    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(playingSpy.count(), 1);

    m_pomodoro->resume();
    QVERIFY(m_manager->isPlaying());
    QCOMPARE(playingSpy.count(), 2);
    // A plain pause/resume picks the same entry back up rather than restarting it.
    QCOMPARE(m_player->resumeCallCount, 1);
    QCOMPARE(m_player->playedStreamUrls.count(), 1);
}

void TestMusicLinkManager::testStreamErrorWhilePausedStaysSilentUntilResume()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);

    m_pomodoro->pause();
    const int playsWhilePaused = m_player->playedStreamUrls.count();

    // A buffering/network error on the open stream while paused must not skip
    // to the next entry and start blasting audio over a paused session.
    m_player->simulateError();
    QCOMPARE(m_player->playedStreamUrls.count(), playsWhilePaused);
    QVERIFY(!m_manager->isPlaying());

    // Resuming acts on what was held back: it advances instead of un-pausing
    // audio that already failed.
    m_pomodoro->resume();
    QCOMPARE(m_player->playedStreamUrls.count(), playsWhilePaused + 1);
    QCOMPARE(m_player->resumeCallCount, 0);
    QVERIFY(m_manager->isPlaying());
}

void TestMusicLinkManager::testSelectingAnotherLinkMidSessionSwitchesImmediately()
{
    m_manager->addMusicLink("First", "https://youtube.com/watch?v=first");
    m_manager->addMusicLink("Second", "https://youtube.com/watch?v=second");
    const int firstId = m_manager->data(m_manager->index(0, 0), MusicLinkManager::IdRole).toInt();
    const int secondId = m_manager->data(m_manager->index(1, 0), MusicLinkManager::IdRole).toInt();

    m_resolver->entriesByUrl["https://youtube.com/watch?v=first"] = {"firstEntry"};
    m_resolver->entriesByUrl["https://youtube.com/watch?v=second"] = {"secondEntry"};

    m_manager->selectMusicLink(firstId);
    m_pomodoro->startSession(60, 10);
    QCOMPARE(m_player->playedStreamUrls, QStringList({"firstEntry"}));

    m_manager->selectMusicLink(secondId);

    QCOMPARE(m_player->playedStreamUrls, QStringList({"firstEntry", "secondEntry"}));
    QCOMPARE(m_manager->currentEntryUrl(), QString("secondEntry"));
    QVERIFY(m_manager->isPlaying());
}

void TestMusicLinkManager::testRemovingThePlayingLinkMidSessionStopsIt()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);
    QVERIFY(m_manager->isPlaying());

    const int stopsBefore = m_player->stopCallCount;
    m_manager->removeMusicLink(0);

    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_manager->currentEntryUrl(), QString());
    QVERIFY(m_player->stopCallCount > stopsBefore);
}

void TestMusicLinkManager::testRecoveredErrorsDoNotAccumulateAcrossHealthyPlayback()
{
    // Every entry plays happily for a while and then breaks, and every skip
    // recovers. That is a working session on a flaky network, not a stream
    // that never plays, so it must never hit the give-up cap.
    m_tuning.healthyPlaybackMs = 0; // every entry counts as having played healthily
    m_manager->setPlaybackTuning(m_tuning);

    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB", "entryC"});

    m_pomodoro->startSession(60, 10);
    QVERIFY(m_manager->isPlaying());

    for (int i = 0; i < 10; ++i)
        m_player->simulateError();

    QVERIFY(m_manager->isPlaying());
    QVERIFY(m_manager->playbackError().isEmpty());
    QCOMPARE(m_player->playedStreamUrls.count(), 11);
}

void TestMusicLinkManager::testLatePlaybackErrorAfterGivingUpStaysSilent()
{
    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB"});
    m_resolver->outcomesByEntry["entryA"] = {false, false};
    m_resolver->outcomesByEntry["entryB"] = {false, false};

    m_pomodoro->startSession(60, 10);
    QTRY_COMPARE(m_resolver->streamUrlRequestCount, 4);
    QVERIFY(m_manager->playbackError().isEmpty());

    // The player can still emit a late error for the media it last held. The
    // playlist already decided to fall silent; that decision has to stick.
    m_player->simulateError();
    m_player->simulateError();

    QVERIFY(m_manager->playbackError().isEmpty());
    QVERIFY(!m_manager->isPlaying());
}

void TestMusicLinkManager::testLatePlaybackErrorAfterStopShowsNoError()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);
    QVERIFY(m_manager->isPlaying());

    m_pomodoro->stop();

    // A queued error from the torn-down media must not paint an error label
    // onto an idle screen.
    m_player->simulateError();

    QVERIFY(m_manager->playbackError().isEmpty());
    QVERIFY(!m_manager->isPlaying());
}

// The manager must have committed its own state before it calls play(), or a
// player that rejects the source from inside that call re-enters an object
// that doesn't yet believe anything is playing - and the failure is dropped,
// leaving the UI reporting playback that is silently dead.
void TestMusicLinkManager::testPlayerFailingSynchronouslyIsStillHandled()
{
    m_player->failSynchronouslyOnPlay = true;

    selectSingleLink("https://youtube.com/watch?v=rejected", {"https://youtube.com/watch?v=rejected"});

    m_pomodoro->startSession(60, 10);

    QVERIFY(!m_player->playedStreamUrls.isEmpty()); // it did try
    QVERIFY(!m_manager->isPlaying());               // and knows it failed
    QVERIFY(!m_manager->playbackError().isEmpty());
}

// Stopping a paused session must tear the audio down, not un-pause it on the
// way out - the player would briefly play again before being stopped.
void TestMusicLinkManager::testStoppingWhilePausedNeverResumesAudio()
{
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    m_pomodoro->startSession(60, 10);

    m_pomodoro->pause();
    QCOMPARE(m_player->pauseCallCount, 1);

    m_pomodoro->stop();

    QCOMPARE(m_player->resumeCallCount, 0);
    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls.count(), 1); // and nothing restarted
}

// Issue #1, story 21: picking a Music Link starts resolving it there and then,
// so that pressing Start later doesn't have to wait on yt-dlp.
void TestMusicLinkManager::testPickingALinkResolvesItEagerly()
{
    enablePrefetch();
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});

    QTRY_COMPARE(m_resolver->entriesRequestCount, 1);
    QTRY_COMPARE(m_resolver->streamUrlRequestCount, 1);

    // Resolving is all it does - nothing plays until a session starts.
    QVERIFY(!m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls.count(), 0);
}

// The point of resolving at pick time: Start plays from what is already
// resolved instead of asking yt-dlp all over again.
void TestMusicLinkManager::testStartUsesThePrefetchedResultWithoutReResolving()
{
    enablePrefetch();
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    QTRY_COMPARE(m_resolver->streamUrlRequestCount, 1);

    const int entriesAfterPick = m_resolver->entriesRequestCount;
    const int streamsAfterPick = m_resolver->streamUrlRequestCount;

    m_pomodoro->startSession(60, 10);

    QVERIFY(m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls, QStringList({"https://youtube.com/watch?v=solo"}));
    QCOMPARE(m_resolver->entriesRequestCount, entriesAfterPick);
    QCOMPARE(m_resolver->streamUrlRequestCount, streamsAfterPick);
}

// A resolved stream URL is a signed, expiring link. Inside the freshness window
// it is used (the test above); past it, Start resolves again rather than
// handing the player something already dead.
void TestMusicLinkManager::testAStalePrefetchIsResolvedAgain()
{
    enablePrefetch();
    m_tuning.prefetchFreshnessMs = 30; // small, but a real window - not zero
    m_manager->setPlaybackTuning(m_tuning);

    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});
    QTRY_COMPARE(m_resolver->streamUrlRequestCount, 1);
    const int streamsAfterPick = m_resolver->streamUrlRequestCount;

    QTest::qWait(60); // age it past the window

    m_pomodoro->startSession(60, 10);

    QVERIFY(m_manager->isPlaying());
    QVERIFY(m_resolver->streamUrlRequestCount > streamsAfterPick);
}

// Enumerating a playlist is the slow half. If only the stream half failed,
// Start must keep the entry list rather than starting the whole job over.
void TestMusicLinkManager::testAPrefetchKeepsItsEntriesWhenOnlyTheStreamFails()
{
    enablePrefetch();
    m_resolver->outcomesByEntry["entryA"] = {false}; // the prefetch's stream attempt fails
    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB"});
    QTRY_COMPARE(m_resolver->streamUrlRequestCount, 1);

    const int entriesAfterPick = m_resolver->entriesRequestCount;

    m_pomodoro->startSession(60, 10);

    QTRY_VERIFY(m_manager->isPlaying());
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryA"));
    QCOMPARE(m_resolver->entriesRequestCount, entriesAfterPick); // enumeration reused
    QVERIFY(m_resolver->streamUrlRequestCount > 1);              // only the stream redone
}

// A prefetch caches the whole entry list plus the first entry's stream. The
// rest of the playlist must still resolve and sequence normally from there.
void TestMusicLinkManager::testAPrefetchedPlaylistStillAdvancesThroughItsEntries()
{
    enablePrefetch();
    selectSingleLink("https://youtube.com/playlist?list=abc", {"entryA", "entryB", "entryC"});
    QTRY_COMPARE(m_resolver->streamUrlRequestCount, 1);

    m_pomodoro->startSession(60, 10);
    QCOMPARE(m_player->playedStreamUrls, QStringList({"entryA"}));

    m_player->simulateFinished();
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryB"));

    m_player->simulateFinished();
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryC"));

    m_player->simulateFinished();
    QCOMPARE(m_manager->currentEntryUrl(), QString("entryA")); // loops back
    QVERIFY(m_manager->isPlaying());
}

// Prefetching is speculative, so a Music Link that can't be resolved must not
// put a red error next to the picker before the user has even pressed Start.
void TestMusicLinkManager::testAFailedPrefetchIsSilentAndStartStillReportsIt()
{
    enablePrefetch();
    selectSingleLink("https://youtube.com/playlist?list=dead"); // resolves to nothing
    QTRY_COMPARE(m_resolver->entriesRequestCount, 1);

    QVERIFY(m_manager->playbackError().isEmpty());
    QVERIFY(!m_manager->isPlaying());

    // Start makes the real attempt, and that one does report.
    m_pomodoro->startSession(60, 10);

    QVERIFY(!m_manager->isPlaying());
    QVERIFY(!m_manager->playbackError().isEmpty());
}

// Mid-session, picking a link switches what is playing right now. Speculation
// must not also fire: it would cancel the resolve the live session is waiting on.
void TestMusicLinkManager::testPickingDuringASessionSwitchesRatherThanPrefetching()
{
    enablePrefetch();
    m_manager->addMusicLink("First", "https://youtube.com/watch?v=first");
    m_manager->addMusicLink("Second", "https://youtube.com/watch?v=second");
    const int firstId = m_manager->data(m_manager->index(0, 0), MusicLinkManager::IdRole).toInt();
    const int secondId = m_manager->data(m_manager->index(1, 0), MusicLinkManager::IdRole).toInt();
    m_resolver->entriesByUrl["https://youtube.com/watch?v=first"] = {"firstEntry"};
    m_resolver->entriesByUrl["https://youtube.com/watch?v=second"] = {"secondEntry"};

    m_manager->selectMusicLink(firstId);
    m_pomodoro->startSession(60, 10);
    const int entriesBeforeSwitch = m_resolver->entriesRequestCount;

    m_manager->selectMusicLink(secondId);

    QCOMPARE(m_player->playedStreamUrls, QStringList({"firstEntry", "secondEntry"}));
    QCOMPARE(m_manager->currentEntryUrl(), QString("secondEntry"));
    QVERIFY(m_manager->isPlaying());

    // Exactly one new request - the switch itself - and nothing left queued
    // that could fire a moment later.
    QCOMPARE(m_resolver->entriesRequestCount, entriesBeforeSwitch + 1);
    QTest::qWait(20);
    QCOMPARE(m_resolver->entriesRequestCount, entriesBeforeSwitch + 1);
}

// Start can land while the prefetch is still out. The speculation is abandoned
// and Start resolves for real - it must not inherit a half-filled cache.
void TestMusicLinkManager::testStartAbandonsAnInFlightPrefetchAndStillPlays()
{
    enablePrefetch();
    m_resolver->deferred = true;
    selectSingleLink("https://youtube.com/watch?v=solo", {"https://youtube.com/watch?v=solo"});

    m_pomodoro->startSession(60, 10); // prefetch has not answered yet

    QTRY_VERIFY(m_manager->isPlaying());
    QCOMPARE(m_player->playedStreamUrls, QStringList({"https://youtube.com/watch?v=solo"}));
    QVERIFY(m_manager->playbackError().isEmpty());
}

// Abandoning speculation has to stop the process it started. Without this the
// real resolver leaves an orphaned yt-dlp behind for every discarded pick.
void TestMusicLinkManager::testASupersededPrefetchIsCancelled()
{
    enablePrefetch();
    m_resolver->deferred = true;
    m_manager->addMusicLink("First", "https://youtube.com/watch?v=first");
    m_manager->addMusicLink("Second", "https://youtube.com/watch?v=second");
    const int firstId = m_manager->data(m_manager->index(0, 0), MusicLinkManager::IdRole).toInt();
    const int secondId = m_manager->data(m_manager->index(1, 0), MusicLinkManager::IdRole).toInt();

    m_manager->selectMusicLink(firstId);
    QTRY_COMPARE(m_resolver->entriesRequestCount, 1);
    const int cancelsBeforeRepick = m_resolver->cancelCount;

    m_manager->selectMusicLink(secondId);

    QVERIFY(m_resolver->cancelCount > cancelsBeforeRepick);
}

// Comparing saved Music Links means clicking down the list. Each click must
// not spawn (and then kill) its own yt-dlp process - only the pick that sticks
// is worth resolving.
void TestMusicLinkManager::testClickingThroughLinksOnlyResolvesTheOneSettledOn()
{
    m_tuning.prefetchDebounceMs = 30;
    m_manager->setPlaybackTuning(m_tuning);

    m_manager->addMusicLink("A", "https://youtube.com/watch?v=a");
    m_manager->addMusicLink("B", "https://youtube.com/watch?v=b");
    m_manager->addMusicLink("C", "https://youtube.com/watch?v=c");
    m_resolver->entriesByUrl["https://youtube.com/watch?v=a"] = {"aEntry"};
    m_resolver->entriesByUrl["https://youtube.com/watch?v=b"] = {"bEntry"};
    m_resolver->entriesByUrl["https://youtube.com/watch?v=c"] = {"cEntry"};

    // Three real clicks, each faster than the debounce but with the event loop
    // running in between - otherwise nothing would have had a chance to fire
    // and the debounce wouldn't be what collapses them.
    for (int row = 0; row < 3; ++row) {
        m_manager->selectMusicLink(m_manager->data(m_manager->index(row, 0), MusicLinkManager::IdRole).toInt());
        QTest::qWait(5);
    }

    QTRY_COMPARE(m_resolver->entriesRequestCount, 1);
    QTest::qWait(60);
    QCOMPARE(m_resolver->entriesRequestCount, 1); // and no stragglers arrive later

    // The one resolve was for the link actually settled on, and Start uses it.
    m_pomodoro->startSession(60, 10);
    QCOMPARE(m_manager->currentEntryUrl(), QString("cEntry"));
    QCOMPARE(m_resolver->entriesRequestCount, 1);
}

QTEST_MAIN(TestMusicLinkManager)
#include "test_musicLinkManager.moc"
