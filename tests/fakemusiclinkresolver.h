#ifndef FAKEMUSICLINKRESOLVER_H
#define FAKEMUSICLINKRESOLVER_H

#include "../src/core/music/musiclinkresolver.h"

#include <QHash>
#include <QTimer>

#include <functional>

// Test double for MusicLinkResolver. Entries and per-entry resolve outcomes
// are configured directly by each test rather than parsed from real yt-dlp
// output, so playback orchestration can be exercised without any network
// access or spawning a real process.
//
// Answers synchronously by default, which keeps most tests to plain
// assertions. Set deferred = true to answer on a later event-loop turn the way
// the real resolver does, for the tests that care about what happens in the
// gap between asking and being answered.
class FakeMusicLinkResolver : public MusicLinkResolver
{
    Q_OBJECT

public:
    explicit FakeMusicLinkResolver(QObject *parent = nullptr) : MusicLinkResolver(parent) {}

    // musicLinkUrl -> ordered entry urls, set up by the test before playback starts.
    QHash<QString, QVector<QString>> entriesByUrl;

    // entryUrl -> queued outcomes, consumed one per requestStreamUrl() call
    // (true = succeeds, false = fails). An entry with no queued outcomes left
    // always succeeds, using streamUrlByEntry (or the entry url itself).
    QHash<QString, QVector<bool>> outcomesByEntry;
    QHash<QString, QString> streamUrlByEntry;

    bool deferred = false;

    int entriesRequestCount = 0;
    int streamUrlRequestCount = 0;
    int cancelCount = 0;

    void requestEntries(const QString &musicLinkUrl) override
    {
        entriesRequestCount++;
        const QVector<QString> entries = entriesByUrl.value(musicLinkUrl);
        answer([this, musicLinkUrl, entries]() { emit entriesReady(musicLinkUrl, entries); });
    }

    void requestStreamUrl(const QString &entryUrl) override
    {
        streamUrlRequestCount++;

        QVector<bool> &outcomes = outcomesByEntry[entryUrl];
        const bool succeeds = outcomes.isEmpty() ? true : outcomes.takeFirst();
        const QString streamUrl = succeeds ? streamUrlByEntry.value(entryUrl, entryUrl) : QString();

        answer([this, entryUrl, streamUrl]() { emit streamUrlReady(entryUrl, streamUrl); });
    }

    void cancel() override
    {
        cancelCount++;
        m_generation++; // a deferred answer already queued is no longer wanted
    }

private:
    void answer(std::function<void()> emitResult)
    {
        if (!deferred) {
            emitResult();
            return;
        }

        const int generation = m_generation;
        QTimer::singleShot(0, this, [this, generation, emitResult]() {
            if (generation != m_generation)
                return;
            emitResult();
        });
    }

    int m_generation = 0;
};

#endif // FAKEMUSICLINKRESOLVER_H
