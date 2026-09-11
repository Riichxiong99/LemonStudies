#include "ytdlpmusiclinkresolver.h"

#include <QProcess>
#include <QTimer>

namespace {

const char *kProgram = "yt-dlp";

// A single yt-dlp call that has not answered within this long is treated as a
// failure and killed. Nothing waits on it, so this only bounds how long a
// broken entry can stall the *playlist*, never the UI.
constexpr int kRequestTimeoutMs = 30000;

QStringList parseLines(const QByteArray &output)
{
    QStringList lines = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);
    for (QString &line : lines)
        line = line.trimmed();
    lines.removeAll(QString());
    return lines;
}

}

YtDlpMusicLinkResolver::YtDlpMusicLinkResolver(QObject *parent)
    : MusicLinkResolver(parent),
    m_process(nullptr),
    m_timeout(new QTimer(this)),
    m_kind(RequestKind::None)
{
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, &YtDlpMusicLinkResolver::onTimeout);
}

YtDlpMusicLinkResolver::~YtDlpMusicLinkResolver()
{
    discardProcess();
}

void YtDlpMusicLinkResolver::requestEntries(const QString &musicLinkUrl)
{
    // %(webpage_url,url)s, not plain `url`: for a *playlist* --flat-playlist
    // makes `url` the entry's watch URL, but a single-video Music Link is
    // still fully extracted, where `url` is the direct (signed, expiring) CDN
    // stream instead. webpage_url is the watch URL in both shapes, and falls
    // back to url for flat entries that don't carry it.
    startRequest(RequestKind::Entries, musicLinkUrl,
                 {"--flat-playlist", "--print", "%(webpage_url,url)s", musicLinkUrl});
}

void YtDlpMusicLinkResolver::requestStreamUrl(const QString &entryUrl)
{
    startRequest(RequestKind::StreamUrl, entryUrl, {"-f", "bestaudio", "-g", entryUrl});
}

void YtDlpMusicLinkResolver::cancel()
{
    discardProcess();
    m_kind = RequestKind::None;
    m_subject.clear();
}

void YtDlpMusicLinkResolver::startRequest(RequestKind kind, const QString &subject,
                                           const QStringList &arguments)
{
    cancel();

    m_kind = kind;
    m_subject = subject;

    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        onProcessFinished(exitCode, static_cast<int>(status));
    });
    connect(m_process, &QProcess::errorOccurred, this, &YtDlpMusicLinkResolver::onProcessErrorOccurred);

    m_timeout->start(kRequestTimeoutMs);
    m_process->start(QString::fromLatin1(kProgram), arguments);
}

void YtDlpMusicLinkResolver::onProcessFinished(int exitCode, int exitStatus)
{
    // exitCode is deliberately ignored. yt-dlp exits non-zero if *anything*
    // went wrong during the run, including per-entry errors it recovered from
    // after already printing the good entries - a playlist holding one private
    // video is the common case. Discarding stdout there would report the whole
    // Music Link as unresolvable instead of letting the caller skip the entries
    // that are genuinely broken.
    Q_UNUSED(exitCode);

    if (!m_process)
        return;

    QStringList lines;
    if (exitStatus == static_cast<int>(QProcess::NormalExit))
        lines = parseLines(m_process->readAllStandardOutput());

    deliver(lines);
}

void YtDlpMusicLinkResolver::onProcessErrorOccurred()
{
    // Covers yt-dlp missing from PATH (FailedToStart), which never reaches
    // finished(). A crash reaches both; whichever lands first answers the
    // request and detaches the process, so the other is a no-op.
    if (!m_process)
        return;

    deliver({});
}

void YtDlpMusicLinkResolver::onTimeout()
{
    deliver({});
}

void YtDlpMusicLinkResolver::deliver(const QStringList &lines)
{
    const RequestKind kind = m_kind;
    const QString subject = m_subject;

    cancel();

    switch (kind) {
    case RequestKind::Entries:
        emit entriesReady(subject, QVector<QString>(lines.cbegin(), lines.cend()));
        break;
    case RequestKind::StreamUrl:
        emit streamUrlReady(subject, lines.value(0));
        break;
    case RequestKind::None:
        break;
    }
}

void YtDlpMusicLinkResolver::discardProcess()
{
    m_timeout->stop();

    if (!m_process)
        return;

    QProcess *process = m_process;
    m_process = nullptr;

    // Detach first so a kill-induced finished()/errorOccurred() can't re-enter
    // this object, then let the event loop reap it. SIGKILL lands immediately,
    // so the deferred delete never blocks on a live child.
    process->disconnect(this);
    process->kill();
    process->deleteLater();
}
