#ifndef MUSICLINKRESOLVER_H
#define MUSICLINKRESOLVER_H

#include <QObject>
#include <QString>
#include <QVector>

// Resolves a Music Link (a single-video or playlist YouTube URL) into playable
// audio. Split into two steps so a playlist's entries can be enumerated cheaply
// up front while each entry's actual stream URL is only resolved just-in-time.
//
// Both steps are asynchronous: resolution shells out to yt-dlp in production,
// which routinely takes seconds and occasionally tens of seconds, and the
// caller (MusicLinkManager) lives on the UI thread. A request returns
// immediately and is answered later by exactly one signal.
//
// Contract:
//  - At most one request is in flight at a time. Issuing a new request, or
//    calling cancel(), abandons any in-flight one: no signal is emitted for
//    the abandoned request.
//  - Failure (deleted/private video, network failure, yt-dlp missing, timeout)
//    is reported as an empty result rather than an error signal, since a
//    failed entry is expected to be skipped, not treated as exceptional.
//  - Signals carry the subject they answer, so a caller that has moved on can
//    tell whether a result is still the one it asked for.
class MusicLinkResolver : public QObject
{
    Q_OBJECT

public:
    explicit MusicLinkResolver(QObject *parent = nullptr) : QObject(parent) {}

    // The ordered list of entry URLs for a Music Link. A single-video Music
    // Link resolves to one entry (itself); a playlist resolves to each of its
    // videos' URLs. Answered by entriesReady().
    virtual void requestEntries(const QString &musicLinkUrl) = 0;

    // The direct, playable audio stream URL for one entry. Answered by
    // streamUrlReady().
    virtual void requestStreamUrl(const QString &entryUrl) = 0;

    // Abandons any in-flight request without emitting its signal.
    virtual void cancel() = 0;

signals:
    void entriesReady(const QString &musicLinkUrl, const QVector<QString> &entries);
    void streamUrlReady(const QString &entryUrl, const QString &streamUrl);
};

#endif // MUSICLINKRESOLVER_H
