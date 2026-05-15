import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import QtMultimedia 6.2
import QtQuick.Controls.Material
import QtQuick.Effects
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 1280
    height: 720
    title: "LemonStudy"

    StackView {
        id: mainStack
        anchors.fill: parent
        initialItem: mainMenu
    }

    // Fontloader to load a custom font.
    FontLoader {
        id: fredoka
        source: "../assets/fonts/Fredoka.ttf"
    }

    FontLoader {
        id: fredokaOne
        source: "../assets/fonts/FredokaOne.ttf"
    }

    SoundEffect {
        id: hoversound
        source: "../assets/sounds/water_drop_sped_up.wav"
        // Source: https://freesound.org/people/deleted_user_2104797/sounds/166322/
        volume: 0.5
    }

    Component {
        id: mainMenu

        Rectangle {
            width: parent ? parent.width : 800
            height: parent ? parent.height : 600
            // color: "#FFDE59" // Soft lemon-yellow background

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#FFDE59" }
                GradientStop { position: 1.0; color: "#FFF8DC" }
            }

            Text {
                id: titleText
                text: "Lemon Studies"
                font.pixelSize: 64
                font.weight: Font.Bold
                color: "#333333"
                font.family: fredoka.name
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 50
            }

            // Vertical column for buttons
            Column {
                anchors.centerIn: parent
                spacing: 20
                width: 474 // Width from your design

                // Streaks button
                Rectangle {
                    width: parent.width
                    height: 96 // Height from your design
                    radius: height / 2
                    color: "#FFF9F1"
                    border.color: "#333333"
                    border.width: 2

                    // Drop shadow effect
                    layer.enabled: false
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 3
                        radius: 8
                        samples: 17
                        color: "#40000000"
                    }

                    Text {
                        text: "Streaks"
                        anchors.centerIn: parent
                        font.pixelSize: 36
                        font.family: fredoka.name
                        color: "#333333"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mainStack.push("streaks.qml")
                        // Hover effect
                        hoverEnabled: true
                        onEntered: {
                            parent.scale = 1.02
                            hoversound.play()

                        }
                        onExited: parent.scale = 1.0
                    }

                    // Smooth animation for scale
                    Behavior on scale {
                        NumberAnimation { duration: 100 }
                    }
                }

                // Pomodoro button
                Rectangle {
                    width: parent.width
                    height: 96
                    radius: height / 2
                    color: "#FFF9F1"
                    border.color: "#333333"
                    border.width: 2

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 3
                        radius: 8
                        samples: 17
                        color: "#40000000"
                    }

                    Text {
                        text: "Pomodoro"
                        anchors.centerIn: parent
                        font.pixelSize: 36
                        font.family: fredoka.name
                        color: "#333333"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mainStack.push("PomodoroView.qml")

                        hoverEnabled: true
                        onEntered: {
                            parent.scale = 1.02
                            hoversound.play()

                        }
                        onExited: parent.scale = 1.0
                    }

                    Behavior on scale {
                        NumberAnimation { duration: 100 }
                    }
                }

                // Todo-list button
                Rectangle {
                    width: parent.width
                    height: 96
                    radius: height / 2
                    color: "#FFF9F1"
                    border.color: "#333333"
                    border.width: 2

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 3
                        radius: 8
                        samples: 17
                        color: "#40000000"
                    }

                    Text {
                        text: "Todo-list"
                        anchors.centerIn: parent
                        font.pixelSize: 36
                        font.family: fredoka.name
                        color: "#333333"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mainStack.push("todo.qml")

                        hoverEnabled: true
                        onEntered: {
                            parent.scale = 1.02
                            hoversound.play()

                        }
                        onExited: parent.scale = 1.0
                    }

                    Behavior on scale {
                        NumberAnimation { duration: 100 }
                    }
                }

                // Block button
                Rectangle {
                    width: parent.width
                    height: 96
                    radius: height / 2
                    color: "#FFF9F1"
                    border.color: "#333333"
                    border.width: 2

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 3
                        radius: 8
                        samples: 17
                        color: "#40000000"
                    }

                    Text {
                        text: "Block Distractions"
                        anchors.centerIn: parent
                        font.pixelSize: 36
                        font.family: fredoka.name
                        color: "#333333"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mainStack.push("Calendar.qml")

                        hoverEnabled: true
                        onEntered: {
                            parent.scale = 1.02
                            hoversound.play()

                        }
                        onExited: parent.scale = 1.0
                    }

                    Behavior on scale {
                        NumberAnimation { duration: 100 }
                    }
                }
            }

            Row {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 24
                spacing: 20

                // // Music icon
                // Rectangle {
                //     width: 50
                //     height: 50
                //     radius: 25
                //     color: "#FFF9F1"
                //     border.color: "#333333"
                //     border.width: 1

                //     Text {
                //         id: name
                //         text: qsTr("text")
                //         anchors.centerIn: parent
                //     }

                //     MouseArea {
                //         anchors.fill: parent
                //         cursorShape: Qt.PointingHandCursor
                //         onClicked: console.log("Music button clicked")
                //         hoverEnabled: true
                //         onEntered: {
                //             parent.scale = 1.02
                //             hoversound.play()

                //         }
                //         onExited: parent.scale = 1.0
                //     }
                // }


                // exit button
                Rectangle {
                    id: exitButton
                    width: 50
                    height: 50
                    radius: 25
                    color: "#FFF9F1"
                    border.color: "#333333"
                    border.width: 1

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 3
                        radius: 8
                        samples: 17
                        color: "#40000000"
                    }

                    Image {
                        source: "../assets/images/exiticon.png"
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        fillMode: Image.PreserveAspectFit
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onEntered: {
                            exitButton.scale = 1.02
                            hoversound.play()
                        }
                        onExited: exitButton.scale = 1.0
                        onClicked: {
                            console.log("Exit button clicked")
                            Qt.quit()
                        }
                    }


                }

            }
        }
    }
}
