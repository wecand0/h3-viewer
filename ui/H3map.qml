import QtQuick 6.8
import QtQuick.Controls 6.8
import QtLocation
import QtPositioning
import QtQuick.Layouts
import QtQuick.Shapes 6.8

import H3VIEWER 1.0  // Ваш модуль

import MapLibre 3.0

Pane {
    id: mapPane

    Universal.background: Universal.altMediumLow
    padding: 5

    //clip: true

    background: Rectangle {
        border.color: "#cccccc"
        border.width: 1
        color: "#f0f0f0"
        radius: 10
    }

    Plugin {
        id: mapPlugin

        name: "maplibre"

        parameters: [
            PluginParameter {
                name: "maplibre.map.styles"
                ////"qrc:/ST/data/styles/style.json"
                value: mapProvider.url
            }
        ]
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 5

        Map {
            id: map

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredHeight: parent.height

            center: QtPositioning.coordinate(55.04915599592714, 37.61226224295367)

            maximumZoomLevel: 24
            //minimumFieldOfView: map.minimumFieldOfView
            minimumZoomLevel: 5
            objectName: "map"
            plugin: mapPlugin
            zoomLevel: 5

            Component.onCompleted: {

            }
            onBearingChanged: {
                if (map.bearing >= 355) {
                    map.bearing = 0;
                }
            }

            //задание границ просмотра карты
            onCenterChanged: {
            }
            onMapReadyChanged: {
            }
            onTiltChanged: {
                if (map.tilt > map.maximumTilt * 0.9) {
                    map.tilt = map.maximumTilt * 0.9;
                }
            }
            onVisibleChanged: {
                if (!visible) {
                    map.clearData();
                }
            }
            onZoomLevelChanged: {
                if (map.zoomLevel <= 3.5) {
                    map.zoomLevel = 3.5;
                }
            }

            DragHandler {
                id: drag

                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                target: null

                onTranslationChanged: delta => map.pan(-delta.x, -delta.y)
            }
            MouseArea {
                id: mapMouseArea

                property var currentCoordinate: map.toCoordinate(Qt.point(mouseX, mouseY))
                property real prevY: -1

                acceptedButtons: Qt.LeftButton | Qt.RightButton// | Qt.MiddleButton
                anchors.fill: parent
                cursorShape: Qt.CrossCursor

                onClicked: event => {
                    if (event.button === Qt.LeftButton) {
                    }
                    if (event.button === Qt.RightButton) {
                    }
                }
                onDoubleClicked: event => {
                    let mouseGeoPos = map.toCoordinate(Qt.point(event.x, event.y));
                    let preZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    if (event.button === Qt.LeftButton) {
                        map.zoomLevel = Math.floor(map.zoomLevel + 1);
                    } else if (event.button === Qt.RightButton) {
                        map.zoomLevel = Math.floor(map.zoomLevel - 1);
                    }
                    let postZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    let dx = postZoomPoint.x - preZoomPoint.x;
                    let dy = postZoomPoint.y - preZoomPoint.y;
                    let mapCenterPoint = Qt.point(map.width / 2.0 + dx, map.height / 2.0 + dy);
                    map.center = map.toCoordinate(mapCenterPoint);
                }
                onPressed: event => {
                    if (event.button === Qt.RightButton) {}
                    if (event.button === Qt.LeftButton) {}
                }
                onReleased: event => {
                    if (event.button === Qt.RightButton) {}
                }
                onWheel: event => {
                    let mouseGeoPos = map.toCoordinate(Qt.point(event.x, event.y));
                    let preZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    map.zoomLevel += (event.angleDelta.y > 0) ? 0.5 : -0.5;
                    let postZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    let dx = postZoomPoint.x - preZoomPoint.x;
                    let dy = postZoomPoint.y - preZoomPoint.y;
                    map.center = map.toCoordinate(Qt.point(map.width / 2 + dx, map.height / 2 + dy));
                }


            }
            Shortcut {
                enabled: map.zoomLevel < map.maximumZoomLevel
                sequence: StandardKey.ZoomIn

                onActivated: map.zoomLevel = Math.round(map.zoomLevel + 1)
            }
            Shortcut {
                enabled: map.zoomLevel > map.minimumZoomLevel
                sequence: StandardKey.ZoomOut

                onActivated: map.zoomLevel = Math.round(map.zoomLevel - 1)
            }
        }
    }
}
