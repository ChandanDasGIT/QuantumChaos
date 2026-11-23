#include "notesViewWrap.h"

#include "notesView/masonry.h"

NotesViewWrap::NotesViewWrap(NotesWrap& notes, QObject* parent) : QAbstractListModel(parent), m_notes(notes)
{
    s_instance = this;
    connect(&notes, &NotesWrap::changed, this, &NotesViewWrap::refresh);
    connect(&notes, &NotesWrap::syncChanged, this, &NotesViewWrap::syncStateChanged);
    refresh();
}

NotesViewWrap* NotesViewWrap::create(QQmlEngine*, QJSEngine*)
{
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

int NotesViewWrap::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant NotesViewWrap::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= count()) return {};
    const Note* n = m_notes.list().find(m_ids[index.row()]);
    if (!n) return {};
    switch (role) {
    case IdRole: return QString::fromStdString(n->id);
    case TitleRole: return QString::fromStdString(n->title);
    case BodyRole: return QString::fromStdString(n->body);
    case ChecklistRole: return n->checklist;
    case PreviewItemsRole: return NotesWrap::toVariant(n->openItems(Note::cardPreviewLines));
    case DoneCountRole: return n->doneCount();
    case BlankRole: return n->isEmpty();
    case ColorRole: return n->color;
    case ColouredRole: return n->coloured();
    case PinnedRole: return n->pinned;
    case ArchivedRole: return n->archived;
    case TrashedRole: return n->trashed;
    case LabelsRole: return NotesWrap::toStringList(n->labels);
    }
    return {};
}

QHash<int, QByteArray> NotesViewWrap::roleNames() const
{
    return {{IdRole, "noteId"}, {TitleRole, "title"}, {BodyRole, "body"}, {ChecklistRole, "checklist"},
            {PreviewItemsRole, "previewItems"}, {DoneCountRole, "doneCount"}, {BlankRole, "blank"},
            {ColorRole, "noteColor"}, {ColouredRole, "coloured"}, {PinnedRole, "pinned"}, {ArchivedRole, "archived"},
            {TrashedRole, "trashed"}, {LabelsRole, "labels"}};
}

void NotesViewWrap::setSection(int section)
{
    if (section == m_section) return;
    m_section = section;
    emit viewChanged();
    refresh();
}

void NotesViewWrap::setLabel(const QString& label)
{
    if (label == m_label) return;
    m_label = label;
    emit viewChanged();
    refresh();
}

void NotesViewWrap::setSearch(const QString& search)
{
    if (search == m_search) return;
    m_search = search;
    emit viewChanged();
    refresh();
}

bool NotesViewWrap::canAdd() const
{
    return notes::NoteList::canCreateIn(static_cast<notes::Section>(m_section));
}

QStringList NotesViewWrap::labels() const
{
    return NotesWrap::toStringList(m_notes.list().labels());
}

void NotesViewWrap::refresh()
{
    const auto section = static_cast<notes::Section>(m_section);
    const std::string label = m_label.toStdString();
    const std::string search = m_search.toStdString();
    std::vector<std::string> ids = m_notes.list().query(section, label, search, true);
    const int pinned = static_cast<int>(ids.size());
    const auto others = m_notes.list().query(section, label, search, false);
    ids.insert(ids.end(), others.begin(), others.end());

    // Same cards in the same order: they are only refreshed, so they keep their state.
    if (ids == m_ids) {
        if (!m_ids.empty()) emit dataChanged(index(0), index(count() - 1));
    } else {
        beginResetModel();
        m_ids = std::move(ids);
        m_pinnedCount = pinned;
        endResetModel();
        emit countsChanged();
    }
    emit labelsChanged();
}

QVariantMap NotesViewWrap::grid(double width, double cardWidth, double gap, bool compact) const
{
    const auto g = masonry::gridFor(width, cardWidth, gap, compact);
    return {{"columns", g.columns}, {"columnWidth", g.columnWidth}, {"leftEdge", g.leftEdge}};
}

QVariantMap NotesViewWrap::layout(const QVariantList& heights, double width, double cardWidth, double gap,
                                  double labelHeight, bool compact) const
{
    std::vector<double> h;
    for (const auto& v : heights) h.push_back(v.toDouble());
    const auto l = masonry::layout(h, m_pinnedCount, width, cardWidth, gap, labelHeight, compact);
    QVariantList x, y;
    for (double v : l.x) x.append(v);
    for (double v : l.y) y.append(v);
    return {{"x", x}, {"y", y}, {"pinnedLabelY", l.pinnedLabelY}, {"othersLabelY", l.othersLabelY},
            {"height", l.height}};
}

QString NotesViewWrap::newNote(bool checklist)
{
    return QString::fromStdString(
        m_notes.list().createIn(static_cast<notes::Section>(m_section), m_label.toStdString(), checklist));
}

void NotesViewWrap::setPinned(const QString& id, bool on) { m_notes.list().setPinned(id.toStdString(), on); }
void NotesViewWrap::setColor(const QString& id, int color) { m_notes.list().setColor(id.toStdString(), color); }
void NotesViewWrap::setArchived(const QString& id, bool on) { m_notes.list().setArchived(id.toStdString(), on); }
void NotesViewWrap::setTrashed(const QString& id, bool on) { m_notes.list().setTrashed(id.toStdString(), on); }
void NotesViewWrap::deleteForever(const QString& id) { m_notes.list().erase(id.toStdString()); }
void NotesViewWrap::emptyTrash() { m_notes.list().emptyTrash(); }
void NotesViewWrap::createLabel(const QString& label) { m_notes.list().createLabel(label.toStdString()); }
