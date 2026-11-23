#include "noteEditorWrap.h"

#include "noteEditor/textediting.h"

namespace {

// QML counts positions in UTF-16 characters; the backend works on UTF-8 bytes.
std::size_t toByte(const QString& text, int position)
{
    return static_cast<std::size_t>(text.left(position).toUtf8().size());
}

int toPosition(const std::string& utf8, std::size_t byte)
{
    return static_cast<int>(QString::fromUtf8(utf8.data(), static_cast<qsizetype>(byte)).size());
}

} // namespace

NoteEditorWrap::NoteEditorWrap(NotesWrap& notes, QObject* parent) : QObject(parent), m_notes(notes)
{
    s_instance = this;
    connect(&notes, &NotesWrap::changed, this, &NoteEditorWrap::noteChanged);
    connect(&notes, &NotesWrap::changed, this, &NoteEditorWrap::labelChoicesChanged);
    connect(&notes, &NotesWrap::remoteChanged, this, [this](const QString& id, bool title, bool content) {
        if (id.toStdString() != m_id) return;
        if (title) emit titleReplaced();
        if (content) {
            emit bodyReplaced();
            emit itemsChanged();
        }
    });
    connect(&notes, &NotesWrap::remoteRemoved, this, [this](const QString& id) {
        if (id.toStdString() == m_id) emit noteRemoved();
    });
}

NoteEditorWrap* NoteEditorWrap::create(QQmlEngine*, QJSEngine*)
{
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

const Note* NoteEditorWrap::note() const
{
    return m_id.empty() ? nullptr : m_notes.list().find(m_id);
}

QString NoteEditorWrap::title() const { const Note* n = note(); return n ? QString::fromStdString(n->title) : QString(); }
QString NoteEditorWrap::body() const { const Note* n = note(); return n ? QString::fromStdString(n->body) : QString(); }
bool NoteEditorWrap::checklist() const { const Note* n = note(); return n && n->checklist; }
QVariantList NoteEditorWrap::items() const { const Note* n = note(); return n ? NotesWrap::toVariant(n->items) : QVariantList(); }
int NoteEditorWrap::noteColor() const { const Note* n = note(); return n ? n->color : 0; }
bool NoteEditorWrap::coloured() const { const Note* n = note(); return n && n->coloured(); }
bool NoteEditorWrap::pinned() const { const Note* n = note(); return n && n->pinned; }
bool NoteEditorWrap::archived() const { const Note* n = note(); return n && n->archived; }
bool NoteEditorWrap::trashed() const { const Note* n = note(); return n && n->trashed; }
bool NoteEditorWrap::editable() const { const Note* n = note(); return n && n->editable(); }
int NoteEditorWrap::doneCount() const { const Note* n = note(); return n ? n->doneCount() : 0; }
QStringList NoteEditorWrap::labels() const { const Note* n = note(); return n ? NotesWrap::toStringList(n->labels) : QStringList(); }
qint64 NoteEditorWrap::modified() const { const Note* n = note(); return n ? n->modified : 0; }

void NoteEditorWrap::setLabelFilter(const QString& text)
{
    if (text == m_labelFilter) return;
    m_labelFilter = text;
    emit labelChoicesChanged();
}

QVariantList NoteEditorWrap::labelChoices() const
{
    QVariantList list;
    for (const auto& choice : m_notes.list().labelChoices(m_id, m_labelFilter.toStdString()))
        list.append(QVariantMap{{"name", QString::fromStdString(choice.name)}, {"on", choice.on}});
    return list;
}

bool NoteEditorWrap::canCreateLabel() const
{
    return m_notes.list().canCreateLabel(m_labelFilter.toStdString());
}

QString NoteEditorWrap::newLabelName() const
{
    return QString::fromStdString(notes::NoteList::labelName(m_labelFilter.toStdString()));
}

void NoteEditorWrap::open(const QString& id)
{
    m_id = id.toStdString();
    emit opened();
    emit noteChanged();
    emit itemsChanged();
    emit labelChoicesChanged();
}

void NoteEditorWrap::close()
{
    if (m_id.empty()) return;
    const std::string id = m_id;
    m_id.clear();
    m_notes.list().discardIfEmpty(id);
}

void NoteEditorWrap::setTitle(const QString& text) { m_notes.list().setTitle(m_id, text.toStdString()); }
void NoteEditorWrap::setBody(const QString& text) { m_notes.list().setBody(m_id, text.toStdString()); }

void NoteEditorWrap::setChecklist(bool on)
{
    m_notes.list().setChecklist(m_id, on);
    emit itemsChanged();
    emit bodyReplaced();
}

void NoteEditorWrap::setItemText(int index, const QString& text)
{
    m_notes.list().setItemText(m_id, index, text.toStdString());
}

void NoteEditorWrap::setItemDone(int index, bool done)
{
    m_notes.list().setItemDone(m_id, index, done);
    emit itemsChanged();
}

int NoteEditorWrap::splitItem(int index, int cursor)
{
    const Note* n = note();
    if (!n || index < 0 || index >= static_cast<int>(n->items.size())) return -1;
    const std::size_t at = toByte(QString::fromStdString(n->items[index].text), cursor);
    const int added = m_notes.list().splitItem(m_id, index, at);
    emit itemsChanged();
    return added;
}

int NoteEditorWrap::addItem(const QString& text)
{
    const Note* n = note();
    if (!n) return -1;
    const int added = m_notes.list().insertItem(m_id, static_cast<int>(n->items.size()), text.toStdString());
    emit itemsChanged();
    return added;
}

void NoteEditorWrap::removeItem(int index)
{
    m_notes.list().removeItem(m_id, index);
    emit itemsChanged();
}

int NoteEditorWrap::firstItemOrNew()
{
    const int index = m_notes.list().firstItemOrNew(m_id);
    emit itemsChanged();
    return index;
}

void NoteEditorWrap::setColor(int color) { m_notes.list().setColor(m_id, color); }
void NoteEditorWrap::setPinned(bool on) { m_notes.list().setPinned(m_id, on); }
void NoteEditorWrap::setArchived(bool on) { m_notes.list().setArchived(m_id, on); }
void NoteEditorWrap::setTrashed(bool on) { m_notes.list().setTrashed(m_id, on); }
void NoteEditorWrap::toggleLabel(const QString& label) { m_notes.list().toggleLabel(m_id, label.toStdString()); }

void NoteEditorWrap::addTypedLabel()
{
    m_notes.list().addTypedLabel(m_id, m_labelFilter.toStdString());
    setLabelFilter({});
}

void NoteEditorWrap::deleteForever()
{
    const std::string id = m_id;
    m_id.clear();
    m_notes.list().erase(id);
}

QVariantMap NoteEditorWrap::continueList(const QString& text, int cursor) const
{
    const std::string utf8 = text.toStdString();
    const auto edit = textediting::onEnterIn(utf8, toByte(text, cursor));
    if (!edit.handled) return {{"handled", false}};
    const std::string edited = utf8.substr(0, edit.removeFrom) + edit.insert;
    return {{"handled", true}, {"removeFrom", toPosition(utf8, edit.removeFrom)},
            {"removeTo", toPosition(utf8, edit.removeTo)}, {"insert", QString::fromStdString(edit.insert)},
            {"cursor", toPosition(edited, edit.cursorAfter)}};
}

double NoteEditorWrap::scrollFor(double top, double height, double viewTop, double viewHeight) const
{
    return textediting::scrollToShow(top, height, viewTop, viewHeight);
}
