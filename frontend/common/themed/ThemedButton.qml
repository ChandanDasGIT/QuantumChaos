import QtQuick
import QtQuick.Controls.Basic

// A text button. `primary` fills it with the accent colour.
Button {
    id: button
    property bool primary: false
    implicitHeight: Theme.textButtonHeight
    leftPadding: Theme.textButtonPadding
    rightPadding: Theme.textButtonPadding
    font.pixelSize: Theme.bodySize
    contentItem: Text {
        text: button.text
        font: button.font
        color: button.primary ? Theme.accentText : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: Theme.radiusSmall
        color: button.primary ? (button.down ? Qt.darker(Theme.accent, Theme.accentDownDarker) : Theme.accent)
                              : (button.down || button.hovered ? Theme.iconHoverBg : "transparent")
    }
}
