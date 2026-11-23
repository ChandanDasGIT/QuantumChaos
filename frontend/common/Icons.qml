pragma Singleton
import QtQuick

// Every icon of the app, drawn as an outline path on a 24 x 24 grid (SVG path syntax).
// Show one with Glyph { path: Icons.search }.
QtObject {
    readonly property string menu:      "M4 6h16M4 12h16M4 18h16"
    readonly property string search:    "M10.5 4a6.5 6.5 0 1 0 0 13a6.5 6.5 0 1 0 0-13zM15.5 15.5L20 20"
    readonly property string close:     "M6 6l12 12M18 6L6 18"
    readonly property string plus:      "M12 5v14M5 12h14"
    readonly property string check:     "M5 12.5l4.5 4.5L19 7.5"
    readonly property string chevronDown: "M6 9l6 6 6-6"
    readonly property string chevronRight: "M9 6l6 6-6 6"

    readonly property string bulb:      "M9 18h6M10 21h4M12 3a6 6 0 0 0-3.5 10.9c.6.5 1 1.2 1 2V16h5v-.1c0-.8.4-1.5 1-2A6 6 0 0 0 12 3z"
    readonly property string label:     "M3 12V4h8l10 10-8 8zM7 7.5a1 1 0 1 0 2 0a1 1 0 1 0-2 0z"
    readonly property string archive:   "M3 5h18v4H3zM5 9v10h14V9M10 13h4"
    readonly property string unarchive: "M3 5h18v4H3zM5 9v10h14V9M12 17v-5M9.5 14.5L12 12l2.5 2.5"
    readonly property string trash:     "M4 7h16M10 11v6M14 11v6M6 7l1 13h10l1-13M9 7V4h6v3"
    readonly property string deleteForever: "M4 7h16M6 7l1 13h10l1-13M9 7V4h6v3M10 11l4 5M14 11l-4 5"
    readonly property string restore:   "M4 12a8 8 0 1 0 2.3-5.6M4 4v5h5"
    readonly property string settings:  "M4 7h9M17 7h3M4 17h3M11 17h9M15 5v4M9 15v4"

    readonly property string pin:       "M9 4h6M10 4v6l-3 4h10l-3-4V4M12 14v6"
    readonly property string palette:   "M12 3a9 9 0 1 0 0 18c1 0 1.7-.7 1.7-1.6c0-.4-.2-.8-.5-1.1c-.3-.3-.5-.7-.5-1.1c0-.9.7-1.6 1.6-1.6H16a5 5 0 0 0 5-5C21 6.6 17 3 12 3zM6.5 11.5a1 1 0 1 0 2 0a1 1 0 1 0-2 0zM9.5 7.5a1 1 0 1 0 2 0a1 1 0 1 0-2 0zM14 7.5a1 1 0 1 0 2 0a1 1 0 1 0-2 0z"
    readonly property string undo:      "M9 14L4 9l5-5M4 9h10.5a5.5 5.5 0 0 1 0 11H11"
    readonly property string redo:      "M15 14l5-5-5-5M20 9H9.5a5.5 5.5 0 0 0 0 11H13"
    readonly property string checklist: "M4 6l1.5 1.5L8 5M4 12l1.5 1.5L8 11M4 18l1.5 1.5L8 17M11 6h9M11 12h9M11 18h9"
    readonly property string textNote:  "M5 6h14M5 10h14M5 14h14M5 18h9"
    readonly property string boxEmpty:  "M5 5h14v14H5z"
    readonly property string boxChecked: "M5 5h14v14H5zM8.5 12l2.5 2.5 4.5-5"

    readonly property string cloud:     "M7 18a4 4 0 0 1-.6-8A6 6 0 0 1 18 9a4.5 4.5 0 0 1-.5 9z"
    readonly property string cloudOff:  "M7 18a4 4 0 0 1-.6-8A6 6 0 0 1 18 9a4.5 4.5 0 0 1-.5 9zM4 4l16 16"
    readonly property string cloudDone: "M7 18a4 4 0 0 1-.6-8A6 6 0 0 1 18 9a4.5 4.5 0 0 1-.5 9zM9.5 13.5l2 2 3.5-3.5"
    readonly property string cloudSync: "M7 18a4 4 0 0 1-.6-8A6 6 0 0 1 18 9a4.5 4.5 0 0 1-.5 9zM12 10.5v5M9.8 13.3l2.2 2.2 2.2-2.2"
    readonly property string cloudAlert: "M7 18a4 4 0 0 1-.6-8A6 6 0 0 1 18 9a4.5 4.5 0 0 1-.5 9zM12 10.5v3M12 15.5v.5"
}
