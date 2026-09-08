pragma Singleton
import QtQuick

QtObject {
    readonly property color background: "#0F172A"
    readonly property color primary: "#1E293B"
    readonly property color textOnPrimary: "#FFFFFF"
    readonly property color secondary: "#334155"
    readonly property color textOnSecondary: "#FFFFFF"
    readonly property color accent: "#22C55E"
    readonly property color textOnAccent: "#0F172A"
    readonly property color foreground: "#F8FAFC"
    readonly property color card: "#1B2336"
    readonly property color cardForeground: "#F8FAFC"
    readonly property color muted: "#272F42"
    readonly property color mutedForeground: "#94A3B8"
    readonly property color border: "#475569"
    readonly property color destructive: "#EF4444"
    readonly property color textOnDestructive: "#FFFFFF"
    readonly property color ring: "#FFFFFF"
    readonly property color glassFill: Qt.rgba(15 / 255, 23 / 255, 42 / 255, 0.55)
    readonly property color glassBorder: Qt.rgba(1, 1, 1, 0.18)

    readonly property int caption: 12
    readonly property int body: 16
    readonly property int h2: 21
    readonly property int h1: 28

    readonly property int spaceXs: 2
    readonly property int spaceSm: 4
    readonly property int spaceMd: 8
    readonly property int spaceLg: 12
    readonly property int spaceXl: 16
    readonly property int space2xl: 24
    readonly property int space3xl: 32

    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int hit: 32
    readonly property int titleBar: 48
    readonly property int rail: 64
    readonly property int commandBar: 44
    readonly property int row: 36
    readonly property int icon: 20

    readonly property string fontUi: "Segoe UI"
    readonly property string fontMono: "Consolas"

    property bool reducedMotion: false
    readonly property int durationFast: reducedMotion ? 0 : 180
    readonly property int durationMed: reducedMotion ? 0 : 250
}
