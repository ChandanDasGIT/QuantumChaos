import QtQuick
import QtQuick.Controls.Basic

// Notes, the labels, Archive and Trash. Folded, it shows only the icons.
Rectangle {
    id: menu
    property bool expanded: true
    signal picked()

    implicitWidth: expanded ? Theme.sideWidth : Theme.sideFolded
    color: Theme.windowBg

    function show(section, label) {
        NotesViewWrap.label = label
        NotesViewWrap.section = section
        menu.picked()
    }

    component Entry: Rectangle {
        id: entry
        property string icon
        property string text
        property bool selected: false
        signal clicked()
        x: Theme.space4
        width: menu.expanded ? menu.width - 2 * Theme.space4 : Theme.entryFolded
        height: Theme.entryHeight
        radius: height / 2
        color: selected ? Theme.selectedBg : (area.containsMouse ? Theme.hoverBg : "transparent")
        Glyph {
            x: (Theme.entryFolded - Theme.iconSize) / 2
            anchors.verticalCenter: parent.verticalCenter
            path: entry.icon
            color: entry.selected ? Theme.text : Theme.iconDefault
        }
        Text {
            visible: menu.expanded
            x: Theme.entryTextX
            width: entry.width - Theme.entryTextX - Theme.space4
            anchors.verticalCenter: parent.verticalCenter
            text: entry.text
            elide: Text.ElideRight
            color: Theme.text
            font.pixelSize: Theme.bodySize
            font.bold: entry.selected
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: entry.clicked()
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: entries.implicitHeight + 2 * Theme.space4
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: entries
            y: Theme.space4
            width: menu.width
            spacing: Theme.space1

            Entry {
                icon: Icons.bulb
                text: "Notes"
                selected: NotesViewWrap.section === NotesViewWrap.Notes
                onClicked: menu.show(NotesViewWrap.Notes, "")
            }
            Repeater {
                model: NotesViewWrap.labels
                delegate: Entry {
                    required property string modelData
                    icon: Icons.label
                    text: modelData
                    selected: NotesViewWrap.section === NotesViewWrap.Label && NotesViewWrap.label === modelData
                    onClicked: menu.show(NotesViewWrap.Label, modelData)
                }
            }
            Entry {
                icon: Icons.plus
                text: "New label"
                onClicked: {
                    menu.expanded = true
                    newLabel.visible = true
                    newLabel.forceActiveFocus()
                }
            }
            ThemedTextField {
                id: newLabel
                visible: false
                x: Theme.space7
                width: menu.width - 2 * Theme.space7
                placeholderText: "Label name"
                onAccepted: {
                    NotesViewWrap.createLabel(text)
                    text = ""
                    visible = false
                }
                onActiveFocusChanged: if (!activeFocus) { text = ""; visible = false }
                Keys.onEscapePressed: { text = ""; visible = false }
            }
            Entry {
                icon: Icons.archive
                text: "Archive"
                selected: NotesViewWrap.section === NotesViewWrap.Archive
                onClicked: menu.show(NotesViewWrap.Archive, "")
            }
            Entry {
                icon: Icons.trash
                text: "Trash"
                selected: NotesViewWrap.section === NotesViewWrap.Trash
                onClicked: menu.show(NotesViewWrap.Trash, "")
            }
        }
    }
}
