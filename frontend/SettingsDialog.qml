import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Popup {
    id: dialog
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(Theme.dialogWidth, parent.width - 2 * Theme.margin)
    padding: Theme.space8
    modal: true
    Overlay.modal: Rectangle { color: Theme.scrim }

    Binding { target: Theme; property: "isDark"; value: SettingsDialogWrap.darkTheme }

    background: Rectangle {
        color: Theme.dialogBg
        radius: Theme.radius
        border.color: Theme.fieldBorder
    }

    contentItem: ColumnLayout {
        spacing: Theme.space7

        Text {
            text: "Settings"
            color: Theme.text
            font.pixelSize: Theme.titleSize
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: "Dark theme"
                color: Theme.text
                font.pixelSize: Theme.bodySize
            }
            Rectangle {
                id: toggle
                implicitWidth: Theme.switchWidth
                implicitHeight: Theme.switchHeight
                radius: height / 2
                color: Theme.isDark ? Theme.accent : Theme.fieldBorder
                Rectangle {
                    width: Theme.switchKnob
                    height: Theme.switchKnob
                    radius: width / 2
                    y: Theme.switchInset
                    x: Theme.isDark ? parent.width - width - Theme.switchInset : Theme.switchInset
                    color: Theme.knob
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: SettingsDialogWrap.darkTheme = !SettingsDialogWrap.darkTheme
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space2
            Text {
                text: "Sync"
                color: Theme.text
                font.pixelSize: Theme.bodySize
            }
            Text {
                readonly property int state: SettingsDialogWrap.syncState
                Layout.fillWidth: true
                text: state === SettingsDialogWrap.SyncOff ? "Sync isn't set up in this build."
                    : state === SettingsDialogWrap.Syncing ? "Syncing with " + SettingsDialogWrap.syncAddress + "…"
                    : state === SettingsDialogWrap.Synced ? "Synced with " + SettingsDialogWrap.syncAddress
                        + " at " + Qt.formatDateTime(new Date(SettingsDialogWrap.lastSync), "hh:mm") + "."
                    : SettingsDialogWrap.syncProblem
                wrapMode: Text.Wrap
                color: state === SettingsDialogWrap.SyncError ? Theme.danger : Theme.textMuted
                font.pixelSize: Theme.smallSize
            }
            Text {
                visible: SettingsDialogWrap.syncState === SettingsDialogWrap.Synced && SettingsDialogWrap.syncProblem !== ""
                Layout.fillWidth: true
                text: SettingsDialogWrap.syncProblem
                wrapMode: Text.Wrap
                color: Theme.danger
                font.pixelSize: Theme.smallSize
            }
            ThemedButton {
                visible: SettingsDialogWrap.syncState !== SettingsDialogWrap.SyncOff
                text: "Sync now"
                onClicked: SettingsDialogWrap.syncNow()
            }
        }

        ColumnLayout {
            visible: SettingsDialogWrap.unreadableNotes > 0 || SettingsDialogWrap.saveFailed
            Layout.fillWidth: true
            spacing: Theme.space2
            Text {
                text: "Notes on this device"
                color: Theme.text
                font.pixelSize: Theme.bodySize
            }
            Text {
                visible: SettingsDialogWrap.unreadableNotes > 0
                Layout.fillWidth: true
                text: SettingsDialogWrap.unreadableNotes + (SettingsDialogWrap.unreadableNotes === 1 ? " note file" : " note files")
                      + " couldn't be opened (made with another key, or damaged). They were left untouched."
                wrapMode: Text.Wrap
                color: Theme.textMuted
                font.pixelSize: Theme.smallSize
            }
            Text {
                visible: SettingsDialogWrap.saveFailed
                Layout.fillWidth: true
                text: "Some changes couldn't be saved on this device (disk full, or no permission)."
                wrapMode: Text.Wrap
                color: Theme.textMuted
                font.pixelSize: Theme.smallSize
            }
        }

        ThemedButton {
            Layout.alignment: Qt.AlignRight
            text: "Close"
            onClicked: dialog.close()
        }
    }
}
