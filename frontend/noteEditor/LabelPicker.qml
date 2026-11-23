import QtQuick
import QtQuick.Controls.Basic

// Ticks labels on and off for the open note; typing a new name creates it.
Popup {
    id: picker
    padding: Theme.space5
    width: Theme.pickerWidth

    onOpened: {
        nameField.text = ""
        nameField.forceActiveFocus()
    }

    background: Rectangle {
        color: Theme.dialogBg
        radius: Theme.radius
        border.color: Theme.fieldBorder
    }
    contentItem: Column {
        spacing: Theme.space3
        Text {
            text: "Label note"
            color: Theme.text
            font.pixelSize: Theme.bodySize
            font.bold: true
        }
        ThemedTextField {
            id: nameField
            width: parent.width
            placeholderText: "Enter label name"
            onTextChanged: NoteEditorWrap.labelFilter = text
            onAccepted: {
                NoteEditorWrap.addTypedLabel()
                text = ""
            }
        }
        Repeater {
            model: NoteEditorWrap.labelChoices
            delegate: Rectangle {
                required property var modelData
                width: picker.availableWidth
                height: Theme.rowHeight
                radius: Theme.radiusSmall
                color: area.containsMouse ? Theme.hoverBg : "transparent"
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.space2
                    spacing: Theme.space4
                    Glyph { size: Theme.iconMedium; path: modelData.on ? Icons.boxChecked : Icons.boxEmpty; color: Theme.text }
                    Text { text: modelData.name; color: Theme.text; font.pixelSize: Theme.bodySize }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: NoteEditorWrap.toggleLabel(modelData.name)
                }
            }
        }
        Rectangle {
            visible: NoteEditorWrap.canCreateLabel
            width: picker.availableWidth
            height: Theme.rowHeight
            radius: Theme.radiusSmall
            color: createArea.containsMouse ? Theme.hoverBg : "transparent"
            Row {
                anchors.verticalCenter: parent.verticalCenter
                x: Theme.space2
                spacing: Theme.space4
                Glyph { size: Theme.iconMedium; path: Icons.plus; color: Theme.text }
                Text { text: "Create \"" + NoteEditorWrap.newLabelName + "\""; color: Theme.text; font.pixelSize: Theme.bodySize }
            }
            MouseArea {
                id: createArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: nameField.accepted()
            }
        }
    }
}
