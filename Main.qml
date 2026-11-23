import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

ApplicationWindow {
    id: root
    width: Theme.windowWidth
    height: Theme.windowHeight
    minimumWidth: Theme.windowMinWidth
    minimumHeight: Theme.windowMinHeight
    visible: true
    title: "QuantumChaos"
    color: Theme.windowBg

    onWidthChanged: Theme.compact = width < Theme.compactBelow

    // Phones: the app fills the whole screen, its own colours behind the status and navigation
    // bars; everything else keeps clear of them with the safe-area margins (0 on the desktop).
    flags: Theme.touchOnly ? Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint : Qt.Window
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    Binding { target: Theme; property: "safeTop"; value: root.contentItem.SafeArea.margins.top }
    Binding { target: Theme; property: "safeBottom"; value: root.contentItem.SafeArea.margins.bottom }
    Binding { target: Theme; property: "safeLeft"; value: root.contentItem.SafeArea.margins.left }
    Binding { target: Theme; property: "safeRight"; value: root.contentItem.SafeArea.margins.right }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.safeLeft
        anchors.rightMargin: Theme.safeRight
        spacing: 0

        // Behind the status bar: the top bar's colour.
        Rectangle {
            visible: Theme.safeTop > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.safeTop
            color: Theme.barBg
        }
        TopBar {
            Layout.fillWidth: true
            onMenuClicked: Theme.compact ? drawer.open() : (side.expanded = !side.expanded)
            onSettingsClicked: settings.open()
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            SideMenu {
                id: side
                visible: !Theme.compact
                Layout.fillHeight: true
                Layout.preferredWidth: implicitWidth
            }
            NotesView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                onOpenNote: (noteId, isNew) => editor.openNote(noteId, isNew)
            }
        }
        // Behind the navigation bar: the window's background.
        Item {
            visible: Theme.safeBottom > 0
            Layout.preferredHeight: Theme.safeBottom
        }
    }

    Drawer {
        id: drawer
        width: Theme.sideWidth
        height: root.height
        edge: Qt.LeftEdge
        background: Rectangle { color: Theme.windowBg }
        SideMenu {
            anchors.fill: parent
            anchors.topMargin: Theme.safeTop
            anchors.bottomMargin: Theme.safeBottom
            anchors.leftMargin: Theme.safeLeft
            expanded: true
            onPicked: drawer.close()
        }
    }

    NoteEditor { id: editor }
    SettingsDialog { id: settings }
}
