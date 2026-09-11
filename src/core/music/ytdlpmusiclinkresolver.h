#ifndef YTDLPMUSICLINKRESOLVER_H
#define YTDLPMUSICLINKRESOLVER_H

#include "musiclinkresolver.h"

#include <QStringList>

class QProcess;
class QTimer;

// Production MusicLinkResolver: shells out to yt-dlp (see ADR 0001).
//
// Every call is driven by QProcess signals rather than waitForFinished(), so a
// slow or hung yt-dlp never blocks the thread this lives on. That matters
// because the caller is on the UI thread: a playlist of dead videos would
// otherwise freeze the whole app for as long as it takes to walk them.
//
// Entry enumeration uses --flat-playlist, which is cheap for a real playlist.
// Stream resolution (-f bestaudio -g) is the slower, more failure-prone call
// and is only ever invoked just-in-time for the entry about to play.
class YtDlpMusicLinkResolver : public MusicLinkResolver
{
    Q_OBJECT

public:
    explicit YtDlpMusicLinkResolver(QObject *parent = nullptr);
    ~YtDlpMusicLinkResolver() override;

    void requestEntries(const QString &musicLinkUrl) override;
    void requestStreamUrl(const QString &entryUrl) override;
    void cancel() override;

private slots:
    void onProcessFinished(int exitCode, int exitStatus);
    void onProcessErrorOccurred();
    void onTimeout();

private:
    enum class RequestKind { None, Entries, StreamUrl };

    void startRequest(RequestKind kind, const QString &subject, const QStringList &arguments);
    // Answers the in-flight request with these lines (empty = failure) and
    // clears it. Emits after tearing the process down, so a slot that issues
    // the next request sees a resolver that is already idle.
    void deliver(const QStringList &lines);
    void discardProcess();

    QProcess *m_process;
    QTimer *m_timeout;
    RequestKind m_kind;
    QString m_subject;
};

#endif // YTDLPMUSICLINKRESOLVER_H
