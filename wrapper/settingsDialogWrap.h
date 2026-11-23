#pragma once

#include "appsettings.h"
#include "notesWrap.h"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqmlregistration.h>

// The settings dialog: the dark theme (kept between runs), how sync is doing, and problems with the
// notes on disk.
class SettingsDialogWrap : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
    Q_PROPERTY(int unreadableNotes READ unreadableNotes CONSTANT)
    Q_PROPERTY(bool saveFailed READ saveFailed NOTIFY saveFailedChanged)
    Q_PROPERTY(int syncState READ syncState NOTIFY syncChanged)
    Q_PROPERTY(QString syncProblem READ syncProblem NOTIFY syncChanged)
    Q_PROPERTY(qint64 lastSync READ lastSync NOTIFY syncChanged)
    Q_PROPERTY(QString syncAddress READ syncAddress CONSTANT)

public:
    // In the order of devicesync::ServerSync::Status.
    enum SyncState { SyncOff, Syncing, Synced, SyncOffline, SyncError };
    Q_ENUM(SyncState)

    SettingsDialogWrap(NotesWrap& notes, const QString& settingsFile, QObject* parent = nullptr);
    static SettingsDialogWrap* create(QQmlEngine*, QJSEngine*);

    bool darkTheme() const { return m_settings.darkTheme; }
    void setDarkTheme(bool on);
    int unreadableNotes() const { return m_notes.library().unreadable(); }
    bool saveFailed() const { return m_notes.library().saveFailed(); }
    int syncState() const { return static_cast<int>(m_notes.sync().status()); }
    QString syncProblem() const { return QString::fromStdString(m_notes.sync().problem()); }
    qint64 lastSync() const { return m_notes.sync().lastSyncMs(); }
    QString syncAddress() const { return m_notes.syncAddress(); }

    Q_INVOKABLE void syncNow() { m_notes.sync().syncNow(); }

signals:
    void darkThemeChanged();
    void saveFailedChanged();
    void syncChanged();

private:
    inline static SettingsDialogWrap* s_instance = nullptr;
    NotesWrap& m_notes;
    std::filesystem::path m_file;
    appsettings::Settings m_settings;
};
