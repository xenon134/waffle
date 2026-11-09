#include "platform.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>

namespace {

bool isSet(const QString &value) { return !value.trimmed().isEmpty(); }

// Runs `programPath args...` and maps the outcome onto the viewer's
// "Deleted!" / "Fail: ..." messages. Returns an empty string on success.
QString runDeleteCommand(const QString &programPath, const QStringList &args)
{
    QProcess proc;
    proc.start(programPath, args);
    if (!proc.waitForStarted(10000)) return QStringLiteral("Exe not found!");
    if (!proc.waitForFinished(30000)) {
        proc.kill();
        proc.waitForFinished(1000);
        return QStringLiteral("Timed out");
    }
    if (proc.exitStatus() != QProcess::NormalExit) return QStringLiteral("Crashed");
    if (proc.exitCode() == 0) return QString();
    return QString("Exit code %1").arg(proc.exitCode());
}

#ifdef Q_OS_WIN
QString toNative(const QString &path) { return QDir::toNativeSeparators(path); }
#endif

#ifdef Q_OS_LINUX
bool findExecutable(const QString &name, QString *resolved = nullptr)
{
    const QString path = QStandardPaths::findExecutable(name);
    if (path.isEmpty()) return false;
    if (resolved) *resolved = path;
    return true;
}

// Runs `program args...`, waits for it and reports failure via `error`.
bool runAndWait(const QString &program, const QStringList &args, int timeoutMs, QString *error)
{
    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForStarted(10000)) {
        if (error) *error = QString("%1 not found").arg(program);
        return false;
    }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(1000);
        if (error) *error = QString("%1 timed out").arg(program);
        return false;
    }
    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        const QString stderrText = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();
        if (error) {
            *error = stderrText.isEmpty()
                ? QString("%1 failed (exit code %2)").arg(program).arg(proc.exitCode())
                : stderrText;
        }
        return false;
    }
    return true;
}
#endif

} // namespace

namespace Platform {

bool isSupportedImageSuffix(const QString &suffix)
{
    static const QSet<QString> supported = {
        QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("jpe"),
        QStringLiteral("jfif"), QStringLiteral("png"), QStringLiteral("gif"),
        QStringLiteral("webp"), QStringLiteral("bmp"), QStringLiteral("dib"),
        QStringLiteral("ico"), QStringLiteral("tiff"), QStringLiteral("tif"),
        QStringLiteral("svg"), QStringLiteral("svgz"), QStringLiteral("heic"),
        QStringLiteral("heif"), QStringLiteral("avif"), QStringLiteral("tga"),
        QStringLiteral("psd"), QStringLiteral("dng"), QStringLiteral("hdr"),
        QStringLiteral("exr"), QStringLiteral("jp2"), QStringLiteral("j2k"),
        QStringLiteral("jpx"), QStringLiteral("jpm"), QStringLiteral("mj2"),
        QStringLiteral("pnm"), QStringLiteral("pbm"), QStringLiteral("pgm"),
        QStringLiteral("ppm"), QStringLiteral("xbm"), QStringLiteral("xpm"),
        QStringLiteral("dds")
    };
    return supported.contains(suffix.toLower());
}

QString desktopDirectory()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty() || !QFileInfo(dir).isDir()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    if (dir.isEmpty()) dir = QDir::tempPath();
    return dir;
}

#ifdef Q_OS_WIN

QString defaultMagickPath()
{
    // Kept byte-for-byte identical to the historical Windows default so existing
    // installs keep working untouched.
    return "C:\\Program Files\\ImageMagick-7.1.1-Q16-HDRI\\magick.exe";
}

QString defaultDeleteCommand() { return QStringLiteral("delet.exe"); }

QString defaultEditorCommand() { return QString(); }

QString openInEditor(const QString &filePath, const QString &editorCommand)
{
    // mspaint.exe has always been the Windows default; an explicit
    // editorCommand takes precedence.
    const QString program = isSet(editorCommand) ? editorCommand : QStringLiteral("mspaint.exe");
    if (!QProcess::startDetached(program, QStringList() << toNative(filePath))) {
        return QString("Could not launch %1").arg(program);
    }
    return QString();
}

QString revealInFileManager(const QString &filePath)
{
    if (!QProcess::startDetached("explorer.exe", QStringList() << ("/select," + toNative(filePath)))) {
        return QStringLiteral("Could not launch explorer.exe");
    }
    return QString();
}

DeleteOutcome deleteFile(const QString &filePath, const QString &deleteCommand)
{
    const QString program = isSet(deleteCommand) ? deleteCommand : defaultDeleteCommand();
    const QString error = runDeleteCommand(program, QStringList() << toNative(filePath));
    if (error.isEmpty()) return DeleteOutcome{DeleteOutcome::Deleted, QString()};
    return DeleteOutcome{DeleteOutcome::Error, error};
}

#elif defined(Q_OS_LINUX)

QString defaultMagickPath()
{
    // ImageMagick 7 ships "magick"; ImageMagick 6 only has "convert".
    QString resolved;
    if (findExecutable("magick", &resolved)) return resolved;
    if (findExecutable("convert", &resolved)) return resolved;
    return QStringLiteral("magick"); // Let the launch fail with a clear message.
}

QString defaultDeleteCommand() { return QString(); }

QString defaultEditorCommand() { return QString(); }

QString openInEditor(const QString &filePath, const QString &editorCommand)
{
    if (isSet(editorCommand)) {
        // Accept both "gimp" and "gimp --new-instance" style values.
        const QStringList parts = QProcess::splitCommand(editorCommand);
        if (parts.isEmpty()) return QStringLiteral("editorCommand is empty");
        const QString program = parts.first();
        if (!QProcess::startDetached(program, parts.mid(1) + QStringList{filePath})) {
            return QString("Could not launch %1").arg(program);
        }
        return QString();
    }

    if (!QProcess::startDetached("xdg-open", QStringList{filePath})) {
        return QStringLiteral("Could not launch xdg-open");
    }
    return QString();
}

QString revealInFileManager(const QString &filePath)
{
    const QString absPath = QFileInfo(filePath).absoluteFilePath();

    // Ask the desktop's file manager to select the file. Nautilus, Dolphin,
    // Thunar, Nemo and Caja all implement org.freedesktop.FileManager1.
    if (findExecutable("dbus-send")) {
        QString error;
        const QString uri = QUrl::fromLocalFile(absPath).toString();
        if (runAndWait("dbus-send", QStringList{
                "--session",
                "--dest=org.freedesktop.FileManager1",
                "--type=method_call",
                "/org/freedesktop/FileManager1",
                "org.freedesktop.FileManager1.ShowItems",
                "array:string:" + uri,
                "string:"
            }, 5000, &error)) {
            return QString();
        }
    }

    // Fall back to simply opening the containing folder.
    if (!QProcess::startDetached("xdg-open", QStringList{QFileInfo(absPath).absolutePath()})) {
        return QStringLiteral("Could not launch xdg-open");
    }
    return QString();
}

DeleteOutcome deleteFile(const QString &filePath, const QString &deleteCommand)
{
    if (isSet(deleteCommand)) {
        const QString error = runDeleteCommand(deleteCommand, QStringList{filePath});
        if (error.isEmpty()) return DeleteOutcome{DeleteOutcome::Deleted, QString()};
        return DeleteOutcome{DeleteOutcome::Error, error};
    }

    // Prefer moving to the trash over destroying data outright.
    QString error;
    if (findExecutable("gio") && runAndWait("gio", QStringList{"trash", filePath}, 30000, &error)) {
        return DeleteOutcome{DeleteOutcome::Deleted, QString()};
    }
    if (findExecutable("trash-put") && runAndWait("trash-put", QStringList{filePath}, 30000, &error)) {
        return DeleteOutcome{DeleteOutcome::Deleted, QString()};
    }

    // No trash available here: let the caller confirm a permanent delete.
    return DeleteOutcome{DeleteOutcome::TrashUnavailable, QString()};
}

#else

QString defaultMagickPath() { return QStringLiteral("magick"); }
QString defaultDeleteCommand() { return QString(); }
QString defaultEditorCommand() { return QString(); }

QString openInEditor(const QString &, const QString &)
{
    return QStringLiteral("unsupported platform");
}

QString revealInFileManager(const QString &)
{
    return QStringLiteral("unsupported platform");
}

DeleteOutcome deleteFile(const QString &, const QString &)
{
    return DeleteOutcome{DeleteOutcome::Error, QStringLiteral("unsupported platform")};
}

#endif

} // namespace Platform
