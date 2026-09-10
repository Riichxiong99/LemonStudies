#ifndef FAKEMUSICLINKRESOLVER_H
#define FAKEMUSICLINKRESOLVER_H

#include "../src/core/music/musiclinkresolver.h"

#include <QHash>

// Test double for MusicLinkResolver. Entries and per-entry resolve outcomes
// are configured directly by each test rather than parsed from real yt-dlp
// output, so playback orchestration can be exercised without any network
// access or spawning a real process.
class FakeMusicLinkResolver : public MusicLinkResolver
{
public:
    // musicLinkUrl -> ordered entry urls, set up by the test before playback starts.
    QHash<QString, QVector<QString>> entriesByUrl;

    // entryUrl -> queued outcomes, consumed one per resolveStreamUrl() call
    // (true = succeeds, false = fails). An entry with no queued outcomes left
    // always succeeds, using streamUrlByEntry (or the entry url itself).
    QHash<QString, QVector<bool>> outcomesByEntry;
    QHash<QString, QString> streamUrlByEntry;

    int resolveCallCount = 0;

    QVector<QString> fetchEntries(const QString &musicLinkUrl) override
    {
        return entriesByUrl.value(musicLinkUrl);
    }

    QString resolveStreamUrl(const QString &entryUrl) override
    {
        resolveCallCount++;

        QVector<bool> &outcomes = outcomesByEntry[entryUrl];
        const bool succeeds = outcomes.isEmpty() ? true : outcomes.takeFirst();
        if (!succeeds)
            return QString();

        return streamUrlByEntry.value(entryUrl, entryUrl);
    }
};

#endif // FAKEMUSICLINKRESOLVER_H
