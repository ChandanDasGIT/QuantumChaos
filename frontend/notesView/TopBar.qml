import QtQuick
import QtQuick.Layouts

// The bar across the top: the menu button, the section's name, search, sync status and settings.
Rectangle {
    id: bar
    signal menuClicked()
    signal settingsClicked()

    readonly property string heading: NotesViewWrap.section === NotesViewWrap.Archive ? "Archive"
        : NotesViewWrap.section === NotesViewWrap.Trash ? "Trash"
        : NotesViewWrap.section === NotesViewWrap.Label ? NotesViewWrap.label : "QuantumChaos"

    implicitHeight: Theme.barHeight
    color: Theme.barBg

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Theme.line
        color: Theme.divider
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.space4
        anchors.rightMargin: Theme.gap
        spacing: Theme.space4

        IconButton {
            icon: Icons.menu
            tip: "Main menu"
            onClicked: bar.menuClicked()
        }
        Glyph {
            visible: !Theme.compact && NotesViewWrap.section === NotesViewWrap.Notes
            size: Theme.logoSize
            path: Icons.bulb
            color: Theme.accent
        }
        Text {
            visible: !Theme.compact
            Layout.preferredWidth: Math.min(implicitWidth, Theme.headingMax)
            text: bar.heading
            elide: Text.ElideRight
            color: Theme.text
            font.pixelSize: Theme.titleSize
        }
        Item { visible: !Theme.compact; Layout.preferredWidth: Theme.headingGap }

        ThemedTextField {
            id: search
            Layout.fillWidth: true
            Layout.maximumWidth: Theme.searchMax
            leftPadding: Theme.searchTextX
            rightPadding: Theme.searchClearRoom
            placeholderText: "Search"
            onTextChanged: NotesViewWrap.search = text
            Keys.onEscapePressed: text = ""
            Glyph {
                anchors.left: parent.left
                anchors.leftMargin: Theme.space5
                anchors.verticalCenter: parent.verticalCenter
                size: Theme.iconMedium
                path: Icons.search
            }
            IconButton {
                visible: search.text !== ""
                anchors.right: parent.right
                anchors.rightMargin: Theme.space1
                anchors.verticalCenter: parent.verticalCenter
                size: Theme.buttonSmall
                icon: Icons.close
                tip: "Clear search"
                onClicked: search.text = ""
            }
        }
        Item { Layout.fillWidth: true; Layout.minimumWidth: 0 }

        IconButton {
            readonly property int state: NotesViewWrap.syncState
            icon: state === NotesViewWrap.Synced ? Icons.cloudDone
                : state === NotesViewWrap.Syncing ? Icons.cloudSync
                : state === NotesViewWrap.SyncError ? Icons.cloudAlert : Icons.cloudOff
            color: state === NotesViewWrap.SyncError ? Theme.danger
                 : state === NotesViewWrap.SyncOff || state === NotesViewWrap.SyncOffline ? Theme.textDim : Theme.iconDefault
            enabled: state !== NotesViewWrap.SyncOff
            tip: "Sync now"
            onClicked: NotesViewWrap.syncNow()
        }
        IconButton {
            icon: Icons.settings
            tip: "Settings"
            onClicked: bar.settingsClicked()
        }
    }
}
