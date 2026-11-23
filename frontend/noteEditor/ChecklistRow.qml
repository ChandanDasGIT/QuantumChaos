import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// One checklist line: its box and its text. Enter starts the next line, Backspace on an empty line removes it.
Item {
    id: row
    required property int index
    required property var modelData
    property bool wantsFocus: false
    property bool focusAtEnd: false
    property bool readOnly: false
    readonly property alias field: field
    signal toggled(bool done)
    signal edited(string text)
    signal splitRequested(int cursor)
    signal removeRequested()
    signal fieldFocused()

    implicitHeight: Math.max(Theme.lineHeight, field.implicitHeight)

    function takeFocus() {
        field.forceActiveFocus()
        field.cursorPosition = row.focusAtEnd ? field.length : 0
    }
    onWantsFocusChanged: if (wantsFocus && visible) Qt.callLater(takeFocus)
    Component.onCompleted: if (wantsFocus && visible) Qt.callLater(takeFocus)

    HoverHandler { id: hover }

    RowLayout {
        anchors.fill: parent
        spacing: Theme.space1
        IconButton {
            Layout.alignment: Qt.AlignTop
            size: Theme.buttonSize
            icon: row.modelData.done ? Icons.boxChecked : Icons.boxEmpty
            tip: row.modelData.done ? "Uncheck" : "Check"
            enabled: !row.readOnly
            onClicked: row.toggled(!row.modelData.done)
        }
        TextArea {
            id: field
            Layout.fillWidth: true
            text: row.modelData.text
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.Wrap
            readOnly: row.readOnly
            placeholderText: "List item"
            placeholderTextColor: Theme.textDim
            color: row.modelData.done ? Theme.textMuted : Theme.text
            font.strikeout: row.modelData.done
            font.pixelSize: Theme.bodySize
            selectionColor: Theme.selectionBg
            selectedTextColor: Theme.text
            background: null
            onTextChanged: if (activeFocus) row.edited(text)
            onActiveFocusChanged: if (activeFocus) row.fieldFocused()
            Keys.onPressed: (event) => {
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                    row.splitRequested(cursorPosition)
                    event.accepted = true
                } else if (event.key === Qt.Key_Backspace && text === "") {
                    row.removeRequested()
                    event.accepted = true
                }
            }
        }
        IconButton {
            Layout.alignment: Qt.AlignTop
            visible: !row.readOnly && (hover.hovered || field.activeFocus)
            size: Theme.buttonSmall
            icon: Icons.close
            tip: "Remove line"
            onClicked: row.removeRequested()
        }
    }
}
