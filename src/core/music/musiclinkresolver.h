#ifndef MUSICLINKRESOLVER_H
#define MUSICLINKRESOLVER_H

#include <QString>
#include <QVector>

// Resolves a Music Link (a single-video or playlist YouTube URL) into playable
// audio. Split into two steps so a playlist's entries can be enumerated cheaply
// up front while each entry's actual stream URL is only resolved just-in-time:
//
//  - fetchEntries(): the ordered list of entry URLs for a Music Link. A
//    single-video Music Link resolves to one entry (itself); a playlist
//    resolves to each of its videos' URLs.
//  - resolveStreamUrl(): the direct, playable audio stream URL for one entry.
//    Returns an empty string on failure (deleted/private video, network
//    failure, etc.) rather than throwing, since a failed entry is expected to
//    be skipped, not treated as exceptional.
class MusicLinkResolver
{
public:
    virtual ~MusicLinkResolver() = default;

    virtual QVector<QString> fetchEntries(const QString &musicLinkUrl) = 0;
    virtual QString resolveStreamUrl(const QString &entryUrl) = 0;
};

#endif // MUSICLINKRESOLVER_H
