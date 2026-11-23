import QtQuick

// A round button showing one icon.
Item {
    id: button
    property string icon
    property bool filled: false
    property color color: Theme.iconDefault
    property real size: Theme.buttonSize
    property string tip
    signal clicked()

    implicitWidth: size
    implicitHeight: size
    opacity: enabled ? 1 : Theme.disabledOpacity
    Accessible.role: Accessible.Button
    Accessible.name: tip

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: area.containsMouse && button.enabled ? Theme.iconHoverBg : "transparent"
    }
    Glyph {
        anchors.centerIn: parent
        size: Math.round(button.size * Theme.iconInButton)
        path: button.icon
        filled: button.filled
        color: button.color
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: button.clicked()
    }
}
