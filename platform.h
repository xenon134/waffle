#pragma once

#include <QString>

// Platform-specific integration points (shell-outs, default tool locations and
// user folders) live here so main.cpp stays platform-agnostic.
//
// Functions returning QString yield an empty string on success and a short
// user-facing message otherwise; callers show that message in the status label.
namespace Platform {

// Default value for the "magickPath" config key.
QString defaultMagickPath();

// Default value for the "deleteCommand" config key. Empty when deletion is
// handled natively by deleteFile().
QString defaultDeleteCommand();

// Default value for the "editorCommand" config key. Empty when the platform has
// a sensible built-in way to open an image in an editor.
QString defaultEditorCommand();

// Open `filePath` in an image editor. `editorCommand` is the configured
// override (may be empty).
QString openInEditor(const QString &filePath, const QString &editorCommand);

// Reveal `filePath` in the platform's file manager, selecting it if possible.
QString revealInFileManager(const QString &filePath);

// Send `filePath` to the recycle bin / trash. `deleteCommand` is the configured
// override (may be empty).
//
// `status == TrashUnavailable` means the platform has no configured command and
// the file cannot be moved to the trash here (e.g. an NTFS/FUSE mount). The
// caller is then responsible for asking the user before deleting permanently.
struct DeleteOutcome {
    enum Status { Deleted, Error, TrashUnavailable };
    Status status = Deleted;
    QString message; // populated for Error
};
DeleteOutcome deleteFile(const QString &filePath, const QString &deleteCommand);

// Writable directory for downloads and scratch files. Never empty.
QString desktopDirectory();

// Default directories scanned by casual browsing mode (launching with no
// arguments), in scan order. Overridable with the "casualDirectories" key.
QStringList defaultCasualDirectories();

// True when `suffix` (without the leading dot) is an extension we can display.
bool isSupportedImageSuffix(const QString &suffix);

} // namespace Platform
