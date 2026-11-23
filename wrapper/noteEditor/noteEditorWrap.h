#pragma once

#include "notesWrap.h"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// The note open in the editor. Every change goes into the note at once: there is no save step.
class NoteEditorWrap : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // Title and text are read when the note opens; while typing, the editor's fields hold them.
    Q_PROPERTY(QString noteId READ noteId NOTIFY opened)
    Q_PROPERTY(QString title READ title NOTIFY opened)
    Q_PROPERTY(QString body READ body NOTIFY opened)
    Q_PROPERTY(bool checklist READ checklist NOTIFY noteChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(int noteColor READ noteColor NOTIFY noteChanged)
    Q_PROPERTY(bool coloured READ coloured NOTIFY noteChanged)
    Q_PROPERTY(bool pinned READ pinned NOTIFY noteChanged)
    Q_PROPERTY(bool archived READ archived NOTIFY noteChanged)
    Q_PROPERTY(bool trashed READ trashed NOTIFY noteChanged)
    Q_PROPERTY(bool editable READ editable NOTIFY noteChanged)
    Q_PROPERTY(int doneCount READ doneCount NOTIFY itemsChanged)
    Q_PROPERTY(QStringList labels READ labels NOTIFY noteChanged)
    Q_PROPERTY(qint64 modified READ modified NOTIFY noteChanged)
    // The label picker: what is typed in it, the labels matching it ({name, on}), and whether it is new.
    Q_PROPERTY(QString labelFilter READ labelFilter WRITE setLabelFilter NOTIFY labelChoicesChanged)
    Q_PROPERTY(QVariantList labelChoices READ labelChoices NOTIFY labelChoicesChanged)
    Q_PROPERTY(bool canCreateLabel READ canCreateLabel NOTIFY labelChoicesChanged)
    Q_PROPERTY(QString newLabelName READ newLabelName NOTIFY labelChoicesChanged)

public:
    explicit NoteEditorWrap(NotesWrap& notes, QObject* parent = nullptr);
    static NoteEditorWrap* create(QQmlEngine*, QJSEngine*);

    QString noteId() const { return QString::fromStdString(m_id); }
    QString title() const;
    QString body() const;
    bool checklist() const;
    QVariantList items() const;
    int noteColor() const;
    bool coloured() const;
    bool pinned() const;
    bool archived() const;
    bool trashed() const;
    bool editable() const;
    int doneCount() const;
    QStringList labels() const;
    qint64 modified() const;
    QString labelFilter() const { return m_labelFilter; }
    void setLabelFilter(const QString& text);
    QVariantList labelChoices() const;
    bool canCreateLabel() const;
    QString newLabelName() const;

    Q_INVOKABLE void open(const QString& id);
    // Closing a note with nothing written in it removes it.
    Q_INVOKABLE void close();

    Q_INVOKABLE void setTitle(const QString& text);
    Q_INVOKABLE void setBody(const QString& text);
    Q_INVOKABLE void setChecklist(bool on);
    Q_INVOKABLE void setItemText(int index, const QString& text);
    Q_INVOKABLE void setItemDone(int index, bool done);
    // Enter in a checklist line: the text after the cursor becomes the next line. Returns its index.
    Q_INVOKABLE int splitItem(int index, int cursor);
    Q_INVOKABLE int addItem(const QString& text);
    Q_INVOKABLE void removeItem(int index);
    // Enter in the title of a checklist: the first line, made if there is none. Returns its index.
    Q_INVOKABLE int firstItemOrNew();
    Q_INVOKABLE void setColor(int color);
    Q_INVOKABLE void setPinned(bool on);
    Q_INVOKABLE void setArchived(bool on);
    Q_INVOKABLE void setTrashed(bool on);
    Q_INVOKABLE void deleteForever();
    Q_INVOKABLE void toggleLabel(const QString& label);
    // Makes the label typed in the picker and puts it on the note.
    Q_INVOKABLE void addTypedLabel();

    // Enter in the note's text. Returns {handled, removeFrom, removeTo, insert, cursor}: when handled,
    // the editor replaces removeFrom..removeTo with insert and puts the cursor at `cursor`, instead
    // of adding a plain new line.
    Q_INVOKABLE QVariantMap continueList(const QString& text, int cursor) const;
    // Where to scroll so the cursor's line [top, top + height] shows in [viewTop, viewTop + viewHeight].
    Q_INVOKABLE double scrollFor(double top, double height, double viewTop, double viewHeight) const;

signals:
    void opened();
    void noteChanged();
    void itemsChanged();
    // The editor reads the title / the text again: another device changed it, or the text became
    // a checklist or back.
    void titleReplaced();
    void bodyReplaced();
    // The open note was deleted for good on another device.
    void noteRemoved();
    void labelChoicesChanged();

private:
    const Note* note() const;

    inline static NoteEditorWrap* s_instance = nullptr;
    NotesWrap& m_notes;
    std::string m_id;
    QString m_labelFilter;
};
