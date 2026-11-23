import QtQuick

// One note in the grid. Its actions show while the mouse is over it (on a phone, open the note instead).
Rectangle {
    id: card
    required property string noteId
    required property string title
    required property string body
    required property bool checklist
    required property var previewItems
    required property int doneCount
    required property bool blank
    required property int noteColor
    required property bool coloured
    required property bool pinned
    required property bool archived
    required property bool trashed
    required property var labels
    signal openRequested()

    readonly property int pad: Theme.cardPad
    readonly property bool showActions: !Theme.compact && (hover.hovered || colours.opened)

    implicitHeight: content.implicitHeight + 2 * pad
    radius: Theme.radius
    color: Theme.noteColors[noteColor]
    border.width: Theme.line
    border.color: hover.hovered ? Theme.textDim : (coloured ? color : Theme.cardBorder)

    HoverHandler { id: hover }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: card.openRequested()
    }

    Column {
        id: content
        x: card.pad
        y: card.pad
        width: card.width - 2 * card.pad
        spacing: Theme.space4

        Text {
            visible: card.title !== ""
            width: parent.width - Theme.pinRoom
            text: card.title
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            maximumLineCount: Theme.titleLines
            elide: Text.ElideRight
            color: Theme.text
            font.pixelSize: Theme.headingSize
            font.bold: true
        }
        Text {
            visible: !card.checklist && card.body !== ""
            width: parent.width
            text: card.body
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            maximumLineCount: Theme.bodyLines
            elide: Text.ElideRight
            color: Theme.text
            font.pixelSize: Theme.bodySize
        }
        Column {
            visible: card.checklist
            width: parent.width
            spacing: Theme.space2
            Repeater {
                model: card.previewItems
                delegate: Row {
                    required property var modelData
                    spacing: Theme.space3
                    Glyph { size: Theme.iconSmall; path: Icons.boxEmpty; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        width: content.width - Theme.previewIndent
                        text: modelData.text
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        color: Theme.text
                        font.pixelSize: Theme.bodySize
                    }
                }
            }
            Text {
                visible: card.checklist && card.doneCount > 0
                text: "+ " + card.doneCount + (card.doneCount === 1 ? " checked item" : " checked items")
                color: Theme.textMuted
                font.pixelSize: Theme.smallSize
            }
        }
        Text {
            visible: card.blank
            text: "Empty note"
            color: Theme.textMuted
            font.pixelSize: Theme.bodySize
        }
        Flow {
            visible: card.labels.length > 0
            width: parent.width
            spacing: Theme.space3
            Repeater {
                model: card.labels
                delegate: Rectangle {
                    required property string modelData
                    width: chipText.implicitWidth + Theme.chipPad
                    height: Theme.chipHeight
                    radius: height / 2
                    color: Theme.chipBg
                    Text {
                        id: chipText
                        anchors.centerIn: parent
                        text: modelData
                        color: Theme.text
                        font.pixelSize: Theme.smallSize
                    }
                }
            }
        }
        Row {
            visible: !Theme.compact
            height: Theme.buttonSmall
            spacing: Theme.space1
            opacity: card.showActions ? 1 : 0
            IconButton {
                id: paletteButton
                visible: !card.trashed
                size: Theme.buttonSmall
                icon: Icons.palette
                tip: "Colour"
                onClicked: colours.open()
                ColourPicker {
                    id: colours
                    y: paletteButton.height
                    current: card.noteColor
                    onPicked: (index) => NotesViewWrap.setColor(card.noteId, index)
                }
            }
            IconButton {
                visible: !card.trashed
                size: Theme.buttonSmall
                icon: card.archived ? Icons.unarchive : Icons.archive
                tip: card.archived ? "Unarchive" : "Archive"
                onClicked: Qt.callLater(NotesViewWrap.setArchived, card.noteId, !card.archived)
            }
            IconButton {
                visible: !card.trashed
                size: Theme.buttonSmall
                icon: Icons.trash
                tip: "Delete"
                onClicked: Qt.callLater(NotesViewWrap.setTrashed, card.noteId, true)
            }
            IconButton {
                visible: card.trashed
                size: Theme.buttonSmall
                icon: Icons.deleteForever
                tip: "Delete forever"
                onClicked: Qt.callLater(NotesViewWrap.deleteForever, card.noteId)
            }
            IconButton {
                visible: card.trashed
                size: Theme.buttonSmall
                icon: Icons.restore
                tip: "Restore"
                onClicked: Qt.callLater(NotesViewWrap.setTrashed, card.noteId, false)
            }
        }
    }

    IconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Theme.space2
        visible: !card.trashed && (card.showActions || (Theme.compact && card.pinned))
        size: Theme.buttonSmall
        icon: Icons.pin
        filled: card.pinned
        color: card.pinned ? Theme.text : Theme.iconDefault
        tip: card.pinned ? "Unpin" : "Pin"
        onClicked: Qt.callLater(NotesViewWrap.setPinned, card.noteId, !card.pinned)
    }
}
