#include "ytdlpmusiclinkresolver.h"

#include <QProcess>

namespace {

// Both yt-dlp calls used here are simple, bounded, one-shot lookups; running
// them synchronously keeps MusicLinkManager's orchestration straightforward
// (no callback bookkeeping) and matches the "just-in-time" resolution model,
// which already keeps the expensive call off the UI's critical path (it never
// runs for more entries than are about to be played).
QStringList runYtDlp(const QStringList &arguments)
{
    QProcess process;
    process.start("yt-dlp", arguments);

    if (!process.waitForStarted(5000))
        return {};

    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(1000);
        return {};
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return {};

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (QString &line : lines)
        line = line.trimmed();
    return lines;
}

}

QVector<QString> YtDlpMusicLinkResolver::fetchEntries(const QString &musicLinkUrl)
{
    const QStringList lines = runYtDlp({"--flat-playlist", "--print", "url", musicLinkUrl});
    return QVector<QString>(lines.cbegin(), lines.cend());
}

QString YtDlpMusicLinkResolver::resolveStreamUrl(const QString &entryUrl)
{
    const QStringList lines = runYtDlp({"-f", "bestaudio", "-g", entryUrl});
    if (lines.isEmpty())
        return QString();
    return lines.first();
}
