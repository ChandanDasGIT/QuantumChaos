import QtQuick
import QtQuick.Controls.Basic

// The row of note colours; the first one is "no colour".
Popup {
    id: picker
    property int current: 0
    signal picked(int index)

    padding: Theme.space4
    background: Rectangle {
        color: Theme.dialogBg
        radius: Theme.radius
        border.color: Theme.fieldBorder
    }
    contentItem: Grid {
        columns: Theme.swatchColumns
        spacing: Theme.space3
        Repeater {
            model: Theme.noteColors.length
            delegate: Rectangle {
                required property int index
                width: Theme.swatchSize
                height: Theme.swatchSize
                radius: width / 2
                color: Theme.noteColors[index]
                border.width: index === picker.current ? Theme.lineBold : Theme.line
                border.color: index === picker.current ? Theme.accent : Theme.fieldBorder
                Glyph {
                    anchors.centerIn: parent
                    visible: index === picker.current
                    size: Theme.iconSmall
                    path: Icons.check
                    color: Theme.text
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        picker.close()
                        picker.picked(index)
                    }
                }
            }
        }
    }
}
