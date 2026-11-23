import QtQuick
import QtQuick.Controls.Basic

// The notes of the chosen section (Notes, a label, Archive, Trash): pinned ones first, then the others.
Flickable {
    id: view
    signal openNote(string noteId, bool isNew)

    readonly property int section: NotesViewWrap.section
    readonly property bool canAdd: NotesViewWrap.canAdd
    readonly property bool empty: NotesViewWrap.count === 0

    function newNote(checklist) {
        view.openNote(NotesViewWrap.newNote(checklist), true)
    }

    clip: true
    contentWidth: width
    contentHeight: page.implicitHeight + 2 * Theme.margin
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar { }

    Column {
        id: page
        x: Theme.margin
        y: Theme.margin
        width: view.width - 2 * Theme.margin
        spacing: Theme.gap

        Rectangle {
            visible: view.canAdd && NotesViewWrap.search === ""
            width: Math.min(Theme.noteMaxWidth, parent.width)
            height: Theme.newNoteHeight
            anchors.horizontalCenter: parent.horizontalCenter
            radius: Theme.radius
            color: Theme.noteColors[0]
            border.color: Theme.cardBorder
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.IBeamCursor
                onClicked: view.newNote(false)
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.space7
                anchors.verticalCenter: parent.verticalCenter
                text: "Take a note…"
                color: Theme.textMuted
                font.pixelSize: Theme.headingSize
            }
            IconButton {
                anchors.right: parent.right
                anchors.rightMargin: Theme.space3
                anchors.verticalCenter: parent.verticalCenter
                icon: Icons.checklist
                tip: "New list"
                onClicked: view.newNote(true)
            }
        }
        Item { visible: view.canAdd && NotesViewWrap.search === ""; width: Theme.line; height: Theme.gap }

        Row {
            visible: view.section === NotesViewWrap.Trash && !view.empty
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.gap
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Notes stay in the Trash until you delete them."
                color: Theme.textMuted
                font.pixelSize: Theme.bodySize
                font.italic: true
            }
            ThemedButton {
                text: "Empty Trash"
                onClicked: NotesViewWrap.emptyTrash()
            }
        }

        MasonryGrid {
            width: parent.width
            height: implicitHeight
            onOpenNote: (noteId) => view.openNote(noteId, false)
        }

        Column {
            visible: view.empty
            anchors.horizontalCenter: parent.horizontalCenter
            topPadding: Theme.emptyTop
            spacing: Theme.space7
            Glyph {
                anchors.horizontalCenter: parent.horizontalCenter
                size: Theme.emptyIcon
                color: Theme.textDim
                path: NotesViewWrap.search !== "" ? Icons.search
                    : view.section === NotesViewWrap.Archive ? Icons.archive
                    : view.section === NotesViewWrap.Trash ? Icons.trash
                    : view.section === NotesViewWrap.Label ? Icons.label : Icons.bulb
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: NotesViewWrap.search !== "" ? "No matching notes"
                    : view.section === NotesViewWrap.Archive ? "Your archived notes appear here"
                    : view.section === NotesViewWrap.Trash ? "No notes in Trash"
                    : view.section === NotesViewWrap.Label ? "No notes with this label yet"
                    : "Notes you add appear here"
                color: Theme.textMuted
                font.pixelSize: Theme.titleSize
            }
        }
    }
}
