#include "wrapper/common/notesWrap.h"
#include "wrapper/noteEditor/noteEditorWrap.h"
#include "wrapper/notesView/notesViewWrap.h"
#include "wrapper/settingsDialogWrap.h"

#include "common/crypto.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QStandardPaths>

int main(int argc, char* argv[])
{
#ifdef Q_OS_ANDROID
    // The controls' style the desktop uses; Android's default (Material) moves a field's placeholder.
    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");
#endif
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("QuantumChaos");
    QCoreApplication::setApplicationName("QuantumChaos");
    QCoreApplication::setApplicationVersion(QUANTUMCHAOS_VERSION);
#ifndef Q_OS_ANDROID
    // The desktop matches the window to its launcher entry and icon by this id.
    QGuiApplication::setDesktopFileName("io.github.chandandasgit.QuantumChaos");
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/icon.svg")));
#endif

#ifdef Q_OS_ANDROID
    // The app's private folder; Android's shared storage would be readable by other apps.
    const QString dataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    // ~/.local/share/QuantumChaos (or wherever XDG_DATA_HOME points), %LOCALAPPDATA%\QuantumChaos on Windows.
    const QString dataFolder = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/QuantumChaos";
#endif
    auto key = crypto::keyFromHex(QUANTUMCHAOS_NOTE_KEY);
    if (!key) return 1;    // CMake already refuses a build without a valid key

    NotesWrap notes(dataFolder, *key, QStringLiteral(QUANTUMCHAOS_SYNC_SERVER), QStringLiteral(QUANTUMCHAOS_SYNC_TOKEN));
    crypto::wipe(*key);
    NotesViewWrap notesView(notes);
    NoteEditorWrap noteEditor(notes);
    SettingsDialogWrap settings(notes, dataFolder + "/settings.json");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("QuantumChaos", "Main");

    // In front: sync and wait for other devices' changes. Behind (Android may stop the app at any
    // time there): stop waiting; coming back syncs again.
    notes.setActive(app.applicationState() == Qt::ApplicationActive);
    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &notes, [&notes](Qt::ApplicationState state) {
        notes.setActive(state == Qt::ApplicationActive);
    });

    return QGuiApplication::exec();
}
