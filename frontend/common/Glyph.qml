import QtQuick
import QtQuick.Shapes

// One icon from Icons, drawn as a line at any size and colour.
Item {
    id: glyph
    property string path
    property color color: Theme.iconDefault
    property bool filled: false
    property real size: Theme.iconSize

    implicitWidth: size
    implicitHeight: size

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: glyph.color
            strokeWidth: Theme.iconStroke
            fillColor: glyph.filled ? glyph.color : "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            scale: Qt.size(glyph.width / Theme.iconGrid, glyph.height / Theme.iconGrid)
            PathSvg { path: glyph.path }
        }
    }
}
