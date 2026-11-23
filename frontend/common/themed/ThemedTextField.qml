import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: field
    implicitHeight: Theme.controlHeight
    leftPadding: Theme.space5
    rightPadding: Theme.space5
    font.pixelSize: Theme.bodySize
    color: Theme.text
    placeholderTextColor: Theme.textDim
    selectionColor: Theme.selectionBg
    selectedTextColor: Theme.text
    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.fieldBg
        border.width: Theme.line
        border.color: field.activeFocus ? Theme.accent : Theme.fieldBorder
    }
}
