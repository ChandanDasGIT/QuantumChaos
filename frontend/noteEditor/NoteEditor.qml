import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// A note opened for editing. Every change is kept as you type (there is no save button);
// closing a note with nothing written in it drops it.
Popup {
    id: editor
    property bool loading: false
    property int focusIndex: -1
    property bool focusAtEnd: false
    property bool showDone: true
    property Item lastField: null
    readonly property bool readOnly: !NoteEditorWrap.editable
    readonly property int doneCount: NoteEditorWrap.doneCount

    function openNote(noteId, isNew) {
        loading = true
        NoteEditorWrap.open(noteId)
        titleField.text = NoteEditorWrap.title
        bodyField.text = NoteEditorWrap.body
        loading = false
        lastField = null
        scroller.contentY = 0
        open()
        if (isNew) {
            if (NoteEditorWrap.checklist) {
                focusAtEnd = true
                focusIndex = 0
            } else {
                bodyField.forceActiveFocus()
            }
        }
    }

    // Keeps the line with the cursor in view while typing.
    function ensureVisible(field) {
        const r = field.cursorRectangle
        const p = field.mapToItem(fields, r.x, r.y)
        scroller.contentY = NoteEditorWrap.scrollFor(p.y, r.height, scroller.contentY, scroller.height)
    }

    function closeAfter(action) {
        action()
        close()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Theme.compact ? parent.width : Math.min(Theme.noteMaxWidth, parent.width - 2 * Theme.margin)
    height: Theme.compact ? parent.height : Math.min(implicitHeight, parent.height - 2 * Theme.margin)
    padding: 0
    // A phone: the editor fills the screen, its colour behind the system bars, its content between them.
    topPadding: Theme.compact ? Theme.safeTop : 0
    bottomPadding: Theme.compact ? Theme.safeBottom : 0
    leftPadding: Theme.compact ? Theme.safeLeft : 0
    rightPadding: Theme.compact ? Theme.safeRight : 0
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onClosed: NoteEditorWrap.close()

    Overlay.modal: Rectangle { color: Theme.scrim }

    background: Rectangle {
        color: Theme.noteColors[NoteEditorWrap.noteColor]
        radius: Theme.compact ? 0 : Theme.radius
        border.width: NoteEditorWrap.coloured ? 0 : Theme.line
        border.color: Theme.cardBorder
    }

    // The note changed on another device while it is open: read it again, the cursor staying where it was.
    Connections {
        target: NoteEditorWrap
        function onTitleReplaced() {
            const cursor = titleField.cursorPosition
            editor.loading = true
            titleField.text = NoteEditorWrap.title
            editor.loading = false
            titleField.cursorPosition = cursor
        }
        function onBodyReplaced() {
            const cursor = bodyField.cursorPosition
            editor.loading = true
            bodyField.text = NoteEditorWrap.body
            editor.loading = false
            bodyField.cursorPosition = cursor
        }
        function onNoteRemoved() {
            editor.close()
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        Flickable {
            id: scroller
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: fields.implicitHeight
            contentWidth: width
            contentHeight: fields.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            Column {
                id: fields
                width: scroller.width
                topPadding: Theme.gap
                bottomPadding: Theme.space2
                leftPadding: Theme.space7
                rightPadding: Theme.space4
                spacing: Theme.space2
                readonly property real lineWidth: width - leftPadding - rightPadding

                RowLayout {
                    width: fields.lineWidth
                    TextArea {
                        id: titleField
                        Layout.fillWidth: true
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.Wrap
                        readOnly: editor.readOnly
                        placeholderText: "Title"
                        placeholderTextColor: Theme.textDim
                        color: Theme.text
                        font.pixelSize: Theme.titleSize
                        selectionColor: Theme.selectionBg
                        selectedTextColor: Theme.text
                        leftPadding: 0
                        background: null
                        onTextChanged: if (!editor.loading) NoteEditorWrap.setTitle(text)
                        onActiveFocusChanged: if (activeFocus) editor.lastField = titleField
                        onCursorRectangleChanged: if (activeFocus) editor.ensureVisible(titleField)
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                if (NoteEditorWrap.checklist) {
                                    editor.focusAtEnd = false
                                    editor.focusIndex = NoteEditorWrap.firstItemOrNew()
                                } else {
                                    bodyField.forceActiveFocus()
                                }
                                event.accepted = true
                            }
                        }
                    }
                    IconButton {
                        Layout.alignment: Qt.AlignTop
                        visible: !editor.readOnly
                        icon: Icons.pin
                        filled: NoteEditorWrap.pinned
                        color: NoteEditorWrap.pinned ? Theme.text : Theme.iconDefault
                        tip: NoteEditorWrap.pinned ? "Unpin" : "Pin"
                        onClicked: NoteEditorWrap.setPinned(!NoteEditorWrap.pinned)
                    }
                }

                TextArea {
                    id: bodyField
                    visible: !NoteEditorWrap.checklist
                    width: fields.lineWidth
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.Wrap
                    readOnly: editor.readOnly
                    placeholderText: "Take a note…"
                    placeholderTextColor: Theme.textDim
                    color: Theme.text
                    font.pixelSize: Theme.editorSize
                    selectionColor: Theme.selectionBg
                    selectedTextColor: Theme.text
                    leftPadding: 0
                    background: null
                    onTextChanged: if (!editor.loading) NoteEditorWrap.setBody(text)
                    onActiveFocusChanged: if (activeFocus) editor.lastField = bodyField
                    onCursorRectangleChanged: if (activeFocus) editor.ensureVisible(bodyField)
                    Keys.onPressed: (event) => {
                        if ((event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter) || (event.modifiers & Qt.ShiftModifier))
                            return
                        if (selectedText !== "") return
                        const result = NoteEditorWrap.continueList(text, cursorPosition)
                        if (!result.handled) return
                        if (result.removeTo > result.removeFrom) remove(result.removeFrom, result.removeTo)
                        if (result.insert !== "") insert(result.removeFrom, result.insert)
                        cursorPosition = result.cursor
                        event.accepted = true
                    }
                }

                Column {
                    visible: NoteEditorWrap.checklist
                    width: fields.lineWidth
                    spacing: 0

                    Repeater {
                        model: NoteEditorWrap.items
                        delegate: ChecklistRow {
                            width: fields.lineWidth
                            visible: !modelData.done
                            readOnly: editor.readOnly
                            wantsFocus: editor.focusIndex === index
                            focusAtEnd: editor.focusAtEnd
                            onFieldFocused: {
                                editor.lastField = field
                                editor.focusIndex = -1
                            }
                            onToggled: (done) => NoteEditorWrap.setItemDone(index, done)
                            onEdited: (text) => NoteEditorWrap.setItemText(index, text)
                            onSplitRequested: (cursor) => {
                                editor.focusAtEnd = false
                                editor.focusIndex = NoteEditorWrap.splitItem(index, cursor)
                            }
                            onRemoveRequested: {
                                editor.focusAtEnd = true
                                editor.focusIndex = index - 1
                                NoteEditorWrap.removeItem(index)
                            }
                        }
                    }
                    Rectangle {
                        visible: !editor.readOnly
                        width: fields.lineWidth
                        height: Theme.lineHeight
                        color: "transparent"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Theme.space5
                            leftPadding: Theme.space4
                            Glyph { size: Theme.iconMedium; path: Icons.plus; anchors.verticalCenter: parent.verticalCenter }
                            Text {
                                text: "List item"
                                color: Theme.textMuted
                                font.pixelSize: Theme.bodySize
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.IBeamCursor
                            onClicked: {
                                editor.focusAtEnd = true
                                editor.focusIndex = NoteEditorWrap.addItem("")
                            }
                        }
                    }
                    Rectangle {
                        visible: editor.doneCount > 0
                        width: fields.lineWidth
                        height: Theme.lineHeight
                        color: "transparent"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Theme.space3
                            Glyph {
                                size: Theme.iconMedium
                                path: editor.showDone ? Icons.chevronDown : Icons.chevronRight
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: editor.doneCount + (editor.doneCount === 1 ? " checked item" : " checked items")
                                color: Theme.textMuted
                                font.pixelSize: Theme.bodySize
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: editor.showDone = !editor.showDone
                        }
                    }
                    Repeater {
                        model: NoteEditorWrap.items
                        delegate: ChecklistRow {
                            width: fields.lineWidth
                            visible: modelData.done && editor.showDone
                            readOnly: editor.readOnly
                            wantsFocus: editor.focusIndex === index
                            focusAtEnd: editor.focusAtEnd
                            onFieldFocused: {
                                editor.lastField = field
                                editor.focusIndex = -1
                            }
                            onToggled: (done) => NoteEditorWrap.setItemDone(index, done)
                            onEdited: (text) => NoteEditorWrap.setItemText(index, text)
                            onSplitRequested: (cursor) => {
                                editor.focusAtEnd = false
                                editor.focusIndex = NoteEditorWrap.splitItem(index, cursor)
                            }
                            onRemoveRequested: NoteEditorWrap.removeItem(index)
                        }
                    }
                }

                Flow {
                    visible: NoteEditorWrap.labels.length > 0
                    width: fields.lineWidth
                    spacing: Theme.space3
                    topPadding: Theme.space3
                    Repeater {
                        model: NoteEditorWrap.labels
                        delegate: Rectangle {
                            required property string modelData
                            width: chip.implicitWidth + (editor.readOnly ? Theme.chipPad : Theme.chipRemoveRoom)
                            height: Theme.chipHeightLarge
                            radius: height / 2
                            color: Theme.chipBg
                            Text {
                                id: chip
                                x: Theme.space4
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData
                                color: Theme.text
                                font.pixelSize: Theme.smallSize
                            }
                            IconButton {
                                visible: !editor.readOnly
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                size: Theme.buttonTiny
                                icon: Icons.close
                                tip: "Remove label"
                                onClicked: NoteEditorWrap.toggleLabel(modelData)
                            }
                        }
                    }
                }

                Text {
                    width: fields.lineWidth
                    topPadding: Theme.space4
                    horizontalAlignment: Text.AlignRight
                    text: NoteEditorWrap.modified > 0
                          ? "Edited " + Qt.formatDateTime(new Date(NoteEditorWrap.modified), "d MMM, hh:mm") : ""
                    color: Theme.textMuted
                    font.pixelSize: Theme.smallSize
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.space4
            Layout.rightMargin: Theme.gap
            Layout.topMargin: Theme.space2
            Layout.bottomMargin: Theme.space3
            spacing: Theme.space1

            IconButton {
                id: paletteButton
                visible: !editor.readOnly
                icon: Icons.palette
                tip: "Colour"
                onClicked: colours.open()
                ColourPicker {
                    id: colours
                    y: -implicitHeight - Theme.space2
                    current: NoteEditorWrap.noteColor
                    onPicked: (index) => NoteEditorWrap.setColor(index)
                }
            }
            IconButton {
                id: labelButton
                visible: !editor.readOnly
                icon: Icons.label
                tip: "Labels"
                onClicked: labelPicker.open()
                LabelPicker {
                    id: labelPicker
                    y: -implicitHeight - Theme.space2
                }
            }
            IconButton {
                visible: !editor.readOnly
                icon: NoteEditorWrap.checklist ? Icons.textNote : Icons.checklist
                tip: NoteEditorWrap.checklist ? "Hide checkboxes" : "Show checkboxes"
                onClicked: NoteEditorWrap.setChecklist(!NoteEditorWrap.checklist)
            }
            IconButton {
                visible: !editor.readOnly
                icon: NoteEditorWrap.archived ? Icons.unarchive : Icons.archive
                tip: NoteEditorWrap.archived ? "Unarchive" : "Archive"
                onClicked: editor.closeAfter(() => NoteEditorWrap.setArchived(!NoteEditorWrap.archived))
            }
            IconButton {
                visible: !editor.readOnly
                icon: Icons.trash
                tip: "Delete"
                onClicked: editor.closeAfter(() => NoteEditorWrap.setTrashed(true))
            }
            IconButton {
                visible: !editor.readOnly
                icon: Icons.undo
                tip: "Undo"
                enabled: editor.lastField !== null && editor.lastField.canUndo
                onClicked: editor.lastField.undo()
            }
            IconButton {
                visible: !editor.readOnly
                icon: Icons.redo
                tip: "Redo"
                enabled: editor.lastField !== null && editor.lastField.canRedo
                onClicked: editor.lastField.redo()
            }
            IconButton {
                visible: editor.readOnly
                icon: Icons.deleteForever
                tip: "Delete forever"
                onClicked: editor.closeAfter(() => NoteEditorWrap.deleteForever())
            }
            IconButton {
                visible: editor.readOnly
                icon: Icons.restore
                tip: "Restore"
                onClicked: NoteEditorWrap.setTrashed(false)
            }
            Text {
                visible: editor.readOnly && !Theme.compact
                leftPadding: Theme.space4
                text: "Notes in the Trash can't be edited"
                color: Theme.textMuted
                font.pixelSize: Theme.smallSize
            }
            Item { Layout.fillWidth: true }
            ThemedButton {
                text: "Close"
                onClicked: editor.close()
            }
        }
    }
}
