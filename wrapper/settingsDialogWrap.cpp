#include "settingsDialogWrap.h"

SettingsDialogWrap::SettingsDialogWrap(NotesWrap& notes, const QString& settingsFile, QObject* parent)
    : QObject(parent), m_notes(notes), m_file(settingsFile.toStdU16String()), m_settings(appsettings::load(m_file))
{
    s_instance = this;
    connect(&notes, &NotesWrap::changed, this, &SettingsDialogWrap::saveFailedChanged);
    connect(&notes, &NotesWrap::syncChanged, this, &SettingsDialogWrap::syncChanged);
}

SettingsDialogWrap* SettingsDialogWrap::create(QQmlEngine*, QJSEngine*)
{
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void SettingsDialogWrap::setDarkTheme(bool on)
{
    if (on == m_settings.darkTheme) return;
    m_settings.darkTheme = on;
    appsettings::save(m_file, m_settings);
    emit darkThemeChanged();
}
