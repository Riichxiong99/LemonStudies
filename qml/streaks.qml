import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Effects
import QtMultimedia 6.2



Page {
    id: streaksPage
    title: "Habit Streaks"

    Connections {
        target: streaksModelInstance
        function onCloseStreaksView() {
            console.log("Received close streaks signal")
            if (streaksPage.StackView.view) {
                streaksPage.StackView.view.pop()
            }
        }
    }

    FontLoader {
        id: fredoka
        source: "../assets/fonts/Fredoka.ttf"
    }


    // Background
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#FFDE59" }
            GradientStop { position: 1.0; color: "#FFF8DC" }
        }
    }


    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // Header Stats Card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            radius: 15
            color: "white"
            //border.color: "black"

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20

                Column {
                    Layout.fillWidth: true

                    Label {
                        text: "Total Habits"
                        font.pixelSize: 14
                        color: "#666"
                        font.family: fredoka.name

                    }
                    Label {
                        text: streaksModelInstance.totalStreaks
                        font.pixelSize: 28
                        font.bold: true
                        color: "#2c3e50"
                    }
                }

                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#e0e0e0"
                }

                Column {
                    Layout.fillWidth: true

                    Label {
                        text: "Active Today"
                        font.pixelSize: 14
                        color: "#666"
                        font.family: fredoka.name

                    }
                    Label {
                        text: streaksModelInstance.activeStreaks
                        font.pixelSize: 28
                        font.bold: true
                        color: "#27ae60"
                        font.family: fredoka.name

                    }
                }

                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#e0e0e0"
                }

                Button {
                    id: backButton
                    text: "←"
                    flat: true
                    // Material.background: Material.Blue
                    Material.foreground: "grey"
                    Layout.preferredWidth: 80

                    onClicked: {
                        console.log("Back button clicked")
                        streaksModelInstance.closeStreaksView()
                    }
                }
            }
        }

        // Add New Streak Section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            radius: 15
            color: "white"

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                TextField {
                    id: newStreakInput
                    Layout.fillWidth: true
                    placeholderText: "Enter new habit to track..."
                    font.family: fredoka.name
                    font.pixelSize: 14
                    Material.accent: "#333333"
                    onAccepted: {
                        if (text.length > 0) {
                            streaksModelInstance.addStreak(text)
                            text = ""
                        }
                    }
                }

                Button {
                    text: "Add Habit"
                    enabled: newStreakInput.text.length > 0
                    Material.background: "#FFDE59"
                    Material.foreground: "#333333"
                    font.family: fredoka.name


                    onClicked: {
                        streaksModelInstance.addStreak(newStreakInput.text)
                        newStreakInput.text = ""
                    }
                }
            }
        }

        // Streaks List
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true


            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            ListView {
                id: streaksList
                model: streaksModelInstance
                spacing: 10

                delegate: Rectangle {
                    width: streaksList.width
                    height: 100
                    radius: 15
                    color: "white"

                    // Border for broken streaks
                    border.color: {
                        if(model.isBestStreakZero && model.isStreakBroken) return "#95a5a6"
                        else if(model.isStreakBroken) return "#e74c3c"
                        else return "transparent"
                    }

                    border.width: 2

                    // Shadow effect
                    MultiEffect {
                        anchors.fill: parent
                        source: parent
                        shadowEnabled: true
                        shadowColor: "#20000000"
                        shadowBlur: 0.4
                        shadowHorizontalOffset: 0
                        shadowVerticalOffset: 2
                    }

                    // Highlight for active today
                    Rectangle {
                        anchors.fill: parent
                        radius: 15
                        color: model.isActiveToday ? "#27ae60" : "transparent"
                        opacity: 0.1
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 15

                        // Streak Icon/Indicator
                        Rectangle {
                            width: 50
                            height: 50
                            radius: 25
                            // color: {
                            //     if (model.isActiveToday) return "#27ae60"
                            //     else if (model.isStreakBroken) return "#e74c3c"
                            //     else return "#95a5a6"
                            // }

                           color: {

                                if(model.isActiveToday) return "#27ae60"
                                else if (model.isBestStreakZero && model.isStreakBroken) return "#95a5a6"
                                else return "#e74c3c"
                            }

                            Label {
                                anchors.centerIn: parent
                                text: model.streakDuration
                                font.pixelSize: 20
                                font.bold: true
                                color: "white"
                                font.family: fredoka.name

                            }
                        }

                        // Streak Info
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 5

                            Label {
                                text: model.title
                                font.pixelSize: 18
                                font.bold: true
                                color: "#2c3e50"
                                font.family: {
                                    fredoka.name
                                }

                            }

                            RowLayout {
                                spacing: 15

                                Label {
                                    text: "Current: " + model.streakDuration + " days"
                                    font.pixelSize: 13
                                    color: "#7f8c8d"
                                    font.family: fredoka.name

                                }

                                Label {
                                    text: "Best: " + model.bestStreak + " days"
                                    font.pixelSize: 13
                                    color: "#7f8c8d"
                                    font.family: fredoka.name

                                }
                            }

                            Label {
                                text: {
                                    if (model.isActiveToday) return "✓ Completed today"
                                    else if (model.isBestStreakZero && model.isStreakBroken) return "New Streak"
                                    else if (model.isStreakBroken) return "⚠ Streak broken"
                                    else return "Pending today"
                                }
                                font.pixelSize: 12
                                color: {
                                    if (model.isActiveToday) return "#27ae60"
                                    else if (model.isStreakBroken) return "#e74c3c"
                                    else return "#f39c12"
                                }
                                font.family: fredoka.name

                            }
                        }

                        // Action Buttons
                        RowLayout {
                            spacing:5
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                            Button {
                                text: "✓"
                                enabled: !model.isActiveToday
                                Material.background: model.isActiveToday ? Material.Grey : Material.Green
                                Material.foreground: "white"
                                Layout.preferredWidth: 50

                                ToolTip.visible: hovered
                                ToolTip.text: model.isActiveToday ? "Already done today" : "Mark as done"

                                onClicked: {
                                    streaksModelInstance.incrementStreak(index)
                                }
                                font.family: fredoka.name

                            }

                            Button {
                                text: "↻"
                                Material.background: Material.Orange
                                Material.foreground: "white"
                                Layout.preferredWidth: 50

                                ToolTip.visible: hovered
                                ToolTip.text: "Reset streak"

                                onClicked: {
                                    resetDialog.streakIndex = index
                                    resetDialog.streakTitle = model.title
                                    resetDialog.open()
                                }
                                font.family: fredoka.name

                            }

                            Button {
                                text: "X"
                                Material.background: Material.Red
                                Material.foreground: "white"
                                Layout.preferredWidth: 50

                                ToolTip.visible: hovered
                                ToolTip.text: "Delete habit"

                                onClicked: {
                                    deleteDialog.streakIndex = index
                                    deleteDialog.streakTitle = model.title
                                    deleteDialog.open()
                                }
                                font.family: fredoka.name

                            }
                        }
                    }
                }
            }
        }
    }

    // Reset Confirmation Dialog
    Dialog {
        id: resetDialog
        property int streakIndex: -1
        property string streakTitle: ""

        title: "Reset Streak?"
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent

        Label {
            text: "Are you sure you want to reset '" + resetDialog.streakTitle + "'?\nThe current streak will be set to 0, but your best streak will be preserved."
        }

        onAccepted: {
            if (streakIndex >= 0) {
                streaksModelInstance.resetStreak(streakIndex)
            }
        }
    }

    // Delete Confirmation Dialog
    Dialog {
        id: deleteDialog
        property int streakIndex: -1
        property string streakTitle: ""

        title: "Delete Habit?"
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent

        Label {
            text: "Are you sure you want to delete '" + deleteDialog.streakTitle + "'?\nThis action cannot be undone."
        }

        onAccepted: {
            if (streakIndex >= 0) {
                streaksModelInstance.removeStreak(streakIndex)
            }
        }
        Material.accent: "#333333"
    }
}
