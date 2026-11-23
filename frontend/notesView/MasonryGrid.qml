import QtQuick

// The cards of the grid, pinned ones first, each group under its label row.
// Where every card and row goes comes from NotesViewWrap.layout().
Item {
    id: grid
    signal openNote(string noteId)

    readonly property var geometry: NotesViewWrap.grid(width, Theme.cardWidth, Theme.gap, Theme.compact)
    readonly property real labelHeight: pinnedLabel.implicitHeight + Theme.gap
    property var placed: ({ "pinnedLabelY": -1, "othersLabelY": -1, "height": 0 })

    implicitHeight: placed.height

    function layout() {
        const heights = []
        for (let i = 0; i < cards.count; ++i) {
            const card = cards.itemAt(i)
            heights.push(card ? card.height : 0)
        }
        placed = NotesViewWrap.layout(heights, width, Theme.cardWidth, Theme.gap, labelHeight, Theme.compact)
        for (let i = 0; i < cards.count; ++i) {
            const card = cards.itemAt(i)
            if (!card) continue
            card.x = placed.x[i]
            card.y = placed.y[i]
        }
    }

    Timer { id: relayout; interval: 0; onTriggered: grid.layout() }
    onWidthChanged: relayout.restart()
    Connections {
        target: Theme
        function onCompactChanged() { relayout.restart() }
    }

    component GroupLabel: Text {
        x: grid.geometry.leftEdge
        leftPadding: Theme.space2
        color: Theme.textMuted
        font.pixelSize: Theme.smallSize
        font.bold: true
        font.letterSpacing: Theme.groupLabelSpacing
    }
    GroupLabel {
        id: pinnedLabel
        visible: grid.placed.pinnedLabelY >= 0
        y: grid.placed.pinnedLabelY
        text: "PINNED"
    }
    GroupLabel {
        visible: grid.placed.othersLabelY >= 0
        y: grid.placed.othersLabelY
        text: "OTHERS"
    }

    Repeater {
        id: cards
        model: NotesViewWrap
        delegate: NoteCard {
            width: grid.geometry.columnWidth
            height: implicitHeight
            onHeightChanged: relayout.restart()
            onOpenRequested: grid.openNote(noteId)
        }
        onItemAdded: relayout.restart()
        onItemRemoved: relayout.restart()
    }
}
