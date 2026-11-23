#include "notesWrap.h"

#include <QDateTime>
#include <QVariantMap>

NotesWrap::NotesWrap(const QString& dataFolder, const crypto::Key& key, const QString& syncAddress,
                     const QString& syncToken, QObject* parent)
    : QObject(parent),
      m_library(std::filesystem::path(dataFolder.toStdU16String()), key, [] { return QDateTime::currentMSecsSinceEpoch(); }),
      m_sync(m_library, m_http, m_scheduler, std::filesystem::path((dataFolder + "/sync.json").toStdU16String())),
      m_syncAddress(syncAddress)
{
    m_library.list().onChanged = [this] { emit changed(); };
    m_sync.onStatusChanged = [this] { emit syncChanged(); };
    m_sync.start(syncAddress.toStdString(), syncToken.toStdString());
}

QVariantList NotesWrap::toVariant(const std::vector<ChecklistItem>& items)
{
    QVariantList list;
    for (const auto& item : items)
        list.append(QVariantMap{{"text", QString::fromStdString(item.text)}, {"done", item.done}});
    return list;
}

QStringList NotesWrap::toStringList(const std::vector<std::string>& texts)
{
    QStringList list;
    for (const auto& text : texts) list.append(QString::fromStdString(text));
    return list;
}
