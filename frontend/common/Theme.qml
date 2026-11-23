pragma Singleton
import QtQuick

// Every colour and size of the app lives here. Use it anywhere as Theme.windowBg, Theme.accent, ...
// The colours are the One Dark palette, with its light counterpart.
QtObject {
    property bool isDark: true

    // ---- surfaces: the window is the darkest, bars and dialogs a step lighter ----
    readonly property color windowBg:   isDark ? "#1e2227" : "#fafafa"
    readonly property color barBg:      isDark ? "#21252b" : "#eaeaeb"
    readonly property color dialogBg:   isDark ? "#282c34" : "#f2f2f3"
    readonly property color fieldBg:    isDark ? "#1b1d23" : "#ffffff"
    readonly property color fieldBorder: isDark ? "#3b4048" : "#c8c8ca"
    readonly property color hoverBg:    isDark ? "#2c313a" : "#e2e2e3"
    readonly property color divider:    isDark ? "#181a1f" : "#dbdbdc"
    readonly property color scrim:      isDark ? "#99000000" : "#66000000"
    readonly property color cardBorder: isDark ? "#3b4048" : "#dadce0"
    readonly property color iconHoverBg: isDark ? "#1affffff" : "#14000000"   // a round icon button under the mouse
    readonly property color selectedBg: isDark ? "#2f3d5c" : "#dde6fb"        // the chosen side menu entry
    readonly property color chipBg:     isDark ? "#26ffffff" : "#14000000"    // a label on a card
    readonly property color selectionBg: isDark ? "#3e4451" : "#cfd8ea"
    readonly property color knob:       "#ffffff"                              // the dot of a switch

    // ---- text ----
    readonly property color text:         isDark ? "#abb2bf" : "#383a42"
    readonly property color textMuted:    isDark ? "#7b828d" : "#696c77"
    readonly property color textDim:      isDark ? "#5b626f" : "#9d9d9f"
    readonly property color iconDefault:  isDark ? "#797e89" : "#696c77"
    readonly property color accentText:     "#ffffff"                            // text on an accent-filled button

    // ---- accents ----
    readonly property color accent:     isDark ? "#528bff" : "#4078f2"
    readonly property real accentDownDarker: 1.15                              // a pressed accent button
    readonly property color danger:     isDark ? "#e06c75" : "#e45649"        // something went wrong

    // ---- note colours: index 0 is "no colour", the rest follow Google Keep's set ----
    // There must be as many as the backend's noteColorCount; the backend keeps every index in range.
    readonly property var noteColors: isDark ? [
        "#262a31", "#5c2b29", "#614a19", "#635d19", "#345920", "#16504b",
        "#2d555e", "#1e3a5f", "#42275e", "#5b2245", "#442f19", "#3c3f43"
    ] : [
        "#ffffff", "#f28b82", "#fbbc04", "#fff475", "#ccff90", "#a7ffeb",
        "#cbf0f8", "#aecbfa", "#d7aefb", "#fdcfe8", "#e6c9a8", "#e8eaed"
    ]

    // ---- the window ----
    readonly property int windowWidth:     1280
    readonly property int windowHeight:    800
    readonly property int windowMinWidth:  360
    readonly property int windowMinHeight: 480
    readonly property int compactBelow:    700     // narrower than this: the phone layout
    // A phone-width window (set by Main): the side menu folds away and the grid has two columns.
    property bool compact: false
    // Phones: the window goes behind the status and navigation bars; what must stay clear of them
    // uses these margins (set by Main from the window's safe area; 0 on the desktop).
    readonly property bool touchOnly: Qt.platform.os === "android" || Qt.platform.os === "ios"
    property real safeTop: 0
    property real safeBottom: 0
    property real safeLeft: 0
    property real safeRight: 0

    // ---- spacing scale ----
    readonly property int space1: 2
    readonly property int space2: 4
    readonly property int space3: 6
    readonly property int space4: 8
    readonly property int space5: 10
    readonly property int space7: 16
    readonly property int space8: 20
    readonly property int gap:    12
    readonly property int margin: compact ? 12 : 24
    readonly property int line:   1                // borders and dividers
    readonly property int lineBold: 2              // the border of the chosen colour

    // ---- shapes ----
    readonly property int radius:      8
    readonly property int radiusSmall: 4

    // ---- icons and buttons ----
    readonly property int iconGrid:     24         // the grid the icon paths are drawn on
    readonly property real iconStroke:  1.8
    readonly property int iconSize:     20
    readonly property int iconSmall:    16
    readonly property int iconMedium:   18
    readonly property int iconLarge:    22
    readonly property int logoSize:     26
    readonly property int emptyIcon:    96         // the big icon of an empty section
    readonly property real iconInButton: 0.56      // icon size as a share of its round button
    readonly property real disabledOpacity: 0.35
    readonly property int buttonSize:   34
    readonly property int buttonSmall:  30
    readonly property int buttonTiny:   22
    readonly property int textButtonHeight: 32
    readonly property int textButtonPadding: 14

    // ---- bars, menus, fields ----
    readonly property int barHeight:     56
    readonly property int sideWidth:     220
    readonly property int sideFolded:    64
    readonly property int entryHeight:   44
    readonly property int entryFolded:   48
    readonly property int entryTextX:    52        // where a side menu entry's name starts
    readonly property int headingMax:    200       // the section name in the top bar
    readonly property int headingGap:    40
    readonly property int searchMax:     720
    readonly property int searchTextX:   38        // room for the magnifying glass
    readonly property int searchClearRoom: 34
    readonly property int controlHeight: 36
    readonly property int rowHeight:     32        // a line in a picker
    readonly property int lineHeight:    34        // a checklist line, the "List item" row

    // ---- cards and the grid ----
    readonly property int cardWidth:     240
    readonly property int cardPad:       12
    readonly property int pinRoom:       26        // room left for the pin beside a card's title
    readonly property int titleLines:    3
    readonly property int bodyLines:     14
    readonly property int previewIndent: 22        // a checklist line's text, after its box
    readonly property int chipHeight:    22
    readonly property int chipHeightLarge: 24
    readonly property int chipPad:       16
    readonly property int chipRemoveRoom: 34       // a label chip with its remove button
    readonly property real groupLabelSpacing: 1    // letter spacing of PINNED / OTHERS
    readonly property int newNoteHeight: 46
    readonly property int noteMaxWidth:  600       // the "Take a note…" bar and the editor
    readonly property int emptyTop:      80

    // ---- dialogs and pop-ups ----
    readonly property int dialogWidth:   420
    readonly property int pickerWidth:   240
    readonly property int swatchSize:    30
    readonly property int swatchColumns: 6
    readonly property int switchWidth:   40
    readonly property int switchHeight:  22
    readonly property int switchKnob:    16
    readonly property int switchInset:   3

    // ---- text sizes ----
    readonly property int smallSize:   11
    readonly property int bodySize:    13
    readonly property int editorSize:  14
    readonly property int headingSize: 15
    readonly property int titleSize:   20
}
