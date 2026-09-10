#ifndef YTDLPMUSICLINKRESOLVER_H
#define YTDLPMUSICLINKRESOLVER_H

#include "musiclinkresolver.h"

// Production MusicLinkResolver: shells out to yt-dlp (see ADR 0001). Entry
// enumeration uses --flat-playlist, which is cheap even for a single video
// (yt-dlp just prints that video's own URL back). Stream resolution uses
// -f bestaudio -g, which is the slower, more failure-prone call — this is
// only ever invoked just-in-time for the entry about to play.
class YtDlpMusicLinkResolver : public MusicLinkResolver
{
public:
    QVector<QString> fetchEntries(const QString &musicLinkUrl) override;
    QString resolveStreamUrl(const QString &entryUrl) override;
};

#endif // YTDLPMUSICLINKRESOLVER_H
