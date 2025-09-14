// FixedH3Visualization.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

import H3VIEWER 1.0
import MapLibre 3.0

ApplicationWindow {
    id: window

    visible: true
    title: qsTr("h3-viewer")

    flags: Qt.Window | Qt.NoDropShadowWindowHint

    height: Screen.height
    minimumHeight: Screen.height / 4
    minimumWidth: Screen.width / 5
    width: Screen.width

    // Модель и менеджер данных
    H3HexagonModel {
        id: h3Model
        zoom: map.zoomLevel
        onH3ResolutionChanged: {
            console.log("H3 resolution changed to:", h3Resolution)
        }
    }

    // Функция для обновления viewport
    function updateViewport() {
        if (!map.mapReady) {
            console.log("Map not ready yet")
            return;
        }

        // Вычисляем размер видимой области в градусах
        let topLeft = map.toCoordinate(Qt.point(0, 0));
        let bottomRight = map.toCoordinate(Qt.point(map.width, map.height));

        console.log("Calculating viewport - TL:", topLeft, "BR:", bottomRight)

        if (topLeft.isValid && bottomRight.isValid) {
            let widthDegrees = Math.abs(bottomRight.longitude - topLeft.longitude);
            let heightDegrees = Math.abs(topLeft.latitude - bottomRight.latitude);

            console.log("Viewport size:", widthDegrees, "x", heightDegrees, "degrees")

            h3Model.setViewportFromCenter(map.center, widthDegrees, heightDegrees);
        }
    }

    // Таймер для обновления viewport
    Timer {
        id: viewportUpdateTimer
        interval: 250
        repeat: false
        onTriggered: updateViewport()
    }

    // Основной интерфейс
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Карта
        Plugin {
            id: mapPlugin
            name: "maplibre"

            parameters: [
                PluginParameter {
                    name: "maplibre.map.styles"
                    value: mapProvider.url
                }
            ]
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Map {
                id: map

                anchors.fill: parent

                center: QtPositioning.coordinate(55.7558, 37.6173) // Москва
                zoomLevel: 4

                maximumZoomLevel: 24
                minimumZoomLevel: 4
                plugin: mapPlugin

                property bool mapReady: false

                Component.onCompleted: {
                    console.log("Map component completed")
                    mapReadyTimer.start()
                }

                // Таймер для отложенной инициализации
                Timer {
                    id: mapReadyTimer
                    interval: 250
                    repeat: false
                    onTriggered: {
                        map.mapReady = true
                        updateViewport()
                    }
                }

                onCenterChanged: {
                    if (mapReady) viewportUpdateTimer.restart()
                }

                onZoomLevelChanged: {
                    if (map.zoomLevel <= 4) {
                        map.zoomLevel = 4;
                    }
                    h3Model.zoom = map.zoomLevel
                    if (mapReady) viewportUpdateTimer.restart()
                }

                onWidthChanged: {
                    if (mapReady && width > 0) viewportUpdateTimer.restart()
                }

                onHeightChanged: {
                    if (mapReady && height > 0) viewportUpdateTimer.restart()
                }

                // Обработчики мыши
                DragHandler {
                    id: drag
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    target: null
                    onTranslationChanged: delta => map.pan(-delta.x, -delta.y)
                }

                WheelHandler {
                    id: wheel
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
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

                // Слой с H3 гексагонами
                MapItemView {
                    id: hexagonLayer
                    model: h3Model

                    delegate: MapPolygon {
                        id: hexagon

                        // Упрощаем установку path: напрямую используем model.boundary
                        path: model.boundary || []

                        color: hexagonStyle.fillColor

                        border.color: hexagonStyle.borderColor
                        border.width: hexagonStyle.borderWidth

                        opacity: 0.5;

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true

                            onClicked: {
                                selectedHexagon.text = "H3 Index: " + model.h3Index;
                                highlightNeighbors(model.h3Index);
                            }

                            onEntered: {
                                hexagon.border.color = "red"
                                hexagon.border.width = hexagonStyle.borderWidth * 4;
                            }

                            onExited: {
                                hexagon.border.color = hexagonStyle.borderColor
                                hexagon.border.width = hexagonStyle.borderWidth;
                            }
                        }
                    }
                }
            }

            // Информационная панель
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                width: 250
                height: infoColumn.height + 20
                color: "#E0FFFFFF"
                radius: 5

                Column {
                    id: infoColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 5

                    Text {
                        text: "H3 Info"
                        font.bold: true
                        font.pixelSize: 16
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#40000000"
                    }

                    Text {
                        text: "Zoom: " + map.zoomLevel.toFixed(1)
                    }

                    Text {
                        text: "H3 Resolution: " + h3Model.h3Resolution
                    }

                    Text {
                        text: "Visible Hexagons: " + h3Model.hexagonCount
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#40000000"
                    }

                    Text {
                        id: selectedHexagon
                        text: "H3 Index: None"
                        font.pixelSize: 12
                    }
                }
            }
        }

        // Панель управления
        Rectangle {
            Layout.fillHeight: true
            implicitWidth: dataX.implicitWidth + 20  // Добавляем небольшой отступ
            color: "#F5F5F5"

            ScrollView {
                anchors.fill: parent
                contentWidth: dataX.width  // Убедимся, что ScrollView знает о ширине содержимого

                ColumnLayout {
                    id: dataX
                    width: Math.max(implicitWidth, 300)  // Минимальная ширина 300, но может расширяться
                    spacing: 10

                    // Заголовок
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        color: "#3F51B5"

                        Text {
                            anchors.centerIn: parent
                            text: "H3 Visualization Controls"
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    // Стиль гексагонов
                    GroupBox {
                        id: hexagonStyleGroupBox
                        Layout.fillWidth: true
                        Layout.margins: 10
                        title: "Hexagon Style"

                        QtObject {
                            id: hexagonStyle
                            property color fillColor: fillColorPicker.color
                            property color borderColor: borderColorPicker.color
                            property real borderWidth: borderWidthSlider.value
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            RowLayout {
                                spacing: 10
                                Label {
                                    text: "Fill Color:"
                                    Layout.preferredWidth: implicitWidth
                                }
                                Rectangle {
                                    id: fillColorPicker
                                    implicitWidth: 30
                                    implicitHeight: 30
                                    color: "#4080FF80"
                                    border.width: 1

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: fillColorPicker.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 0.5)
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 10
                                Label {
                                    text: "Border Color:"
                                    Layout.preferredWidth: implicitWidth
                                }
                                Rectangle {
                                    id: borderColorPicker
                                    implicitWidth: 30
                                    implicitHeight: 30
                                    color: "#FF0080FF"
                                    border.width: 1

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: borderColorPicker.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0)
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 10
                                Label {
                                    text: "Border Width:"
                                    Layout.preferredWidth: implicitWidth
                                }
                                Slider {
                                    id: borderWidthSlider
                                    from: 0.5
                                    to: 5.0
                                    value: 1.0
                                    stepSize: 0.5
                                    Layout.fillWidth: true
                                    implicitWidth: 150  // Задаем базовую ширину для слайдера
                                }
                                Label {
                                    text: borderWidthSlider.value.toFixed(1)
                                    Layout.preferredWidth: 30
                                }
                            }
                        }
                    }

                    // Навигация
                    GroupBox {
                        Layout.fillWidth: true
                        Layout.margins: 10
                        title: "Quick Navigation"

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 5

                            Button {
                                text: "Moscow"
                                Layout.fillWidth: true
                                //implicitWidth: Math.max(implicitWidth, 150)  // Минимальная ширина кнопок
                                onClicked: {
                                    if (map) {
                                        map.center = QtPositioning.coordinate(55.7558, 37.6173)
                                        map.zoomLevel = 10
                                    }
                                }
                            }

                            Button {
                                text: "St. Petersburg"
                                Layout.fillWidth: true
                                //implicitWidth: Math.max(implicitWidth, 150)
                                onClicked: {
                                    if (map) {
                                        map.center = QtPositioning.coordinate(59.9311, 30.3609)
                                        map.zoomLevel = 10
                                    }
                                }
                            }

                            Button {
                                text: "Novosibirsk"
                                Layout.fillWidth: true
                                //implicitWidth: Math.max(implicitWidth, 150)
                                onClicked: {
                                    if (map) {
                                        map.center = QtPositioning.coordinate(55.0084, 82.9357)
                                        map.zoomLevel = 10
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}