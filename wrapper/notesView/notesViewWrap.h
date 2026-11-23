#pragma once

#include "notesWrap.h"

#include <QAbstractListModel>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// The grid: the cards of the chosen section (pinned ones first), the search, and the actions on a card.
class NotesViewWrap : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int section READ section WRITE setSection NOTIFY viewChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY viewChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY viewChanged)
    Q_PROPERTY(bool canAdd READ canAdd NOTIFY viewChanged)
    Q_PROPERTY(int count READ count NOTIFY countsChanged)
    Q_PROPERTY(int pinnedCount READ pinnedCount NOTIFY countsChanged)
    Q_PROPERTY(QStringList labels READ labels NOTIFY labelsChanged)
    Q_PROPERTY(int syncState READ syncState NOTIFY syncStateChanged)

public:
    enum Section { Notes, Archive, Trash, Label };
    Q_ENUM(Section)
    // In the order of devicesync::ServerSync::Status.
    enum SyncState { SyncOff, Syncing, Synced, SyncOffline, SyncError };
    Q_ENUM(SyncState)
    enum Role { IdRole = Qt::UserRole + 1, TitleRole, BodyRole, ChecklistRole, PreviewItemsRole, DoneCountRole,
                BlankRole, ColorRole, ColouredRole, PinnedRole, ArchivedRole, TrashedRole, LabelsRole };

    explicit NotesViewWrap(NotesWrap& notes, QObject* parent = nullptr);
    static NotesViewWrap* create(QQmlEngine*, QJSEngine*);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int section() const { return m_section; }
    void setSection(int section);
    QString label() const { return m_label; }
    void setLabel(const QString& label);
    QString search() const { return m_search; }
    void setSearch(const QString& search);
    bool canAdd() const;
    int count() const { return static_cast<int>(m_ids.size()); }
    int pinnedCount() const { return m_pinnedCount; }
    QStringList labels() const;
    int syncState() const { return static_cast<int>(m_notes.sync().status()); }

    // {columns, columnWidth, leftEdge}
    Q_INVOKABLE QVariantMap grid(double width, double cardWidth, double gap, bool compact) const;
    // {x: [...], y: [...], pinnedLabelY, othersLabelY, height}: where each card and label row goes.
    Q_INVOKABLE QVariantMap layout(const QVariantList& heights, double width, double cardWidth, double gap,
                                   double labelHeight, bool compact) const;

    Q_INVOKABLE QString newNote(bool checklist);
    Q_INVOKABLE void setPinned(const QString& id, bool on);
    Q_INVOKABLE void setColor(const QString& id, int color);
    Q_INVOKABLE void setArchived(const QString& id, bool on);
    Q_INVOKABLE void setTrashed(const QString& id, bool on);
    Q_INVOKABLE void deleteForever(const QString& id);
    Q_INVOKABLE void emptyTrash();
    Q_INVOKABLE void createLabel(const QString& label);
    Q_INVOKABLE void syncNow() { m_notes.sync().syncNow(); }

signals:
    void viewChanged();
    void countsChanged();
    void labelsChanged();
    void syncStateChanged();

private:
    void refresh();

    inline static NotesViewWrap* s_instance = nullptr;
    NotesWrap& m_notes;
    std::vector<std::string> m_ids;
    int m_pinnedCount = 0;
    int m_section = Notes;
    QString m_label;
    QString m_search;
};
