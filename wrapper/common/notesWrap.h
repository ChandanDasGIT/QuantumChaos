#pragma once

#include "common/notelibrary.h"
#include "common/serversync.h"
#include "httpClientWrap.h"
#include "schedulerWrap.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

// Owns the app's notes and their sync. The grid's, the editor's and the settings' wrappers all
// work on this one library.
class NotesWrap : public QObject {
    Q_OBJECT

public:
    // syncAddress / syncToken empty: sync stays off.
    NotesWrap(const QString& dataFolder, const crypto::Key& key, const QString& syncAddress,
              const QString& syncToken, QObject* parent = nullptr);

    notes::NoteList& list() { return m_library.list(); }
    NoteLibrary& library() { return m_library; }
    devicesync::ServerSync& sync() { return m_sync; }
    const QString& syncAddress() const { return m_syncAddress; }

    // The app came to the front or went to the back.
    void setActive(bool active) { m_sync.setActive(active); }

    static QVariantList toVariant(const std::vector<ChecklistItem>& items);
    static QStringList toStringList(const std::vector<std::string>& texts);

signals:
    void changed();
    void syncChanged();

private:
    NoteLibrary m_library;
    HttpClientWrap m_http;
    SchedulerWrap m_scheduler;
    devicesync::ServerSync m_sync;
    QString m_syncAddress;
};
